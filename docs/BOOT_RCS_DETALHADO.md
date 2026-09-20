# Boot detalhado a partir de `rcS`

Esta análise descreve o boot normal a partir da base imutável `extracted/`. Os caminhos `/fh/bin` e `/fh/extend` são escolhidos dinamicamente pelos flags A/B do U-Boot. Na unidade analisada os flags são B, portanto os arquivos vêm de `mtd9` e `mtd10`.

## Árvore de execução confirmada

```text
BusyBox init
└── /etc/rc.d/rcS
    ├── source /etc/profile
    ├── /bin/fhdrv_kdrv_mount             [binário]
    └── /fh/extend/initialize.sh
        ├── source mk_cfg_dir.sh
        ├── source cp_cfg.sh
        ├── fhdrv_kdrv_board_impl_load.sh
        │   └── fhdrv_kdrv_board_load.sh
        ├── source fh_printf_redirect.sh
        ├── fh_ver_export.sh
        ├── tr069/bin/runTr069
        ├── runWeb.sh
        ├── upnp/upnp-init                 [background]
        │   └── upnp/upnp-run
        └── load_cli                       [binário, primeiro plano]
```

`rcS` só alcança o seu `/bin/sh` final se `initialize.sh` retornar. Em boot normal, `load_cli` é a última chamada de `initialize.sh` e não é colocada em background; ela toma o console com a CLI FiberHome.

## 1. BusyBox init e `rcS`

`/etc/inittab` contém `console::sysinit:-/etc/rc.d/rcS`; assim, `rcS` é executado pelo init como root.

1. **`source /etc/profile`** — acrescenta bibliotecas FiberHome a `LD_LIBRARY_PATH` e `/fh/bin`, `/fh/extend` e `/fhcfg` a `PATH`.
2. **Console e pseudo-filesystems** — limpa a tela, monta `proc`, `sysfs`, `tmpfs` em `/dev`, `/tmp` e `/var`, cria `/dev/shm` e monta `devpts`.
3. **Gerência de dispositivos** — `mdev -s` popula `/dev`; grava `/sbin/mdev` em `/proc/sys/kernel/hotplug` para hotplug futuro.
4. **Montagem das partições de aplicação** — `fhdrv_kdrv_mount` é binário. Strings confirmam que monta:
   - `/dev/mtdblock11` em `/fhcfg` como JFFS2;
   - `mtd6` ou `mtd9` em `/fh/bin`, conforme `app_bin_curr_flag`;
   - `mtd7` ou `mtd10` em `/fh/extend`, conforme `app_ex_curr_flag`.
5. **Módulos básicos** — carrega base HSAN (`hi_kbasic`, `hi_kipc`, `hi_klog_cmd`), depois sysctl, MDIO, PIE, entrega de pacotes, GPIO, I²C, timer, SPI e hardware.
6. **Módulos de acesso/PON** — obtém `hwcfg` de `/proc/driver/fh_hw_cfg_for_hi_bridge`, carrega bridge, GPON, EPON, L3, OAM e CNT. Em seguida carrega HAL, adaptadores CFE/rede, óptica, operação, SLIC e VoIP.
7. **Netfilter e túneis** — carrega conntrack, NAT FTP/TFTP, IPv4/IPv6 e túneis. Isto não configura a LAN; apenas disponibiliza a pilha e módulos.
8. **PIE inicial** — configura `pie0` com `192.168.0.10/24`.
9. **Chamada de alto nível** — executa `/fh/extend/initialize.sh`.

O `telnetd -p 26` presente no arquivo está comentado e não é executado por `rcS`.

## 2. `initialize.sh`

### Preparação

1. Define variáveis internas como `APP_TYPE=RGW` e `IS_GPON=1`. `KERNELVER=3.4.11-rt19` é apenas uma variável não usada; o kernel real é 2.6.34.10.
2. Complementa `PATH` e `LD_LIBRARY_PATH` para bibliotecas e binários de `/fh/extend`.
3. Mostra `Press Ctrl + C to stop auto setup` e aguarda cerca de dois segundos. Não há tratador de sinal: interromper nessa fase encerra o script.
4. Pede ao kernel para descartar page cache (`echo 3 > /proc/sys/vm/drop_caches`).

### Scripts sourced e estado persistente

5. **`mk_cfg_dir.sh`** — cria diretórios ausentes em `/fhcfg`: `fh_wifi`, `fh_pon`, `cpepatch`, `l3_def`, `ppp` e `extend/dhcpforwan`; cria ainda `/dev/shm/fh_ver_tmp`. Os `rm -rf` internos só são alcançados se o respectivo diretório já estiver ausente, portanto não removem uma árvore existente no caminho normal.
6. **`cp_cfg.sh`** — semeia arquivos padrão em `/fhcfg` somente se eles não existirem, para PPP, PPPoE, VoIP, DHCP e `udhcpd.conf`. Exceção importante: se `/fh/extend/omci.conf` existir, ele é copiado para `/fhcfg/omci.conf` incondicionalmente em todo boot.
7. Lê `hwcfg` de `/proc/driver/fh_hw_cfg`, garante `fh_pon/pon_type` (cria `gpon` se ausente) e o lê em `PONTYPE`.

### Board, saída e devices

8. **`fhdrv_kdrv_board_impl_load.sh`** chama `fhdrv_kdrv_board_load.sh`, carrega `fhdrv_kdrv_board.ko` caso ausente, recria `/dev/fhdrv_kdrv_board` com o major obtido de `/proc/devices`, carrega `fhdrv_kdrv_board_impl.ko` e executa o binário `fhdrv_kdrv_usb_led`.
9. **`fh_printf_redirect.sh`** remove `/fhcfg/ptys_name`, inicia `fh_printf_redirect`, espera um segundo, lê os PTYs gerados e, se válidos, redireciona stdout/stderr do shell atual aos PTYs. Ele não muda stdin; o console serial permanece o mesmo para entrada.
10. Carrega `iomsg_drv.ko`, cria `/dev/fh_omci` e `/dev/iomsg`, carrega `i2cdev.ko` e inicia em background `fh_bsp_led_act` e `detectHwEvent start`.
11. Em placas `hwcfg` `0x30` ou `0x31`, executa um comando `cli` para transformar portas. O `0x24` desta ONU não entra nesse ramo.

### PON e rede Ethernet

12. Executa `hi_xpon_app gpon 2 4095` e, após um segundo, carrega `hi_kploam.ko`. Isso inicializa a pilha XPON antes da LAN.
13. Executa o binário `net_dev_created`. Suas strings confirmam que ele cria os roots CFE para `wan` e `eth0`–`eth3`, cria `br0`, adiciona as Ethernet à bridge, atribui MACs e cria interfaces auxiliares PON/VoIP. Esta é a etapa que torna a Ethernet utilizável.
14. Configura `br0` como `192.168.1.1` e sobe `lo`. O runtime coletado posteriormente mostra `br0` com `192.168.0.1`; logo alguma aplicação posterior ou configuração persistente a alterou depois desta linha.

### Limpeza e estado de atualização

15. Se existir `/fhcfg/extend/delete_flag_after_update`, remove `use_edit_boot`, limpa arquivos de DHCP em `/fhcfg/extend/dhcpforwan/` e remove o flag.
16. Se `/fh/extend/boot_version_control` existir e `use_edit_boot` estiver ausente, copia-o para `/fhcfg/cpepatch/boot_version_control`.
17. Ajusta `nf_conntrack_max` para 20.000 e cria `/var/run`.

### Serviços de aplicação

18. Executa `rm_init.exe`; inicia em background `l3mng`, `dhcpl2` e `onu_igmpv3`.
19. Lê `/fhcfg/voip.conf` e inicia em background um entre `sip`, `h248` ou `voip_preStart`.
20. **`fh_ver_export.sh`** recria `/dev/shm/fh_ver_tmp`, remove `/dev/shm/fh_version_ko` e coleta parâmetros `fh_moduler_version` de módulos carregados para esse arquivo temporário.
21. Executa `dmz-init`, inicia `pppoeManage` em background e aguarda dois segundos.
22. Se `PONTYPE=epon`, chama `epon_oam` em primeiro plano; em GPON (caso desta unidade), inicia `load_omci` em background.

### Gerência, WebUI, UPnP e CLI

23. **`tr069/bin/runTr069`** prepara permissões de scripts e do agente, cria `/fhcfg/tr069/fhcfg` se necessário e inicia `agent -F /fhcfg/ -L 0 -M 1 -S 600` em background. Os scripts `runtime_default_fhcfg_fetch.sh` e `tr069_config_version_check.sh` existem, mas `runTr069` não os chama diretamente; o agente pode chamá-los, porém isto não é confirmável apenas por análise estática.
24. **`runWeb.sh`** cria `/fhcfg/web_log` se necessário, torna `webs` executável e inicia `webs -L 3 -M 1 -S 100 -m all` em background. É a WebUI.
25. **`upnp/upnp-init`** roda em background e chama `upnp-run`. `upnp-run` cria `/etc/linuxigd`, copia XMLs padrão caso ausentes e cria links de bibliotecas UPnP em `/lib` e do arquivo de configuração em `/etc/upnpd.conf`. Ele não inicia `upnpd`; as linhas correspondentes estão comentadas.
26. Inicia `ntpclient` e `watchdog` em background.
27. Executa `load_cli` em primeiro plano. Ele carrega a CLI FiberHome e o prompt próprio de login; é o bloqueio normal que impede o retorno de `initialize.sh` a `rcS`.

## Scripts existentes, mas não diretamente acionados por `rcS`

- `tr069_config_version_check.sh`: sincroniza `param.xml`, `cpe_mib.xml` e versões entre a área runtime TR-069 e `/fhcfg`; pode sobrescrever arquivos persistentes quando chamado pelo agente.
- `runtime_default_fhcfg_fetch.sh`: recebe nome do dispositivo e ISP, e copia configurações padrão do warehouse em `/fh/extend/tr069/fhcfg/...` para `/fhcfg/tr069/fhcfg`.
- `upnp/upnp-stop`: mata `upnpd` usando PID de `/fhcfg/upnpid`; não é chamado no boot normal.

## Consequências para o Linux mínimo

- Para obter shell root com rede, não interrompa `initialize.sh` antes de `net_dev_created`.
- O patch atual que desativa somente `runWeb.sh` e `load_cli` mantém os passos anteriores, incluindo PON, TR-069, VoIP e UPnP.
- Remover GPON/EPON exige um patch posterior e isolado: `hi_xpon_app`, `hi_kploam`, módulos PON e `load_omci` têm dependências não eliminadas por desativar a WebUI.
- A primeira fonte de alterações persistentes a evitar em um perfil mínimo é `cp_cfg.sh`/TR-069, pois escrevem em `/fhcfg`.
