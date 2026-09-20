# Benchmark reproduzível para a FiberHome SD5116.

TOOLCHAIN_DIR ?= ../../toolchains/armv5-eabi--uclibc--stable-2025.08-1
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
HOST_CC ?= gcc

CPPFLAGS += -D_POSIX_C_SOURCE=200809L
CFLAGS += -Os -ffunction-sections -fdata-sections -Wall -Wextra -Werror -std=c99
LDFLAGS += -static -Wl,--gc-sections -s

TARGET := bin/onu-benchmark
SOURCE := src/onu_benchmark.c

.PHONY: all clean host-check

all: $(TARGET)

$(TARGET): $(SOURCE) | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

bin:
	mkdir -p $@

host-check:
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only $(SOURCE)

clean:
	rm -rf bin
