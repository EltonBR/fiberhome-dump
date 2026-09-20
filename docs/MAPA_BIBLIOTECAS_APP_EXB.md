# Dependências ELF de `app_exB`

Relatório gerado automaticamente por `tools/mapear-dependencias-elf.sh`.

## Método e limite

O relatório lê `DT_NEEDED` com `readelf -d` de todos os ELF nas partições B e
relaciona apenas as bibliotecas que existem diretamente em `app_exB/`.

- A relação é **direta**: `A → B` significa que o ELF A declara B em
  `DT_NEEDED`.
- A relação não inclui dependências transitivas, `dlopen()`, scripts shell,
  módulos `.ko` ou bibliotecas carregadas por caminho construído em runtime.
- A lista principal é organizada por ELF consumidor. A repetição de uma
  biblioteca em vários binários é intencional.

## Por binário: bibliotecas locais declaradas

### `app_exB/checkcip`

- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libdmzauto.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_pppoe.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_net_adapter.so`
- `libio_msg.so`
- `libl3ctl.so`
- `libnet_if.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libudhcpWeb.so`
- `libudhcpcctl.so`
- `libudhcpdctl.so`

### `app_exB/curl`

- `libcurl.so.4`

### `app_exB/detectHwEvent`

- `libcliom.so`
- `libcm.so`
- `libddns.so`
- `libdhcpL2OmciCfg.so`
- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_advance.so`
- `libgl3_pppoe.so`
- `libgpon_l2.so`
- `libgpon_l3.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_mc_adapter.so`
- `libhi_net_adapter.so`
- `libigmpcfg.so`
- `libio_msg.so`
- `libl3ctl.so`
- `libled_interface.so`
- `libmxml.so`
- `libnet_if.so`
- `libntp_interface.so`
- `libpon_iadcfg.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libtr104.so`
- `libuapi.so`
- `libudhcpWeb.so`
- `libudhcpcctl.so`
- `libudhcpdctl.so`
- `libweb_Msgclient.so`

### `app_exB/dhcp6c`

- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_mc_adapter.so`
- `libhi_net_adapter.so`
- `libipv6.so`
- `libl3ctl.so`
- `libnet_if.so`
- `libpppoe.so`
- `libtr069MsgClient.so`

### `app_exB/dhcp6s`

- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_mc_adapter.so`
- `libhi_net_adapter.so`
- `libipv6.so`
- `libl3ctl.so`
- `libnet_if.so`
- `libpppoe.so`
- `libtr069MsgClient.so`

### `app_exB/dhcpl2`

- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libhi_intf_api.so`
- `libhi_net_adapter.so`
- `libio_msg.so`
- `libnet_if.so`

### `app_exB/dmzconfig`

- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_pppoe.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_net_adapter.so`
- `libio_msg.so`
- `libl3ctl.so`
- `libnet_if.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libudhcpWeb.so`

### `app_exB/dmzinit`

- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_pppoe.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_net_adapter.so`
- `libio_msg.so`
- `libl3ctl.so`
- `libnet_if.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libudhcpWeb.so`

### `app_exB/dnsrelay`

- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_pppoe.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_mc_adapter.so`
- `libhi_net_adapter.so`
- `libl3ctl.so`
- `libnet_if.so`
- `libpppoe.so`
- `libtr069MsgClient.so`

### `app_exB/fh_bsp_led_act`

- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libled_interface.so`

### `app_exB/fh_led_debug`

- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libled_interface.so`

### `app_exB/fhdrv_kdrv_button_debug`

- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`

### `app_exB/fhdrv_kdrv_hwcfg_export`

- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`

### `app_exB/h248`

- `libfh_hi_voipadp.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_pppoe.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libh248Call.so`
- `libhi_intf_api.so`
- `libhi_net_adapter.so`
- `libio_msg.so`
- `libled_interface.so`
- `libnet_if.so`
- `libping.so`
- `libpon_iadcfg.so`
- `libpppoe.so`
- `libprivate_h248.so`
- `libtr069MsgClient.so`

### `app_exB/kdrv_debug`

- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`

### `app_exB/l3mng`

- `libddns.so`
- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_advance.so`
- `libgl3_pppoe.so`
- `libgpon_l2.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_mc_adapter.so`
- `libhi_net_adapter.so`
- `libigmpcfg.so`
- `libl3ctl.so`
- `libled_interface.so`
- `libmxml.so`
- `libnet_if.so`
- `libntp_interface.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libtr104.so`
- `libuapi.so`
- `libudhcpWeb.so`

### `app_exB/libnetdev_cli.so`

- `libnet_if.so`

### `app_exB/load_cli`

- `libcli_adaptation_layer.so`
- `libcli_cli.so`
- `libcliom.so`
- `libcm.so`
- `libddns.so`
- `libdhcpL2Cmd.so`
- `libdhcpL2OmciCfg.so`
- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libfhdrv_kdrv_cli.so`
- `libfhdrv_kdrv_cli_impl.so`
- `libgl3_advance.so`
- `libgl3_cli.so`
- `libgl3_pppoe.so`
- `libgpon_l2.so`
- `libgpon_l2_cli.so`
- `libgpon_l3.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_mc_adapter.so`
- `libhi_net_adapter.so`
- `libi2c_interface.so`
- `libigmpcfg.so`
- `libigmpcli.so`
- `libio_msg.so`
- `libipv6.so`
- `libl3ctl.so`
- `libled_interface.so`
- `libmxml.so`
- `libnatcli.so`
- `libnet_if.so`
- `libnetdev_cli.so`
- `libntp_cli.so`
- `libntp_interface.so`
- `libomci_cli.so`
- `libpon_iadcfg.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libtr069cli.so`
- `libtr104.so`
- `libuapi.so`
- `libudhcpWeb.so`
- `libudhcpcctl.so`
- `libudhcpdctl.so`
- `libvoice_cli.so`
- `libweb_Msgclient.so`

### `app_exB/load_omci`

- `libcliom.so`
- `libcm.so`
- `libddns.so`
- `libdhcpL2OmciCfg.so`
- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_advance.so`
- `libgl3_pppoe.so`
- `libgpon_l2.so`
- `libgpon_l3.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_mc_adapter.so`
- `libhi_net_adapter.so`
- `libigmpcfg.so`
- `libio_msg.so`
- `libl3ctl.so`
- `libled_interface.so`
- `libmxml.so`
- `libnet_if.so`
- `libnomci.so`
- `libntp_interface.so`
- `libpon_iadcfg.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libtr104.so`
- `libuapi.so`
- `libudhcpWeb.so`
- `libudhcpcctl.so`
- `libudhcpdctl.so`
- `libweb_Msgclient.so`

### `app_exB/net_dev_created`

- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libhi_intf_api.so`

### `app_exB/onu_igmpv3`

- `libhi_mc_adapter.so`
- `libigmp.so`
- `libigmpcfg.so`
- `libipi.so`
- `libmc_hal.so`
- `libmld.so`
- `libpal.so`

### `app_exB/pppoeManage`

- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_net_adapter.so`
- `libl3ctl.so`
- `libnet_if.so`
- `libntp_interface.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libtr104.so`

### `app_exB/pppoetest`

- `libgl3_pppoe.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libio_msg.so`
- `libnet_if.so`
- `libonu_bsp.so`
- `libpppoe.so`

### `app_exB/rawpacket`

- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_net_adapter.so`
- `libnet_if.so`

### `app_exB/sip`

- `libfh_hi_voipadp.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_pppoe.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_net_adapter.so`
- `libio_msg.so`
- `libled_interface.so`
- `libnet_if.so`
- `libping.so`
- `libpon_iadcfg.so`
- `libpppoe.so`
- `libprivate_sip.so`
- `libsipTK.so`
- `libsip_oneip.so`
- `libtr069MsgClient.so`

### `app_exB/tr069/bin/agent`

- `libcliom.so`
- `libcm.so`
- `libddns.so`
- `libdhcpL2OmciCfg.so`
- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libfh_tr143_api.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_advance.so`
- `libgl3_pppoe.so`
- `libgpon_l2.so`
- `libgpon_l3.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_mc_adapter.so`
- `libhi_net_adapter.so`
- `libigmpcfg.so`
- `libio_msg.so`
- `libl3ctl.so`
- `libled_interface.so`
- `libmxml.so`
- `libnet_if.so`
- `libnomci.so`
- `libntp_interface.so`
- `libpon_iadcfg.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libtr104.so`
- `libuapi.so`
- `libudhcpWeb.so`
- `libudhcpdctl.so`
- `libweb_Msgclient.so`

### `app_exB/udhcpcforwan`

- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libdmzauto.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_pppoe.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_net_adapter.so`
- `libio_msg.so`
- `libl3ctl.so`
- `libnet_if.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libudhcpWeb.so`
- `libudhcpcctl.so`
- `libudhcpdctl.so`

### `app_exB/udhcpd`

- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libdmzauto.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_pppoe.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_net_adapter.so`
- `libio_msg.so`
- `libl3ctl.so`
- `libnet_if.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libudhcpWeb.so`
- `libudhcpcctl.so`
- `libudhcpdctl.so`

### `app_exB/webs`

- `libcliom.so`
- `libcm.so`
- `libddns.so`
- `libdhcpL2OmciCfg.so`
- `libdhcpcoptionctl.so`
- `libdhcpctl.so`
- `libdmz.so`
- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`
- `libgl3_advance.so`
- `libgl3_pppoe.so`
- `libgpon_l2.so`
- `libgpon_l3.so`
- `libgpon_l3_api.so`
- `libgpon_rm.so`
- `libhi_intf_api.so`
- `libhi_mc_adapter.so`
- `libhi_net_adapter.so`
- `libigmpcfg.so`
- `libio_msg.so`
- `libl3ctl.so`
- `libled_interface.so`
- `libmxml.so`
- `libnet_if.so`
- `libnomci.so`
- `libntp_interface.so`
- `libpon_iadcfg.so`
- `libpppoe.so`
- `libtr069MsgClient.so`
- `libtr104.so`
- `libuapi.so`
- `libudhcpWeb.so`
- `libudhcpcctl.so`
- `libudhcpdctl.so`
- `libweb_Msgclient.so`

### `kernel_rootfsB/bin/fhdrv_kdrv_usb_led`

- `libfhdrv_kdrv_board.so`
- `libfhdrv_kdrv_board_impl.so`


## Bibliotecas declaradas por um único executável

Esta seção ignora consumidores que também são bibliotecas `.so`; ela responde
quais bibliotecas de `app_exB` são declaradas diretamente por exatamente um
executável ELF das três partições B. Isso não comprova exclusividade em runtime
nem ausência de `dlopen()`.

### `app_exB/curl`

- `libcurl.so.4`

### `app_exB/h248`

- `libh248Call.so`
- `libprivate_h248.so`

### `app_exB/load_cli`

- `libcli_adaptation_layer.so`
- `libcli_cli.so`
- `libdhcpL2Cmd.so`
- `libfhdrv_kdrv_cli.so`
- `libfhdrv_kdrv_cli_impl.so`
- `libgl3_cli.so`
- `libgpon_l2_cli.so`
- `libi2c_interface.so`
- `libigmpcli.so`
- `libnatcli.so`
- `libnetdev_cli.so`
- `libntp_cli.so`
- `libomci_cli.so`
- `libtr069cli.so`
- `libvoice_cli.so`

### `app_exB/onu_igmpv3`

- `libigmp.so`
- `libipi.so`
- `libmc_hal.so`
- `libmld.so`
- `libpal.so`

### `app_exB/pppoetest`

- `libonu_bsp.so`

### `app_exB/sip`

- `libprivate_sip.so`
- `libsipTK.so`
- `libsip_oneip.so`

### `app_exB/tr069/bin/agent`

- `libfh_tr143_api.so`


## Apêndice: por biblioteca, quem a declara

Um `—` significa que nenhum ELF das três partições B declarou a biblioteca
diretamente pelo nome presente em `app_exB`; não significa que ela seja
descartável, pois ainda pode haver `dlopen()` ou nome construído em runtime.

| Biblioteca em `app_exB` | Consumidores ELF diretos |
| --- | --- |
| `libbsp_cli.so` | — |
| `libcli_adaptation_layer.so` | app_exB/load_cli |
| `libcli_cli.so` | app_exB/load_cli |
| `libcliom.so` | app_exB/detectHwEvent, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `libcm.so` | app_exB/detectHwEvent, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `libcurl.so` | — |
| `libcurl.so.4` | app_exB/curl |
| `libcurl.so.4.2.0` | — |
| `libddns.so` | app_exB/detectHwEvent, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `libdhcpL2Cmd.so` | app_exB/load_cli |
| `libdhcpL2OmciCfg.so` | app_exB/detectHwEvent, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `libdhcpcoptionctl.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libdhcpctl.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libdmz.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dmzconfig, app_exB/dmzinit, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libdmzauto.so` | app_exB/checkcip, app_exB/udhcpcforwan, app_exB/udhcpd |
| `libfh_bsp_spi.so` | — |
| `libfh_bsp_thread_monitor.so` | — |
| `libfh_hi_voipadp.so` | app_exB/h248, app_exB/sip |
| `libfh_tr143_api.so` | app_exB/tr069/bin/agent |
| `libfhdrv_gpio.so` | — |
| `libfhdrv_kdrv_board.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dhcpl2, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/fh_bsp_led_act, app_exB/fh_led_debug, app_exB/fhdrv_kdrv_button_debug, app_exB/fhdrv_kdrv_hwcfg_export, app_exB/h248, app_exB/kdrv_debug, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/net_dev_created, app_exB/pppoeManage, app_exB/rawpacket, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs, kernel_rootfsB/bin/fhdrv_kdrv_usb_led |
| `libfhdrv_kdrv_board_impl.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dhcpl2, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/fh_bsp_led_act, app_exB/fh_led_debug, app_exB/fhdrv_kdrv_button_debug, app_exB/fhdrv_kdrv_hwcfg_export, app_exB/h248, app_exB/kdrv_debug, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/net_dev_created, app_exB/pppoeManage, app_exB/rawpacket, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs, kernel_rootfsB/bin/fhdrv_kdrv_usb_led |
| `libfhdrv_kdrv_cli.so` | app_exB/load_cli |
| `libfhdrv_kdrv_cli_impl.so` | app_exB/load_cli |
| `libfhdrv_starter_version.so` | — |
| `libgl3_advance.so` | app_exB/detectHwEvent, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `libgl3_cli.so` | app_exB/load_cli |
| `libgl3_pppoe.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/h248, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoetest, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libgpon_l2.so` | app_exB/detectHwEvent, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `libgpon_l2_cli.so` | app_exB/load_cli |
| `libgpon_l3.so` | app_exB/detectHwEvent, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `libgpon_l3_api.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/h248, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/pppoetest, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libgpon_rm.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/h248, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/pppoetest, app_exB/rawpacket, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libh248Call.so` | app_exB/h248 |
| `libhi_fh_bsp.so` | — |
| `libhi_intf_api.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dhcpl2, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/h248, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/net_dev_created, app_exB/pppoeManage, app_exB/rawpacket, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libhi_mc_adapter.so` | app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dnsrelay, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/onu_igmpv3, app_exB/tr069/bin/agent, app_exB/webs |
| `libhi_net_adapter.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dhcpl2, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/h248, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/rawpacket, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libi2c_interface.so` | app_exB/load_cli |
| `libigmp.so` | app_exB/onu_igmpv3 |
| `libigmpcfg.so` | app_exB/detectHwEvent, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/onu_igmpv3, app_exB/tr069/bin/agent, app_exB/webs |
| `libigmpcli.so` | app_exB/load_cli |
| `libio_msg.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcpl2, app_exB/dmzconfig, app_exB/dmzinit, app_exB/h248, app_exB/load_cli, app_exB/load_omci, app_exB/pppoetest, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libipi.so` | app_exB/onu_igmpv3 |
| `libipv6.so` | app_exB/dhcp6c, app_exB/dhcp6s, app_exB/load_cli |
| `libl3ctl.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libl3log.so` | — |
| `libled_interface.so` | app_exB/detectHwEvent, app_exB/fh_bsp_led_act, app_exB/fh_led_debug, app_exB/h248, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/sip, app_exB/tr069/bin/agent, app_exB/webs |
| `libmc_hal.so` | app_exB/onu_igmpv3 |
| `libmld.so` | app_exB/onu_igmpv3 |
| `libmxml.so` | app_exB/detectHwEvent, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `libnatcli.so` | app_exB/load_cli |
| `libnet_if.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dhcpl2, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/h248, app_exB/l3mng, app_exB/libnetdev_cli.so, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/pppoetest, app_exB/rawpacket, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libnetdev_cli.so` | app_exB/load_cli |
| `libnomci.so` | app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `libntp_cli.so` | app_exB/load_cli |
| `libntp_interface.so` | app_exB/detectHwEvent, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/tr069/bin/agent, app_exB/webs |
| `libomci_cli.so` | app_exB/load_cli |
| `libonu_bsp.so` | app_exB/pppoetest |
| `libpal.so` | app_exB/onu_igmpv3 |
| `libping.so` | app_exB/h248, app_exB/sip |
| `libpon_iadcfg.so` | app_exB/detectHwEvent, app_exB/h248, app_exB/load_cli, app_exB/load_omci, app_exB/sip, app_exB/tr069/bin/agent, app_exB/webs |
| `libpppoe.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/h248, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/pppoetest, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libprivate_h248.so` | app_exB/h248 |
| `libprivate_sip.so` | app_exB/sip |
| `libsipTK.so` | app_exB/sip |
| `libsip_oneip.so` | app_exB/sip |
| `libsys.so` | — |
| `libtr069MsgClient.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dhcp6c, app_exB/dhcp6s, app_exB/dmzconfig, app_exB/dmzinit, app_exB/dnsrelay, app_exB/h248, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/sip, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libtr069cli.so` | app_exB/load_cli |
| `libtr104.so` | app_exB/detectHwEvent, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/pppoeManage, app_exB/tr069/bin/agent, app_exB/webs |
| `libuapi.so` | app_exB/detectHwEvent, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `libudhcpWeb.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/dmzconfig, app_exB/dmzinit, app_exB/l3mng, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libudhcpcctl.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/load_cli, app_exB/load_omci, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libudhcpdctl.so` | app_exB/checkcip, app_exB/detectHwEvent, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/udhcpcforwan, app_exB/udhcpd, app_exB/webs |
| `libvoice_cli.so` | app_exB/load_cli |
| `libvsftpd_api.so` | — |
| `libweb_Msgclient.so` | app_exB/detectHwEvent, app_exB/load_cli, app_exB/load_omci, app_exB/tr069/bin/agent, app_exB/webs |
| `rp-pppoe.so` | — |

## Reprodução

```sh
tools/mapear-dependencias-elf.sh > docs/MAPA_BIBLIOTECAS_APP_EXB.md
```

O script é destinado ao computador de análise (Bash e `readelf`), não à ONU.
