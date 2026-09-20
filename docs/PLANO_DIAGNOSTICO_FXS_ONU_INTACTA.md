# Plano de diagnóstico FXS com ONU intacta

Objetivo: observar a pilha original da segunda ONU e identificar a sequência
mínima para usar a porta telefônica como FXS — telefone analógico, toque, áudio
e, posteriormente, SIP em servidor local. Esta ONU não recebe overlay, patch,
gravação MTD ou mudança persistente durante as fases 0–3.

O SLIC observado na outra unidade foi identificado como Microchip/Microsemi
LE9641. Isso é uma pista, não uma garantia de que a ONU intacta tenha a mesma
PCB, SLIC ou `hwcfg`; registrar a identificação antes de comparar.

## Fase 0 — Preparação segura

1. Ligar a ONU intacta à UART em 115200 baud e capturar o boot integral em um
   arquivo no PC.
2. Não conectar fibra/OLT durante a primeira coleta. Ethernet para LAN isolada
   é suficiente.
3. Não conectar telefone, fax ou osciloscópio à porta FXS nesta fase.
4. Não executar `slic/kinit`, `slic/*spi*`, `voip/kopenchannal`,
   `voip/kgeneraltone`, `ploam/*`, `optical/*`, `phymem_write` ou comandos
   `*_set`/`*_write`.
5. Guardar a coleta no PC com data e identificação física da unidade, sem
   reutilizar MAC ou serial como constante de scripts.

## Fase 1 — Snapshot runtime sem escrita

No shell root da ONU intacta, executar e salvar a saída. `/tmp` é suficiente:

```sh
mkdir -p /tmp/fxs-diag

uname -a > /tmp/fxs-diag/uname.txt
cat /proc/cmdline > /tmp/fxs-diag/cmdline.txt
cat /proc/mtd > /tmp/fxs-diag/mtd.txt
mount > /tmp/fxs-diag/mount.txt
ps > /tmp/fxs-diag/ps.txt
lsmod > /tmp/fxs-diag/lsmod.txt
ifconfig -a > /tmp/fxs-diag/ifconfig.txt
netstat -anp > /tmp/fxs-diag/netstat.txt
find /dev -maxdepth 2 \( -type c -o -type b \) > /tmp/fxs-diag/devices.txt
find /sys/module -maxdepth 1 -mindepth 1 -type d > /tmp/fxs-diag/modules.txt
dmesg > /tmp/fxs-diag/dmesg.txt
```

Separar a pilha de telefonia e a sequência de boot:

```sh
lsmod | grep -Ei 'slic|voip|pcm|spi|i2c|gpon|epon' > /tmp/fxs-diag/modules-fxs.txt
ps | grep -Ei 'sip|h248|voip|slic' > /tmp/fxs-diag/processes-fxs.txt
find /fh/extend /fh/bin /usr/sbin -maxdepth 2 -type f \( -name '*slic*' -o -name '*voip*' -o -name 'sip' -o -name 'h248' \) > /tmp/fxs-diag/files-fxs.txt
```

Copiar também, para análise estática, sem modificar nada:

```sh
cp /fh/extend/initialize.sh /tmp/fxs-diag/initialize.sh
cp /etc/rc.d/rcS /tmp/fxs-diag/rcS
cp /fh/extend/voip_preStart /tmp/fxs-diag/ 2>/dev/null
cp /usr/sbin/hi_slic_preinit /tmp/fxs-diag/ 2>/dev/null
cp /fhcfg/voip.conf /tmp/fxs-diag/ 2>/dev/null
```

Transferir o diretório ao PC com `onu-transfer pull` ou salvar a saída serial.

## Fase 2 — Processos, módulos e arquivos consumidos

Para cada PID de `sip`, `h248`, `voip_preStart` ou processo VoIP encontrado,
coletar apenas metadados do kernel:

```sh
PID=<pid>
cat /proc/$PID/status > /tmp/fxs-diag/$PID-status.txt
cat /proc/$PID/maps > /tmp/fxs-diag/$PID-maps.txt
ls -l /proc/$PID/fd > /tmp/fxs-diag/$PID-fd.txt
tr '\000' ' ' < /proc/$PID/cmdline > /tmp/fxs-diag/$PID-cmdline.txt
```

Isso revela bibliotecas carregadas, argumentos e devices abertos sem usar
tracing invasivo. Procurar especialmente por `/dev/iomsg`, `/dev/fh_omci`,
nodes SLIC, PCM, SPI e arquivos em `/fhcfg`.

Comparar o boot original com o `new-overlay`:

```text
original: hi_kslic_common + hi_kvoip + hi_slic_preinit + voip_preStart
overlay:  módulos VoIP/SLIC desativados e nenhum processo de telefonia
```

O resultado esperado desta fase é a lista exata de módulos, binários,
bibliotecas e configuração que a porta FXS exige — não uma hipótese baseada em
outro modelo.

## Fase 3 — CLI, somente consultas

Primeiro salvar os descritores; eles mostram parâmetros sem fazer chamadas:

```sh
mkdir -p /tmp/fxs-diag/cli
cp -r /home/cli/slic /tmp/fxs-diag/cli/
cp -r /home/cli/voip /tmp/fxs-diag/cli/
```

Depois, somente se os módulos SLIC/VoIP estiverem carregados, executar estas
consultas e registrar a saída:

```sh
/usr/bin/cli /home/cli/slic/kget_chiptype > /tmp/fxs-diag/cli-chiptype.txt 2>&1
/usr/bin/cli /home/cli/slic/kregister_dump > /tmp/fxs-diag/cli-registers.txt 2>&1
/usr/bin/cli /home/cli/voip/kchipstat > /tmp/fxs-diag/cli-chipstat.txt 2>&1
/usr/bin/cli /home/cli/voip/kchanstat > /tmp/fxs-diag/cli-chanstat.txt 2>&1
/usr/bin/cli /home/cli/voip/kallchancodecstate > /tmp/fxs-diag/cli-codec.txt 2>&1
```

Esses descritores são de leitura/dump pelo nome e não recebem `-v`. Se algum
comando pedir parâmetros ou retornar erro, parar e registrar a saída; não
substituir por comandos `set`, `write`, `init` ou SPI.

## Fase 4 — Identificar a cadeia de áudio e sinalização

Com as coletas no PC, responder nesta ordem:

1. Qual módulo SLIC corresponde ao CI físico (`hi_kslic_zl`, `hi_kslic_lt` ou
   outro)?
2. Qual binário inicializa a alimentação, reset e SPI do SLIC:
   `hi_slic_preinit`, `voip_preStart` ou ambos?
3. Qual processo mantém o canal: `sip`, `h248` ou outro daemon?
4. Quais arquivos de `/fhcfg` são realmente abertos pelos processos?
5. Qual interface/canal representa a FXS: `tel0`, `voip0`, `voip1` ou `voip2`?
6. A pilha opera mesmo sem GPON/OMCI/WebUI, ou requer alguma delas?

Somente depois dessas respostas será montada uma cadeia mínima na ONU
modificada. A primeira reprodução deve usar cópias em `/tmp` e não alterar
`rcS` ou filesystem persistente.

## Fase 5 — Teste elétrico e telefone, posterior

Somente após conhecer a sequência de inicialização:

1. Medir, com a linha em repouso e multímetro de alta impedância, tensão DC
   entre Tip e Ring. Registrar polaridade e valor.
2. Não curto-circuitar Tip/Ring. Não medir ringing AC com instrumento cuja
   faixa/categoria não suporte a tensão esperada de telefonia.
3. Conectar um telefone analógico simples, não fax/modem, e observar se há tom
   de discagem, loop current e ring após inicialização original.
4. Para áudio e toque controlados, apontar a configuração original para um
   servidor SIP local na LAN ou reproduzir os parâmetros em uma ONU de teste.
5. Fax/modem vem por último: primeiro confirmar chamada SIP estável, codec
   adequado e ausência de perda de áudio. FXS não transforma automaticamente a
   ONU em modem; ela fornece a interface analógica para o equipamento externo.

## Critério de sucesso

O diagnóstico está completo quando houver evidência para uma tabela:

| Etapa | Módulo/binário | Biblioteca | Configuração | Resultado observável |
|---|---|---|---|---|
| inicialização elétrica | a preencher | a preencher | a preencher | SLIC identificado/linha energizada |
| canal FXS | a preencher | a preencher | a preencher | telefone detecta tom |
| sinalização | a preencher | a preencher | SIP/H.248 | telefone toca |
| mídia | a preencher | a preencher | codec/RTP | áudio bidirecional |

Não passar de fase sem preservar os arquivos de evidência da fase anterior.
