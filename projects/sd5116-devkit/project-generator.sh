#!/bin/sh
set -eu

usage() { echo "uso: $0 NOME_DO_PROJETO" >&2; exit 2; }
[ "$#" -eq 1 ] || usage
toolchain_dir=${SD5116_TOOLCHAIN_DIR:-/home/elton/fiberhome-dump/toolchains/armv5-eabi--uclibc--stable-2025.08-1}
kernel_headers_dir=${SD5116_KERNEL_HEADERS:-$toolchain_dir/arm-buildroot-linux-uclibcgnueabi/sysroot/usr/include}
target_address=${SD5116_TARGET:-192.168.1.245}
escape_sed() { printf '%s' "$1" | sed 's/[&|]/\\&/g'; }
toolchain_sed=$(escape_sed "$toolchain_dir")
kernel_headers_sed=$(escape_sed "$kernel_headers_dir")
target_sed=$(escape_sed "$target_address")
name=$1
case "$name" in
    ''|*[!A-Za-z0-9_.-]*) echo "nome invalido: use letras, numeros, ., _ ou -" >&2; exit 2 ;;
esac
[ ! -e "$name" ] || { echo "ja existe: $name" >&2; exit 1; }

mkdir -p "$name/src" "$name/lib/hello" "$name/build"
mkdir -p "$name/.vscode"
cat > "$name/src/main.c" <<EOF
#include <stdio.h>
#include "hello/hello.h"

int main(void)
{
    hello_print("$name");
    return 0;
}
EOF
cat > "$name/lib/hello/hello.h" <<'EOF'
#ifndef HELLO_H
#define HELLO_H
void hello_print(const char *project);
#endif
EOF
cat > "$name/lib/hello/hello.c" <<'EOF'
#include <stdio.h>
#include "hello.h"

void hello_print(const char *project)
{
    printf("hello from %s\\n", project);
}
EOF
cat > "$name/Makefile" <<'EOF'
TOOLCHAIN_DIR ?= @TOOLCHAIN_DIR@
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-buildroot-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
HOST_CC ?= cc
CPPFLAGS += -Ilib
KERNEL_HEADERS_DIR ?= @KERNEL_HEADERS_DIR@
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
EOF
cat > "$name/.vscode/tasks.json" <<'EOF'
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "compilar ARM/uClibc",
            "type": "shell",
            "command": "make",
            "options": { "cwd": "${workspaceFolder}" },
            "group": { "kind": "build", "isDefault": true },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "verificar no host",
            "type": "shell",
            "command": "make host-check",
            "options": { "cwd": "${workspaceFolder}" },
            "problemMatcher": ["$gcc"]
        }
    ]
}
EOF
cat > "$name/.vscode/launch.json" <<'EOF'
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Depurar na ONU via gdbserver",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/app",
            "cwd": "${workspaceFolder}",
            "MIMode": "gdb",
            "miDebuggerPath": "@TOOLCHAIN_DIR@/bin/arm-buildroot-linux-uclibcgnueabi-gdb",
            "miDebuggerServerAddress": "@TARGET_ADDRESS@:2345",
            "externalConsole": false,
            "stopAtEntry": false
        }
    ]
}
EOF
cat > "$name/.vscode/c_cpp_properties.json" <<'EOF'
{
    "version": 4,
    "configurations": [
        {
            "name": "SD5116 ARM/uClibc",
            "compilerPath": "@TOOLCHAIN_DIR@/bin/arm-buildroot-linux-uclibcgnueabi-gcc",
            "compilerArgs": ["--sysroot=@TOOLCHAIN_DIR@/arm-buildroot-linux-uclibcgnueabi/sysroot"],
            "intelliSenseMode": "gcc-arm",
            "cStandard": "c99",
            "cppStandard": "c++11",
            "includePath": [
                "${workspaceFolder}/lib",
                "@TOOLCHAIN_DIR@/arm-buildroot-linux-uclibcgnueabi/sysroot/usr/include",
                "@KERNEL_HEADERS_DIR@"
            ],
            "defines": ["__arm__", "__ARMEL__"]
        }
    ]
}
EOF
sed -i \
    -e "s|@TOOLCHAIN_DIR@|$toolchain_sed|g" \
    -e "s|@KERNEL_HEADERS_DIR@|$kernel_headers_sed|g" \
    -e "s|@TARGET_ADDRESS@|$target_sed|g" \
    "$name/Makefile" "$name/.vscode/launch.json" "$name/.vscode/c_cpp_properties.json"
cat > "$name/README.md" <<EOF
# $name

Aplicação gerada para a ONU SD5116. Compile com make usando ARM/uClibc;
use make host-check para verificar a sintaxe no PC. A biblioteca inicial
fica em lib/hello; acrescente outras bibliotecas no LIBS do Makefile. A pasta
.vscode já contém tarefas de build e configurações de IntelliSense/debug.
EOF
echo "projeto criado: $name"
