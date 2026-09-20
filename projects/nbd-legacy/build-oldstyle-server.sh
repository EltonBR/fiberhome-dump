#!/bin/sh
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
TOOLCHAIN_DIR=${TOOLCHAIN_DIR:-"$ROOT/toolchains/armv5-eabi--uclibc--stable-2025.08-1"}
CROSS=${CROSS:-"$TOOLCHAIN_DIR/bin/arm-buildroot-linux-uclibcgnueabi-"}
mkdir -p "$SCRIPT_DIR/bin/arm" "$SCRIPT_DIR/bin/host"
"${CROSS}gcc" -Os -static -Wall -Wextra -o "$SCRIPT_DIR/bin/arm/nbd-oldstyle-server" "$SCRIPT_DIR/src/nbd-oldstyle-server.c"
gcc -Os -Wall -Wextra -o "$SCRIPT_DIR/bin/host/nbd-oldstyle-server" "$SCRIPT_DIR/src/nbd-oldstyle-server.c"
"${CROSS}strip" --strip-unneeded "$SCRIPT_DIR/bin/arm/nbd-oldstyle-server"
