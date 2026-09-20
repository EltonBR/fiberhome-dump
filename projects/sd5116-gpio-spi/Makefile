# SPI por bit-banging sobre GPIO na FiberHome SD5116.
TOOLCHAIN_DIR ?= ../../toolchains/armv5-eabi--uclibc--stable-2025.08-1
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
HOST_CC ?= gcc

CPPFLAGS += -D_POSIX_C_SOURCE=200809L
CFLAGS += -Os -ffunction-sections -fdata-sections -Wall -Wextra -Werror -std=c99
LDFLAGS += -static -Wl,--gc-sections -s

TARGET := bin/onu-gpio-spi

.PHONY: all clean host-check

all: $(TARGET)

$(TARGET): src/onu_gpio_spi.c | bin

	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

bin:

	mkdir -p $@

host-check:

	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only src/onu_gpio_spi.c

clean:

	rm -rf bin
