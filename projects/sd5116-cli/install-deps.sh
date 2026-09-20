#!/usr/bin/env bash
# Instala dependências comuns do host para os exemplos SD5116.
# Destinado a Debian/Ubuntu; não executa nada na ONU.

set -eu

if ! command -v apt-get >/dev/null 2>&1; then
    echo "Erro: este instalador suporta somente sistemas Debian/Ubuntu (apt-get)." >&2
    echo "Instale manualmente make, file e qemu-user." >&2
    exit 1
fi

if [ "$(id -u)" -eq 0 ]; then
    SUDO=""
elif command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
else
    echo "Erro: execute como root ou instale/configure sudo." >&2
    exit 1
fi

echo "Instalando make, file e qemu-user..."
$SUDO apt-get update
$SUDO apt-get install -y \
    make \
    file \
    qemu-user

echo
echo "Pronto. Valide com:"
echo "  make host-check"
echo "  make"
echo "  file bin/onu-led"
echo
echo "A toolchain uClibc-ng deve existir em ../../toolchains/."
echo "O binário é ARM EABI estático: copie-o para /tmp na ONU para teste."
