TOOLCHAIN_DIR ?= ../../toolchains/armv5-eabi--uclibc--stable-2025.08-1
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
HOST_CC ?= gcc

I2C_DIR := ../../libs/libfh_i2c
LCD_DIR := ../../libs/libfh_i2c_lcd
TARGET_LIBDIR := ../../extracted/kernel_rootfsB/lib
CPPFLAGS += -I$(I2C_DIR)/include
CFLAGS += -Os -ffunction-sections -fdata-sections -Wall -Wextra -Werror -std=c99
LDFLAGS += -Wl,--gc-sections,--allow-shlib-undefined -L$(TARGET_LIBDIR) -Wl,-l:libdl.so.0

.PHONY: all clean host-check

all: bin/onu-i2c-ssd1306-read bin/onu-i2c-oled-speed-test bin/onu-i2c-oled-recover bin/onu-lcd1602-pcf8574 bin/onu-i2c-scan bin/onu-i2c-oled-animate bin/onu-lcd1602-map-test

bin/onu-i2c-ssd1306-read: src/onu_i2c_ssd1306_read.c $(I2C_DIR)/lib/libfh_i2c.a | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(I2C_DIR)/lib/libfh_i2c.a $(LDFLAGS)

bin/onu-i2c-oled-speed-test: src/onu_i2c_oled_speed_test.c $(I2C_DIR)/lib/libfh_i2c.a $(LCD_DIR)/lib/libfh_i2c_lcd.a | bin
	$(CC) $(CPPFLAGS) -I$(LCD_DIR)/include $(CFLAGS) -o $@ $< $(LCD_DIR)/lib/libfh_i2c_lcd.a $(I2C_DIR)/lib/libfh_i2c.a $(LDFLAGS)

bin/onu-i2c-oled-recover: src/onu_i2c_oled_recover.c $(I2C_DIR)/lib/libfh_i2c.a $(LCD_DIR)/lib/libfh_i2c_lcd.a | bin
	$(CC) $(CPPFLAGS) -I$(LCD_DIR)/include $(CFLAGS) -o $@ $< $(LCD_DIR)/lib/libfh_i2c_lcd.a $(I2C_DIR)/lib/libfh_i2c.a $(LDFLAGS)

bin/onu-lcd1602-pcf8574: src/onu_lcd1602_pcf8574.c $(I2C_DIR)/lib/libfh_i2c.a | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(I2C_DIR)/lib/libfh_i2c.a $(LDFLAGS)

bin/onu-i2c-scan: src/onu_i2c_scan.c $(I2C_DIR)/lib/libfh_i2c.a | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(I2C_DIR)/lib/libfh_i2c.a $(LDFLAGS)

bin/onu-i2c-oled-animate: src/onu_i2c_oled_animate.c $(I2C_DIR)/lib/libfh_i2c.a $(LCD_DIR)/lib/libfh_i2c_lcd.a | bin
	$(CC) $(CPPFLAGS) -I$(LCD_DIR)/include $(CFLAGS) -o $@ $< $(LCD_DIR)/lib/libfh_i2c_lcd.a $(I2C_DIR)/lib/libfh_i2c.a $(LDFLAGS)

bin/onu-lcd1602-map-test: src/onu_lcd1602_map_test.c $(I2C_DIR)/lib/libfh_i2c.a | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(I2C_DIR)/lib/libfh_i2c.a $(LDFLAGS)

bin:
	mkdir -p $@

host-check:
	$(HOST_CC) $(CPPFLAGS) -I$(LCD_DIR)/include $(CFLAGS) -fsyntax-only src/onu_i2c_ssd1306_read.c src/onu_i2c_oled_speed_test.c src/onu_i2c_oled_recover.c src/onu_lcd1602_pcf8574.c src/onu_i2c_scan.c src/onu_i2c_oled_animate.c src/onu_lcd1602_map_test.c

clean:
	rm -rf bin
