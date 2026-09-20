#!/bin/sh
# Pisca PON, LOS, VoIP, LAN1 e LAN2 em sequência.
# Compatível com BusyBox ash 1.18.

LEDCTL=${LEDCTL:-/tmp/onu-led-direct}
DELAY=${DELAY:-0.2}
LEDS="pon los voip lan1 lan2"

if [ ! -x "$LEDCTL" ]; then
    echo "Erro: nao encontrei executavel: $LEDCTL" >&2
    exit 1
fi

all_off()
{
    for led in $LEDS; do
        "$LEDCTL" led "$led" off >/dev/null 2>&1
    done
}

cleanup()
{
    all_off
}

trap cleanup 0
trap 'exit 0' INT TERM HUP

if [ "$1" = "--loop" ]; then
    LOOP=1
else
    LOOP=0
fi

all_off

while :; do
    for led in $LEDS; do
        "$LEDCTL" led "$led" on || exit 1
        sleep "$DELAY"
        "$LEDCTL" led "$led" off || exit 1
    done

    [ "$LOOP" -eq 1 ] || break
done

cleanup
