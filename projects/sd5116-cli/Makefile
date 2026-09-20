# Base de aplicações CLI para a FiberHome SD5116.
#
# Por padrão, usa a toolchain ARMv5/EABI/uClibc-ng estática do repositório.
# Ela produz código compatível com o Cortex-A9 sem depender do userspace
# proprietário. Para outra toolchain: make CROSS_COMPILE=/caminho/prefixo-
#
# Verificação de sintaxe no PC (não gera binário utilizável na ONU):
#   make host-check

TOOLCHAIN_DIR ?= ../../toolchains/armv5-eabi--uclibc--stable-2025.08-1
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
HOST_CC ?= gcc

CPPFLAGS += -D_POSIX_C_SOURCE=200809L
CFLAGS += -Os -ffunction-sections -fdata-sections -Wall -Wextra -Werror -std=c99
LDFLAGS += -static -Wl,--gc-sections -s

TARGETS := bin/onu-led bin/onu-led-direct bin/onu-i2c

.PHONY: all clean host-check

all: $(TARGETS)

bin/onu-led: src/onu_led.c | bin

	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

bin/onu-led-direct: src/onu_led_direct.c | bin

	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

bin/onu-i2c: src/onu_i2c.c | bin

	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

bin:

	mkdir -p $@

host-check:

	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only src/onu_led.c
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only src/onu_led_direct.c
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only src/onu_i2c.c

clean:

	rm -rf bin
