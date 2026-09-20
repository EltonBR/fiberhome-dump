#!/bin/sh
# Executar NA ONU, não no PC. Compatível com BusyBox /bin/sh.
# Move somente arquivos de /fh/extend para uma quarentena na mesma partição.
set -u

apply=0
if [ "${1:-}" = "--apply" ]; then
    apply=1
    shift
fi
if [ "$#" -ne 0 ]; then
    echo "Uso: $0 [--apply]" >&2
    exit 2
fi

SOURCE=/fh/extend
DESTINATION=$SOURCE/remove-candidates

if [ ! -d "$SOURCE" ] || [ ! -f "$SOURCE/initialize.sh" ]; then
    echo "Recusado: este script deve rodar na ONU, com /fh/extend montado." >&2
    exit 1
fi

move_one() {
    relative=$1
    source=$SOURCE/$relative
    target=$DESTINATION/$relative

    if [ ! -e "$source" ]; then
        echo "AUSENTE  $relative"
        return 0
    fi
    if [ -e "$target" ]; then
        echo "PULADO   $relative (ja existe na quarentena)"
        return 0
    fi
    if [ "$apply" -eq 0 ]; then
        echo "MOVERIA  $relative"
        return 0
    fi
    mkdir -p "$(dirname "$target")" || return 1
    mv "$source" "$target" || return 1
    echo "MOVIDO    $relative"
}

echo "Fonte:       $SOURCE"
echo "Quarentena:  $DESTINATION"
if [ "$apply" -eq 0 ]; then
    echo "Modo seco: nada sera movido. Use --apply depois de revisar."
fi

# Não são iniciados pelo new-overlay atual. Todos ficam em /fh/extend.
for item in \
    fh_printf_redirect \
    fh_printf_redirect.sh \
    fh_bsp_led_act \
    detectHwEvent \
    rm_init.exe \
    l3mng \
    dhcpl2 \
    onu_igmpv3 \
    sip \
    h248 \
    voip_preStart \
    dmz-init \
    pppoeManage \
    load_omci \
    tr069/bin/runTr069 \
    runWeb.sh \
    webs \
    web \
    upnp \
    ntpclient \
    watchdog \
    load_cli
do
    move_one "$item" || exit 1
done

# Bibliotecas de /fh/extend exclusivas dos serviços acima no new-overlay atual.
# HAL, bibliotecas de placa e uClibc não estão nesta lista.
for item in \
    libcli_adaptation_layer.so \
    libcli_cli.so \
    libcliom.so \
    libcm.so \
    libddns.so \
    libdhcpL2Cmd.so \
    libdhcpL2OmciCfg.so \
    libdhcpcoptionctl.so \
    libdhcpctl.so \
    libdmz.so \
    libfh_hi_voipadp.so \
    libgl3_advance.so \
    libgl3_cli.so \
    libgl3_pppoe.so \
    libgpon_l2.so \
    libgpon_l3.so \
    libgpon_l3_api.so \
    libgpon_rm.so \
    libh248Call.so \
    libigmp.so \
    libigmpcfg.so \
    libigmpcli.so \
    libio_msg.so \
    libipi.so \
    libipv6.so \
    libl3ctl.so \
    libled_interface.so \
    libmc_hal.so \
    libmld.so \
    libmxml.so \
    libnatcli.so \
    libnetdev_cli.so \
    libnomci.so \
    libntp_cli.so \
    libntp_interface.so \
    libomci_cli.so \
    libpal.so \
    libping.so \
    libpppoe.so \
    libprivate_h248.so \
    libprivate_sip.so \
    libsipTK.so \
    libsip_oneip.so \
    libtr069MsgClient.so \
    libtr069cli.so \
    libtr104.so \
    libuapi.so \
    libudhcpWeb.so \
    libudhcpcctl.so \
    libudhcpdctl.so \
    libvoice_cli.so
do
    move_one "$item" || exit 1
done

if [ "$apply" -eq 0 ]; then
    echo "Revise a lista; execute $0 --apply para efetivar."
else
    echo "Concluido. Os arquivos permanecem em $DESTINATION e podem ser restaurados com mv."
fi
