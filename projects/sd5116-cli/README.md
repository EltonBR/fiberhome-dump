# sd5116-cli

Base para aplicações CLI em C destinadas à ONU FiberHome SD5116. O primeiro
exemplo, `onu-led`, controla os LEDs e lê os dois botões físicos mapeados em
runtime nesta placa.

`onu-led-direct` é a versão sem dependência de executáveis ou bibliotecas de
userspace FiberHome. Ela abre diretamente `/dev/fhdrv_kdrv_board` e usa a ABI
ioctl recuperada do módulo e de sua biblioteca de referência. Antes de cada
leitura/escrita, ela replica a preparação confirmada no `kdrv_debug`: mux GPIO
e modo entrada/saída.

## Estado do backend

O programa é C nativo, mas usa `execv()` para chamar o utilitário do firmware
`/fh/extend/kdrv_debug`. Isso é intencional: a ABI/ioctl de
`/dev/fhdrv_kdrv_board` ainda não está documentada. Não usa `/bin/sh`, não
monta strings de shell e não escreve arquivos persistentes.

`onu-led-direct` já implementa o backend ioctl. O `onu-led` original permanece
como referência funcional para comparar o comportamento durante os testes.

## Mapa confirmado em runtime

| Item | GPIO | Polaridade |
|---|---:|---|
| Botão Reset | 32 | baixo quando pressionado |
| Botão LED | 14 | baixo quando pressionado |
| LED PON | 31 | baixo acende |
| LED LOS | 30 | inferido ativo-baixo |
| LED VoIP | 6 | inferido ativo-baixo |
| LED LAN1 | 12 | inferido ativo-baixo |
| LED LAN2 | 13 | inferido ativo-baixo |

O LED PON foi testado fisicamente: GPIO 31 em nível baixo o acende. Os demais
compartilham o padrão observado do driver, mas devem ser validados um a um.

`GPIO 33` é a chave geral de LEDs, em nível baixo no boot observado. Ele não é
controlado por este exemplo para evitar apagar todos os indicadores de uma vez.

## Compilação

O padrão do projeto é a toolchain ARMv5/EABI/uClibc-ng estática instalada em
`../../toolchains/armv5-eabi--uclibc--stable-2025.08-1`. Ela é compatível com
o Cortex-A9 e não requer as bibliotecas proprietárias da ONU.

Para preparar somente as dependências comuns do host Debian/Ubuntu:

```sh
./install-deps.sh
```

O projeto usa uClibc-ng para produzir binário ARM EABI **estático**. Isso evita
depender do loader/bibliotecas uClibc proprietárias da ONU. O uso de APIs é
deliberadamente básico e compatível com o kernel 2.6.34.

```sh
make
```

O resultado será `bin/onu-led`. Para usar outro toolchain, sobrescreva
`CROSS_COMPILE`, por exemplo `make CROSS_COMPILE=/caminho/arm-linux-uclibc-`.
Antes de obter o toolchain, valide o código no host:

```sh
make host-check
```

## Uso na ONU

Copie o binário para um diretório gravável, por exemplo `/tmp`, e execute:

```sh
/tmp/onu-led list
/tmp/onu-led led pon on
/tmp/onu-led led pon off
/tmp/onu-led led lan1 on
/tmp/onu-led button reset
/tmp/onu-led buttons
/tmp/onu-led-direct led pon on
/tmp/onu-led-direct led pon off
/tmp/onu-led-direct button reset
```

### Sequência de LEDs

`scripts/led-chase.sh` acende PON, LOS, VoIP, LAN1 e LAN2 por 200 ms cada,
usando somente `onu-led-direct` (ioctl direto). Copie-o para `/tmp` e execute:

```sh
chmod 755 /tmp/led-chase.sh
/tmp/led-chase.sh
/tmp/led-chase.sh --loop
```

Para instalar o binário em outro caminho ou ajustar a duração:

```sh
LEDCTL=/fh/extend/onu-led-direct DELAY=0.5 /tmp/led-chase.sh --loop
```

Não mantenha o botão Reset pressionado se `detectHwEvent` voltar a executar:
o firmware original associa pressões longas à restauração de fábrica.

## I²C: `onu-i2c`

`onu-i2c` oferece uma interface pequena, no estilo de `i2c-tools`, para o
driver que existe nesta imagem. Ela **não** usa `i2c-tools` nem `/dev/i2c-*`:
embora `i2cdev.ko` esteja carregado, a ONU não expõe esses nós de dispositivo.
O backend confirmado é `/fh/extend/kdrv_debug i2c`, chamado por `execv()` sem
shell e sem criar arquivos persistentes.

Há dois canais lógicos aceitos pelo firmware, `0` e `1`; não está confirmado
ainda se representam dois controladores físicos independentes nesta PCB.
Endereços são I²C de 7 bits.

```sh
# Testa uma leitura de um byte, sem escrever.
/tmp/onu-i2c detect 0 0x50

# Lê bytes a partir de um offset (máximo 32 por chamada).
/tmp/onu-i2c read 0 0x50 0x00 16

# Varre somente uma faixa pequena durante a investigação.
/tmp/onu-i2c scan 0 0x50 0x57

# Varredura integral: gera tentativas/timeout para endereços ausentes.
/tmp/onu-i2c scan 0

# Gera leituras I²C continuamente para localizar SDA/SCL no osciloscópio.
# Ctrl+C encerra. --force é obrigatório porque pode disputar o barramento.
/tmp/onu-i2c wave --force 0 0x50 0x00 1

# Padrão repetitivo para trigger, usando o OLED externo já confirmado.
# 0x40 é o byte de controle SSD1306 para dados; 0x55 gera bits alternados.
/tmp/onu-i2c wave-write --force 0 0x3c 0x40 0x55

# Escrita de UM byte. --force é obrigatório.
/tmp/onu-i2c write --force 0 0x50 0x20 0xaa
```

`detect` e `scan` não escrevem dados, mas também não são passivos: uma leitura
I²C normalmente envia o offset de registrador antes de receber o byte. Uma
varredura pode demorar e pode produzir mensagens de timeout no console. Use
primeiro uma faixa curta. Nunca use `write` nos endereços internos sem saber
qual CI responde: `0x50` e `0x51` responderam nesta unidade, mas ainda não há
confirmação de qual deles corresponde à EEPROM HE24C08 nem se há aliases.

O binário é estático e funciona com kernel antigo, mas depende de
`/fh/extend/kdrv_debug` e dos módulos/serviços I²C proprietários já carregados.
A ABI direta do HAL `hi_hal_i2c_*` foi localizada nas bibliotecas do firmware,
porém ainda não foi validada de forma independente; por isso esta ferramenta
não a chama diretamente.

### `wave`: sinal contínuo para osciloscópio

`wave` repete leituras I²C do endereço/offset indicado. Cada iteração emite
START, endereço de escrita, offset, repeated START, endereço de leitura, bytes
e STOP conforme a implementação proprietária do driver. Não escreve bytes de
dados, mas a seleção de offset de alguns CIs altera seu ponteiro interno; por
isso exige `--force`.

O alvo inicialmente recomendado é o endereço já confirmado `0x50`, no canal
`0`, com leitura de um byte:

```sh
/tmp/onu-i2c wave --force 0 0x50 0x00 1
```

Use ponta de alta impedância, GND da sonda no GND da ONU e não curto-circuite
SDA/SCL. O tráfego pode disputar o caminho I²C óptico com drivers ativos; pare
imediatamente se a ONU apresentar falhas e não deixe o teste executando sem
supervisão. A ferramenta não prova que pads suspeitos pertencem ao canal 0:
repita no canal 1, um por vez, se necessário.

### `wave-write`: padrão contínuo para trigger

`wave-write` repete `START + endereço + offset + byte + STOP`. Use somente em
um periférico externo conhecido. Para o OLED SSD1306 validado em `0x3c`, o
comando abaixo gera bytes de dados `0x55` (bits alternados), mais fáceis de
disparar no osciloscópio:

```sh
/tmp/onu-i2c wave-write --force 0 0x3c 0x40 0x55
```

Não usar `wave-write` nos endereços ópticos `0x50` e `0x51`: escrita repetida
pode alterar registros, ponteiros ou estado de dispositivos do caminho óptico.

Para esse modo específico, a mensagem textual de falha do `kdrv_debug` não é
usada como critério de parada: ela foi observada mesmo quando o OLED recebeu a
escrita. O comando só confirma que o helper foi executado; a confirmação
elétrica continua sendo observar SCL/SDA ou o próprio OLED. O comando `write`
normal mantém a validação textual estrita.

### Observação de implementação

O `kdrv_debug` encerra com código zero mesmo quando uma leitura I²C falha. Por
isso `onu-i2c` não considera o status do processo suficiente: para `read`,
`detect` e `scan`, exige a seção `hex data:` que o helper imprime somente após
receber dados e rejeita mensagens `failled!`/`fail.`. Isto evita que uma
varredura reporte todos os endereços como presentes.

## Desenvolvimento futuro

- trocar o backend para ioctl direto do driver de placa;
- acrescentar debounce e eventos por polling para botões;
- criar comandos de Ethernet, rede estática, diagnósticos e LEDs de atividade;
- instalar somente em `modified/app_exB/` ou via overlay de teste, nunca em
  `extracted/`.
