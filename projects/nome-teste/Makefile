TOOLCHAIN_DIR ?= /home/elton/fiberhome-dump/toolchains/armv5-eabi--uclibc--stable-2025.08-1
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
HOST_CC ?= cc
CPPFLAGS += -Ilib
KERNEL_HEADERS_DIR ?= /home/elton/fiberhome-dump/toolchains/armv5-eabi--uclibc--stable-2025.08-1/arm-buildroot-linux-uclibcgnueabi/sysroot/usr/include
ifneq (,$(wildcard $(KERNEL_HEADERS_DIR)))
CPPFLAGS += -I$(KERNEL_HEADERS_DIR)
endif
CFLAGS += -Os -Wall -Wextra -Werror -std=c99 -ffunction-sections -fdata-sections
LDFLAGS += -static -Wl,--gc-sections -s
LIBS += build/libhello.a

.PHONY: all host-check clean
all: build/app
build/app: build/main.o build/libhello.a
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ build/main.o $(LIBS) $(LDFLAGS)
build/main.o: src/main.c lib/hello/hello.h | build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
build/libhello.a: build/hello.o | build
	$(CROSS_COMPILE)ar rcs $@ $<
build/hello.o: lib/hello/hello.c lib/hello/hello.h | build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
host-check:
	$(HOST_CC) -std=c99 -Wall -Wextra -Werror -Ilib -fsyntax-only src/main.c lib/hello/hello.c
build:
	mkdir -p $@
clean:
	rm -rf build
