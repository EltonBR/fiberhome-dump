TOOLCHAIN_DIR ?= ../../toolchains/armv5-eabi--uclibc--stable-2025.08-1
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
HOST_CC ?= gcc
GPIO_DIR := ../../libs/libfh_gpio
SPI_DIR := ../../libs/libfh_gpio_spi
I2C_DIR := ../../libs/libfh_i2c
TARGET_LIBDIR := ../../extracted/kernel_rootfsB/lib
CPPFLAGS += -D_POSIX_C_SOURCE=200809L -I$(GPIO_DIR)/include -I$(SPI_DIR)/include -I$(I2C_DIR)/include
CFLAGS += -Os -ffunction-sections -fdata-sections -Wall -Wextra -Werror -std=c99
STATIC_LDFLAGS := -static -Wl,--gc-sections -s
DYNAMIC_LDFLAGS := -Wl,--gc-sections,--allow-shlib-undefined -L$(TARGET_LIBDIR) -Wl,-l:libdl.so.0

.PHONY: all clean host-check libs
all: libs bin/onu-gpio-test bin/onu-spi-test bin/onu-i2c-test
libs:
	$(MAKE) -C $(GPIO_DIR)
	$(MAKE) -C $(SPI_DIR)
	$(MAKE) -C $(I2C_DIR)
bin/onu-gpio-test: src/onu_gpio_test.c $(GPIO_DIR)/lib/libfh_gpio.a | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(GPIO_DIR)/lib/libfh_gpio.a $(STATIC_LDFLAGS)
bin/onu-spi-test: src/onu_spi_test.c $(SPI_DIR)/lib/libfh_gpio_spi.a $(GPIO_DIR)/lib/libfh_gpio.a | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(SPI_DIR)/lib/libfh_gpio_spi.a $(GPIO_DIR)/lib/libfh_gpio.a $(STATIC_LDFLAGS)
bin/onu-i2c-test: src/onu_i2c_test.c $(I2C_DIR)/lib/libfh_i2c.a | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(I2C_DIR)/lib/libfh_i2c.a $(DYNAMIC_LDFLAGS)
bin:
	mkdir -p $@
host-check:
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only src/onu_gpio_test.c src/onu_spi_test.c src/onu_i2c_test.c
clean:
	rm -rf bin
