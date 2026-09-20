TOOLCHAIN_DIR ?= ../../toolchains/armv5-eabi--uclibc--stable-2025.08-1
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
HOST_CC ?= gcc
TARGET_LIBDIR := ../../extracted/kernel_rootfsB/lib

CPPFLAGS += -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200809L
CFLAGS += -Os -ffunction-sections -fdata-sections -Wall -Wextra -Werror -std=c99
LDFLAGS += -Wl,--gc-sections,--allow-shlib-undefined -L$(TARGET_LIBDIR) -Wl,-l:libdl.so.0

.PHONY: all clean host-check
all: bin/onu-serial-oled

bin/onu-serial-oled: src/onu_serial_oled.c | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDFLAGS)

bin:
	mkdir -p $@

host-check:
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only src/onu_serial_oled.c

clean:
	rm -rf bin
