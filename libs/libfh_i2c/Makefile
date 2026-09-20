TOOLCHAIN_DIR ?= ../../toolchains/armv5-eabi--uclibc--stable-2025.08-1
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
CFLAGS += -Os -Wall -Wextra -Werror -std=c99 -Iinclude
all: lib/libfh_i2c.a
lib/libfh_i2c.a: src/fh_i2c.c | lib
	$(CC) $(CFLAGS) -c -o lib/fh_i2c.o $<
	$(AR) rcs $@ lib/fh_i2c.o
lib:
	mkdir -p lib
clean:
	rm -rf lib
