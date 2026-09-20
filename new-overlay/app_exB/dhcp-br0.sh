#!/bin/sh
# Cliente DHCP IPv4 independente para a bridge LAN br0.
#
# Uso:
#   dhcp-br0.sh start [servidor-ntp]
#   dhcp-br0.sh stop
#   dhcp-br0.sh renew
#   dhcp-br0.sh status
#
# "start" substitui o endereco/rota IPv4 estatica atual de br0 quando
# receber uma concessao. O NTP e opcional: se informado, usa o applet
# BusyBox ntpd para ajustar o relogio uma vez apos obter a concessao.

set -u

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/fh/bin:/fh/extend
export PATH

IFACE=br0
PIDFILE=/var/run/udhcpc-br0.pid
EVENT_SCRIPT=/fh/extend/udhcpc-br0.script
NTP_FILE=/var/run/udhcpc-br0.ntp-server
RESOLV_CONF=/etc/resolv.conf

log()
{
    printf '%s\n' "$*"
}

have_live_pid()
{
    [ -r "$PIDFILE" ] || return 1
    read pid < "$PIDFILE" || return 1
    [ -n "$pid" ] || return 1
    kill -0 "$pid" 2>/dev/null
}

start_client()
{
    ntp_server=${1:-}

    if [ ! -x "$EVENT_SCRIPT" ]; then
        log "Erro: script DHCP ausente ou sem permissao: $EVENT_SCRIPT"
        return 1
    fi

    if [ -n "$ntp_server" ]; then
        printf '%s\n' "$ntp_server" > "$NTP_FILE"
    else
        rm -f "$NTP_FILE"
    fi

    if have_live_pid; then
        log "udhcpc ja esta em execucao (PID $(cat "$PIDFILE"))"
        return 0
    fi
    rm -f "$PIDFILE"

    ifconfig "$IFACE" up || return 1
    log "Solicitando IPv4 por DHCP em $IFACE..."
    udhcpc -i "$IFACE" -s "$EVENT_SCRIPT" -p "$PIDFILE" -b
}

stop_client()
{
    if have_live_pid; then
        read pid < "$PIDFILE"
        kill "$pid" || return 1
        log "udhcpc interrompido (PID $pid)"
    else
        log "udhcpc nao esta em execucao"
    fi
    rm -f "$PIDFILE" "$NTP_FILE"
}

renew_client()
{
    if ! have_live_pid; then
        log "udhcpc nao esta em execucao"
        return 1
    fi
    read pid < "$PIDFILE"
    kill -USR1 "$pid"
    log "Renovacao DHCP solicitada"
}

status_client()
{
    if have_live_pid; then
        log "udhcpc ativo (PID $(cat "$PIDFILE"))"
    else
        log "udhcpc inativo"
    fi
    ifconfig "$IFACE" 2>/dev/null || true
    route -n 2>/dev/null || true
    if [ -r "$RESOLV_CONF" ]; then
        log "DNS em $RESOLV_CONF:"
        sed -n '1,20p' "$RESOLV_CONF"
    fi
}

case "${1:-}" in
    start)
        start_client "${2:-}"
        ;;
    stop)
        stop_client
        ;;
    renew)
        renew_client
        ;;
    status)
        status_client
        ;;
    *)
        log "Uso: $0 {start [servidor-ntp]|stop|renew|status}"
        exit 2
        ;;
esac
