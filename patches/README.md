# Contrato dos patches

Coloque aqui patches `*.sh` deliberados e revisados. `tools/patch-rootfs.sh` executa cada um com `ROOTFS` apontando para o filesystem extraído selecionado. Cada patch deve ser idempotente e não deve acessar `firmware-original/`.

O patch `010-desativar-webui-e-cli.sh` é o primeiro patch de diagnóstico para `app_exB`: ele desativa somente a WebUI e a CLI FiberHome. É POSIX `/bin/sh`, compatível com o BusyBox da ONU; executado diretamente nela, altera `/fh/extend/initialize.sh`. No computador, `tools/patch-rootfs.sh app_exB` preserva `extracted/app_exB` e cria/aplica o resultado em `modified/app_exB`. `ROOTFS` é opcional e serve somente para essa aplicação externa. As configurações extraídas permanecem disponíveis e não são alteradas automaticamente.

## PON

`020-desativar-servicos-pon.sh` atua em `app_exB`: impede `hi_xpon_app`,
`hi_kploam`, `load_omci` e a chamada EPON OAM. Ele preserva a criação de
`br0`, `net_dev_created`, Ethernet e UART.

O experimento `021-nao-carregar-drivers-gpon-epon.sh` foi **rejeitado** após
teste real: esses módulos exportam símbolos usados por `hi_khal`,
`hi_knet_adapter`, `hi_koptical` e pela criação das interfaces Ethernet. Em
imagens novas, mantenha esses módulos carregados e use somente o patch 020
para impedir a inicialização funcional PON. Para uma árvore que já recebeu o
patch 021, aplique `022-restaurar-drivers-gpon-epon.sh` em `kernel_rootfsB`.

Como os patches pertencem a partições diferentes, use o aplicador seletivo:

```sh
tools/aplicar-patches.sh app_exB 020-desativar-servicos-pon.sh
tools/aplicar-patches.sh kernel_rootfsB 022-restaurar-drivers-gpon-epon.sh
```

Ele copia somente a partição solicitada de `extracted/` para `modified/` e se
recusa a sobrescrever um resultado existente.

## Serviços WAN e gerenciamento

`030-desativar-servicos-wan-gerenciados.sh` atua em `app_exB` e desativa as
inicializações de `pppoeManage`, `onu_igmpv3`, TR-069 e UPnP. Também desativa a
WebUI quando aplicado isoladamente; quando usado após o patch 010, reconhece a
WebUI como já desativada. Mantém `l3mng`, `dhcpl2`, VoIP/FXS, Ethernet, UART e
o watchdog.

## Watchdog

`040-desativar-watchdog.sh` atua em `app_exB` e impede a inicialização de
`/fh/extend/watchdog`. Assim o daemon não executa `fhdrv_kdrv_wd_toggle` e não
gera as mensagens repetidas no serial. Aplicar em uma árvore já modificada não
remove um watchdog que já esteja em execução: o efeito é observado no próximo
boot.
