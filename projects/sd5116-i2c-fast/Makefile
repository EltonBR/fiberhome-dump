# Teste temporário de I2C Fast-mode para a FiberHome SD5116.
# Este binário é dinâmico de propósito: chama as bibliotecas HAL existentes
# na imagem, sem copiar nem substituir qualquer biblioteca proprietária.

TOOLCHAIN_DIR ?= ../../toolchains/armv5-eabi--uclibc--stable-2025.08-1
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
HOST_CC ?= gcc

CPPFLAGS += -D_POSIX_C_SOURCE=200809L
CFLAGS += -Os -ffunction-sections -fdata-sections -Wall -Wextra -Werror -std=c99
TARGET_LIBDIR := ../../extracted/kernel_rootfsB/lib
LDFLAGS += -Wl,--gc-sections,--allow-shlib-undefined -L$(TARGET_LIBDIR) -Wl,-l:libdl.so.0

.PHONY: all clean host-check

all: bin/onu-i2c-fast

bin/onu-i2c-fast: src/onu_i2c_fast.c | bin

	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDFLAGS)

bin:

	mkdir -p $@

host-check:

	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only src/onu_i2c_fast.c

clean:

	rm -rf bin
