# Dependências de boot do `new-overlay`

Fonte desta análise: `new-overlay/etc/rc.d/rcS` e
`new-overlay/app_exB/initialize.sh`, comparados aos arquivos originais em
`extracted/`. É uma análise estática do fluxo atualmente escrito no overlay;
ela não autoriza apagar arquivos da imagem nem módulos da NAND.

## Fluxo efetivamente executado

```text
init
 └─ rcS
    ├─ BusyBox: mount, mdev, telnetd -p 23
    ├─ /bin/fhdrv_kdrv_mount
    │  └─ monta /fh/bin, /fh/extend e /fhcfg
    ├─ insmod dos módulos listados no rcS
    └─ /fh/extend/initialize.sh
       ├─ source mk_cfg_dir.sh
       ├─ source cp_cfg.sh
       ├─ fhdrv_kdrv_board_impl_load.sh
       │  ├─ fhdrv_kdrv_board_load.sh
       │  ├─ insmod fhdrv_kdrv_board_impl.ko
       │  └─ /bin/fhdrv_kdrv_usb_led
       ├─ insmod iomsg_drv.ko; cria /dev/iomsg e /dev/fh_omci
       ├─ insmod i2cdev.ko
       ├─ hi_xpon_app gpon 2 4095
       ├─ insmod hi_kploam.ko
       ├─ net_dev_created
       ├─ ifconfig br0 / route / ifconfig lo
       ├─ manutenção condicional de /fhcfg/cpepatch e dhcpforwan
       └─ fh_ver_export.sh
```

`/bin/sh` no final de `rcS` mantém o shell serial. `telnetd -p 23` é iniciado
antes da montagem das aplicações e não depende de binário em `app_exB`.

## Executáveis e bibliotecas dinâmicas

As árvores abaixo são `DT_NEEDED`, obtidas com `readelf -d`. `libc.so.0`,
`libpthread.so.0` e `libm.so.0` são a uClibc do rootfs. As bibliotecas `libhi_*`
ficam em `/lib/hsan/so/service`; as bibliotecas de placa ficam em `app_exB` ou
em `/lib`.

### Montagem das partições

```text
/bin/fhdrv_kdrv_mount
 ├─ libfhdrv_kdrv_hi_adapter.so
 │  └─ libc.so.0
 └─ libc.so.0
```

Função: monta as aplicações A/B e `cfg` conforme o environment U-Boot. É
necessário para que `/fh/extend`, `/fh/bin` e `/fhcfg` existam; não remover.

### Driver de placa e LED USB

```text
/bin/fhdrv_kdrv_usb_led
 ├─ libfhdrv_kdrv_board_impl.so ── libc.so.0
 ├─ libfhdrv_kdrv_board.so ─────── libc.so.0
 ├─ libfhdrv_kdrv_hi_adapter.so ── libc.so.0
 ├─ libhi_hal.so ───────────────── libhi_ubasic.so ── libpthread.so.0, libc.so.0
 ├─ libhi_ipc.so ───────────────── libhi_ubasic.so, libhi_ioreactor.so, libc.so.0
 ├─ libhi_ioreactor.so ─────────── libc.so.0
 └─ libpthread.so.0, libc.so.0
```

Antes dele, os scripts carregam `fhdrv_kdrv_board.ko`, criam
`/dev/fhdrv_kdrv_board` e carregam `fhdrv_kdrv_board_impl.ko`. Esta base é
necessária para GPIO direto e para as bibliotecas locais `libfh_gpio` e
`libfh_i2c`. O utilitário `fhdrv_kdrv_usb_led` apenas procura portas USB em
sysfs e ajusta LEDs se encontrá-las; não habilita USB. É candidato a deixar de
ser chamado, mas os dois módulos de placa devem permanecer.

### Inicialização XPON

```text
hi_xpon_app gpon 2 4095
 ├─ libhi_hal.so ─────────────── libhi_ubasic.so, libc.so.0
 ├─ libhi_ipc.so ─────────────── libhi_ubasic.so, libhi_ioreactor.so, libc.so.0
 ├─ libhi_ipc_cmd.so ─────────── libpthread.so.0, libhi_ubasic.so,
 │                                libhi_ioreactor.so, libhi_ipc.so, libm.so.0, libc.so.0
 ├─ libhi_ioreactor.so ───────── libc.so.0
 ├─ libhi_ubasic.so ──────────── libpthread.so.0, libc.so.0
 └─ libc.so.0
```

O binário recebe modo, número de portas Ethernet e VLAN padrão por argumento;
as strings não revelam leitura direta de configuração persistente. No overlay,
os valores são fixos: `gpon 2 4095`. Ele roda antes de `net_dev_created`.
Como a remoção prévia dos drivers GPON/EPON interrompeu Ethernet, este binário
e sua cadeia devem ser mantidos até existir uma sequência Ethernet alternativa
validada.

`hi_kploam.ko`, carregado depois, depende de `hi_kbasic`, `hi_gpon` e `hi_spi`.

### Criação de interfaces de rede

```text
net_dev_created
 ├─ libhi_intf_api.so ────────── libhi_ubasic.so, libhi_ipc.so, libc.so.0
 ├─ libhi_hal.so ─────────────── libhi_ubasic.so, libc.so.0
 ├─ libhi_ipc.so ─────────────── libhi_ubasic.so, libhi_ioreactor.so, libc.so.0
 ├─ libhi_log_cmd.so ─────────── libpthread.so.0, libc.so.0
 ├─ libhi_ubasic.so ──────────── libpthread.so.0, libc.so.0
 ├─ libhi_ioreactor.so ───────── libc.so.0
 ├─ libfhdrv_kdrv_hi_adapter.so ─ libc.so.0
 ├─ libfhdrv_kdrv_board_impl.so ─ libc.so.0
 ├─ libfhdrv_kdrv_board.so ────── libc.so.0
 └─ libpthread.so.0, libc.so.0
```

Ele configura interfaces `eth*`, `wan*`, `pie0`, `tel0` e `voip*`, e cria
`br0`; não foram encontradas strings de caminho para `.conf`, `.ini` ou
`/fhcfg` nele. Nesta configuração, ele é indispensável: a própria sequência
depois atribui IP e rota a `br0`.

### Diagnóstico de versão

`fh_ver_export.sh` não usa ELF adicional. Ele percorre `/sys/module/*` e grava
parâmetros `fh_moduler_version` em `/dev/shm/fh_version_ko`. Não há consumidor
ativo desse arquivo no overlay. É removível do boot se esse diagnóstico não for
necessário.

## Configurações tocadas pelo fluxo atual

| Caminho | Quem toca | Uso no overlay | Observação |
|---|---|---|---|
| `/fhcfg` | `fhdrv_kdrv_mount` | montagem persistente | manter |
| `/fhcfg/fh_pon/pon_type` | `initialize.sh` | cria `gpon` se ausente | o overlay não o lê depois; `hi_xpon_app` recebe `gpon` fixo |
| `/fhcfg/fh_wifi`, `fh_pon`, `cpepatch`, `l3_def`, `ppp`, `extend/dhcpforwan` | `mk_cfg_dir.sh` | cria diretórios | `ppp`, `l3_def` e Wi-Fi não são consumidos no boot atual |
| `/fhcfg/ppp/*`, `pppoe*.conf`, `pppoesim.conf`, `voip.conf`, `dhcpc.script`, `l3_def/udhcpd.conf` | `cp_cfg.sh` | copia defaults se ausentes | os serviços PPPoE, VoIP, DHCP e DHCP server estão desativados; cópias dispensáveis se o script for removido |
| `/fhcfg/extend/delete_flag_after_update`, `/fhcfg/cpepatch/use_edit_boot` | `initialize.sh` | limpeza pós-upgrade | dispensável sem fluxo de atualização FiberHome |
| `/fh/extend/boot_version_control` | `initialize.sh` | copia para `cpepatch` | dispensável junto com o bloco pós-upgrade |
| `/proc/driver/fh_hw_cfg` | `initialize.sh` | identifica hardware | lido apenas para imprimir `DEV` nesta variante (`0x24`) |
| `/proc/driver/fh_hw_cfg_for_hi_bridge` | `rcS` | argumento para `hi_bridge.ko` | necessário ao carregamento atual da bridge |

## Itens removidos do fluxo pelo overlay

Estes binários não são chamados pelos scripts novos e não ficam residentes
como consequência deles:

```text
fh_printf_redirect / fh_printf_redirect.sh
fh_bsp_led_act, detectHwEvent
rm_init.exe, l3mng, dhcpl2, onu_igmpv3
sip, h248, voip_preStart
dmz-init, pppoeManage
load_omci (e o ramo `epon_oam`, ausente em `app_exB`)
runTr069, runWeb.sh, webs
upnp/upnp-init, ntpclient, watchdog, load_cli
```

Suas bibliotecas e configurações não são mais necessárias **para o boot atual**.
Isso não prova que possam ser apagadas da partição: uma invocação manual, um
teste futuro ou outro script ainda pode precisar delas. Para reduzir a imagem,
primeiro substituir/remover os scripts que os chamavam e validar boot, serial,
Ethernet e I²C; depois reexecutar este levantamento sobre a árvore resultante.

### Fechamento de bibliotecas dos serviços desativados

Esta tabela agrupa as dependências diretas (`DT_NEEDED`) dos binários que o
overlay deixou de chamar. Bibliotecas de base repetidas — `libc`, `libpthread`,
`libhi_hal`, `libhi_ubasic`, `libhi_ipc`, `libhi_ioreactor`,
`libfhdrv_kdrv_board*` e `libfhdrv_kdrv_hi_adapter` — **não** são candidatas a
remoção: continuam necessárias para `net_dev_created`, GPIO ou I²C.

| Família removida | Binários | Bibliotecas específicas ou de camada alta observadas |
|---|---|---|
| Log e LEDs do fornecedor | `fh_printf_redirect`, `fh_bsp_led_act` | `libled_interface.so`; `fh_printf_redirect` só usa uClibc/pthread |
| Eventos de botão/hardware | `detectHwEvent` | `libtr104`, `libigmpcfg`, `libtr069MsgClient`, `libgpon_l2`, `libgpon_l3`, `libio_msg`, `libl3ctl`, `libgl3_pppoe`, `libgpon_l3_api`, `libpppoe`, `libdhcpctl`, `libdhcpcoptionctl`, `libgl3_advance`, `libgpon_rm`, `libntp_interface`, `libcliom`, `libpon_iadcfg`, `libdhcpL2OmciCfg`, `libcm`, `libddns`, `libdmz`, `libudhcpWeb`, `libuapi`, `libweb_Msgclient`, `libudhcpdctl`, `libudhcpcctl` |
| L3, PPP e DHCP | `l3mng`, `dhcpl2`, `pppoeManage`, `rm_init.exe`, `dmz-init` | `libl3ctl`, `libgl3_pppoe`, `libgpon_l3_api`, `libnet_if`, `libpppoe`, `libdhcpctl`, `libdhcpcoptionctl`, `libgl3_advance`, `libntp_interface`, `libgpon_rm`, `libddns`, `libdmz`, `libudhcpWeb`, `libgpon_l2`, `libmxml`; `rm_init.exe` só usa uClibc; `dmz-init` é script não-ELF |
| Multicast | `onu_igmpv3` | `libmc_hal`, `libigmpcfg`, `libigmp`, `libmld`, `libipi`, `libpal`, `libhi_mc_adapter`, `libhi_ploam` |
| Telefonia | `sip`, `h248`, `voip_preStart` | `libsip_oneip`, `libsipTK`, `libprivate_sip`, `libprivate_h248`, `libh248Call`, `libping`, `libhi_voip`, `libhi_slic`, `libfh_hi_voipadp`, `libtr104` |
| OMCI/GPON de userspace | `load_omci`, `epon_oam` se presente em outra variante | `libnomci`, `libpon_iadcfg`, `libcliom`, `libgpon_l2`, `libgpon_l3`, `libgpon_l3_api`, `libgpon_rm`, `libdhcpL2OmciCfg`, além das bibliotecas L3/gestão acima |
| TR-069 e WebUI | `tr069/bin/runTr069`, `runWeb.sh`, `webs` | `libcrypto.so.1.0.0`, `libweb_Msgclient`, `libtr069MsgClient`, `libtr104`, `libhi_omci_adapter`, `libnomci`, `libdhcpL2OmciCfg`; `runWeb.sh` é script e inicia `webs` |
| UPnP, NTP, watchdog e CLI | `upnp/upnp-init`, `ntpclient`, `watchdog`, `load_cli` | UPnP usa suas bibliotecas no subdiretório `upnp`; `ntpclient` só usa uClibc/pthread; `watchdog` usa a HAL; `load_cli` puxa `libcli_adaptation_layer`, `libcli_cli`, `libi2c_interface`, `libfhdrv_kdrv_cli*`, `libgpon_l2_cli`, `libnetdev_cli`, `libigmpcli`, `libtr069cli`, `libgl3_cli`, `libipv6`, `libntp_cli`, `libdhcpL2Cmd`, `libnatcli`, `libvoice_cli`, `libomci_cli` e as bibliotecas de gestão já listadas |

Uma biblioteca dessa tabela só pode ser considerada removível quando nenhuma
linha restante de `rcS`, `initialize.sh`, ferramenta manual ou binário mantido
mostrar `DT_NEEDED` para ela. O comando de verificação está no final deste
documento.

## Classificação de remoção para o Linux mínimo

Esta classificação vale exclusivamente para uma **imagem mínima nova baseada
no `new-overlay` atual**. Ela foi feita depois de verificar que os nomes abaixo
não aparecem em linha executável do overlay; `fh_printf_redirect`,
`fh_bsp_led_act` e `detectHwEvent` aparecem somente em comentário. Não apagar
arquivos da ONU em execução: primeiro excluí-los da imagem de teste e validar o
boot pelo slot de recuperação.

### Seguro excluir da imagem mínima atual

Estes executáveis não são chamados pelo `new-overlay`, não são `DT_NEEDED` das
raízes mantidas (`hi_xpon_app`, `net_dev_created`, montagem e GPIO/I²C) e seus
serviços já foram desativados deliberadamente:

| Grupo | Arquivos executáveis/scripts que podem ser excluídos | Recurso que deixa de existir |
|---|---|---|
| Log/indicadores | `fh_printf_redirect`, `fh_printf_redirect.sh`, `fh_bsp_led_act`, `detectHwEvent` | redirecionamento de log, política automática de LEDs e botões FiberHome |
| Roteador L3 | `rm_init.exe`, `l3mng`, `dhcpl2`, `onu_igmpv3`, `dmz-init`, `pppoeManage` | PPPoE, DHCP L2/servidor, IGMP, DMZ e roteamento do fornecedor |
| Telefonia | `sip`, `h248`, `voip_preStart` | SIP/H.248/VoIP; VoIP também não tem módulos carregados no overlay |
| Gestão PON | `load_omci`; `epon_oam` caso exista em outra árvore | OMCI/EPON userspace; não remove os drivers GPON/EPON do kernel |
| Gestão remota/Web | `tr069/bin/runTr069`, `runWeb.sh`, `webs`, diretório `web/`, `upnp/upnp-init` | TR-069, WebUI e UPnP |
| Serviços auxiliares | `ntpclient`, `watchdog`, `load_cli` | NTP, watchdog de userspace e prompt/login FiberHome |

O shell serial final de `rcS` e o `telnetd -p 23` continuam disponíveis mesmo
sem `load_cli`. Remover `detectHwEvent` elimina também qualquer tratamento
automático de pressão longa do botão Reset; os testes diretos de GPIO continuam
possíveis através de `/dev/fhdrv_kdrv_board`.

### Seguro excluir junto com os grupos acima, após busca de referências

As bibliotecas abaixo pertencem apenas às famílias que o overlay desativou e
não estão na árvore direta das raízes mantidas. Elas são candidatas de exclusão
na mesma imagem mínima, desde que uma busca `readelf` sobre os binários que
restarem não encontre referência:

```text
# Web, gestão e TR-069
libweb_Msgclient.so  libtr069MsgClient.so  libtr104.so  libcliom.so
libcli_adaptation_layer.so  libcli_cli.so  libtr069cli.so  libweb_Msgclient.so
libomci_cli.so  libhi_omci_adapter.so  libnomci.so

# Roteador, PPP, DHCP, multicast e IPv6 de userspace
libl3ctl.so  libgl3_pppoe.so  libgl3_advance.so  libgpon_l3.so
libgpon_l3_api.so  libgpon_l2.so  libgpon_rm.so  libpppoe.so
libdhcpctl.so  libdhcpcoptionctl.so  libudhcpdctl.so  libudhcpcctl.so
libudhcpWeb.so  libdhcpL2OmciCfg.so  libdhcpL2Cmd.so  libigmpcfg.so
libigmp.so  libmld.so  libipi.so  libpal.so  libmc_hal.so  libipv6.so
libnetdev_cli.so  libigmpcli.so  libgl3_cli.so  libnatcli.so

# Voz
libhi_voip.so  libhi_slic.so  libfh_hi_voipadp.so  libsip_oneip.so
libsipTK.so  libprivate_sip.so  libprivate_h248.so  libh248Call.so
libvoice_cli.so  libping.so

# Serviços desligados diversos
libled_interface.so  libntp_interface.so  libntp_cli.so  libddns.so
libdmz.so  libcm.so  libmxml.so  libuapi.so  libio_msg.so
```

`libcrypto.so.1.0.0` pode sair se WebUI/TR-069 não forem mantidos e nenhuma
ferramenta manual restante a usar. Não apagar por associação as bibliotecas de
base `libhi_*` do quadro anterior: algumas continuam necessárias à rede, GPIO
ou I²C direta.

### Não excluir ainda

```text
/bin/fhdrv_kdrv_mount
hi_xpon_app
net_dev_created
fhdrv_kdrv_board_load.sh
fhdrv_kdrv_board_impl_load.sh
fhdrv_kdrv_board.ko
fhdrv_kdrv_board_impl.ko
libfhdrv_kdrv_board.so
libfhdrv_kdrv_board_impl.so
libfhdrv_kdrv_hi_adapter.so
libhi_ubasic.so  libhi_ioreactor.so  libhi_ipc.so  libhi_hal.so
libhi_ipc_cmd.so  libhi_log_cmd.so  libhi_intf_api.so
```

Também manter, por enquanto, `hi_bridge`, `hi_gpon`, `hi_epon`, `hi_l3`,
`hi_oam`, `hi_knet_adapter` e os módulos CFE carregados pelo `rcS`.

### Próximas remoções de baixo risco no *boot*, não no filesystem

Estas linhas podem ser comentadas/removidas do overlay uma por vez; os arquivos
podem ficar na imagem inicialmente:

1. chamada a `/bin/fhdrv_kdrv_usb_led`;
2. chamada a `fh_ver_export.sh`;
3. `insmod /fh/extend/i2cdev.ko` — a biblioteca atual usa HAL, não `/dev/i2c-*`;
4. `source cp_cfg.sh` e `source mk_cfg_dir.sh`, depois de decidir abandonar
   PPP/VoIP/DHCP e atualização FiberHome;
5. blocos de limpeza/cópia pós-upgrade em `initialize.sh`.

## Candidatos de simplificação adicionais

| Item ainda chamado/carregado | Motivo para manter agora | Condição para remover do overlay |
|---|---|---|
| `mk_cfg_dir.sh` e `cp_cfg.sh` | compatibilidade com configuração FiberHome | remover as duas linhas `source`; não iniciar PPP/VoIP/DHCP depois |
| `iomsg_drv.ko`, `/dev/iomsg`, `/dev/fh_omci` | possivelmente parte da cadeia XPON | confirmar, em runtime, que `hi_xpon_app`/módulos PON não abrem esses nós |
| `i2cdev.ko` | o overlay ainda o carrega | `libfh_i2c` usa HAL, não `/dev/i2c-*`; remover o `insmod` e validar OLED/I²C |
| `fhdrv_kdrv_usb_led` | só política de LED USB | remover apenas sua chamada; manter módulos de placa |
| `fh_ver_export.sh` | relatório de versão em tmpfs | nenhum consumidor necessário |
| blocos pós-upgrade | preservam semântica do firmware do fornecedor | não usar atualização/cpepatch FiberHome |
| IPv6, SIT, IPIP, XFRM e `nf_conntrack_ipv6` | carregados por `rcS`, sem uso por IP estático IPv4 | não precisar IPv6, túneis ou firewall associado |
| `nf_conntrack*` IPv4 | carregados por `rcS`; não são necessários para atribuir IP/rota estáticos | não usar NAT, firewall stateful ou serviços que dependam de conntrack |

Não classificar `hi_bridge`, `hi_gpon`, `hi_epon`, `hi_l3`, `hi_oam`,
`hi_knet_adapter`, CFE ou `hi_xpon_app` como removíveis: a experiência anterior
mostrou que a remoção de componentes PON pode quebrar Ethernet.

## Comando de reprodução estática

```sh
readelf -d extracted/app_exB/net_dev_created
readelf -d extracted/app_exB/hi_xpon_app
readelf -d extracted/kernel_rootfsB/bin/fhdrv_kdrv_usb_led
modinfo extracted/kernel_rootfsB/lib/hsan/ko/service/hi_kploam.ko
rg -n '(^|[;&|[:space:]])(\./|/fh/extend/)' new-overlay
```

Para validar uma remoção, usar uma única alteração no overlay, iniciar pelo
slot de teste, conferir shell serial, `br0`, link Ethernet e OLED em I²C antes
de tentar a próxima.
