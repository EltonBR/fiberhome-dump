TOOLCHAIN_DIR ?= ../../toolchains/armv5-eabi--uclibc--stable-2025.08-1
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
CFLAGS += -D_POSIX_C_SOURCE=200809L -Os -Wall -Wextra -Werror -std=c99 -Iinclude -I../libfh_gpio/include
all: lib/libfh_gpio_spi.a
lib/libfh_gpio_spi.a: src/fh_gpio_spi.c | lib
	$(CC) $(CFLAGS) -c -o lib/fh_gpio_spi.o $<
	$(AR) rcs $@ lib/fh_gpio_spi.o
lib:
	mkdir -p lib
clean:
	rm -rf lib
