#!/bin/sh
set -eu

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
as=arm-none-eabi-as
ld=arm-none-eabi-ld

"$as" -march=armv7-a -o "$dir/hiusb-start.o" "$dir/hiusb-start.S"
"$ld" -r -o "$dir/hiusb-start.ko" "$dir/hiusb-start.o"

"$as" -march=armv7-a -o "$dir/hiusb-ehci.o" "$dir/hiusb-ehci.S"
"$ld" -r -o "$dir/hiusb-ehci.ko" "$dir/hiusb-ehci.o"

arm-none-eabi-readelf -h "$dir/hiusb-start.ko"
arm-none-eabi-readelf -s "$dir/hiusb-start.ko"
arm-none-eabi-readelf -r "$dir/hiusb-start.ko"
arm-none-eabi-readelf -r "$dir/hiusb-ehci.ko"
