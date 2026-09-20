#!/bin/sh
# Compila somente o nbd-server 3.9.1 para o PC anfitriao.
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
BUILD="$ROOT/build/nbd-legacy-host"
OUT="$SCRIPT_DIR/bin/host"
SOURCE="$ROOT/third_party/nbd-nbd-3.9.1"

test -d "$SOURCE" || {
    echo "fonte ausente: $SOURCE" >&2
    exit 1
}

rm -rf "$BUILD"
mkdir -p "$BUILD" "$OUT"
cp -a "$SOURCE" "$BUILD/src"
patch -d "$BUILD/src" -p1 < "$SCRIPT_DIR/patches/002-cliente-leitura-tcp-completa.patch"
for page in nbd-server.1 nbd-server.5 nbd-client.8 nbd-trdump.1; do
    cp "$BUILD/src/man/sh.tmpl" "$BUILD/src/man/$page.sh.in"
done
(
    cd "$BUILD/src"
    autoreconf -f -i
)
mkdir "$BUILD/out"
(
    cd "$BUILD/out"
    ../src/configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
    make -j"$(getconf _NPROCESSORS_ONLN)"
    cp nbd-server "$OUT/"
)

file "$OUT/nbd-server"
sha256sum "$OUT/nbd-server"
echo "pronto: $OUT"
