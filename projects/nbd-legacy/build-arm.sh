#!/bin/sh
# Compila nbd-server 3.9.1 para a ONU SD5116 (ARM EABI/uClibc).
# Execute a partir de qualquer diretorio: sh projects/nbd-legacy/build-arm.sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
BUILD="$ROOT/build/nbd-legacy-arm"
# Staging privado deste projeto; nao apaga um sysroot usado por outro projeto.
STAGE="$BUILD/stage"
OUT="$SCRIPT_DIR/bin/arm"
GLIB_CACHE="$SCRIPT_DIR/config/glib-2.26.1-arm.cache"
TOOLCHAIN_DIR=${TOOLCHAIN_DIR:-"$ROOT/toolchains/armv5-eabi--uclibc--stable-2025.08-1"}
CROSS=${CROSS:-"$TOOLCHAIN_DIR/bin/arm-buildroot-linux-uclibcgnueabi-"}

for source in zlib-1.2.13 glib-2.26.1 nbd-nbd-3.9.1; do
    test -d "$ROOT/third_party/$source" || {
        echo "fonte ausente: third_party/$source" >&2
        exit 1
    }
done
test -x "${CROSS}gcc" || {
    echo "compilador cruzado ausente: ${CROSS}gcc" >&2
    exit 1
}
test -r "$GLIB_CACHE" || {
    echo "cache de configuracao GLib ausente: $GLIB_CACHE" >&2
    exit 1
}

# BUILD e seu STAGE interno contem somente produtos descartaveis deste processo.
rm -rf "$BUILD"
mkdir -p "$BUILD" "$STAGE" "$OUT"

echo '[1/4] zlib estatica para ARM'
cp -a "$ROOT/third_party/zlib-1.2.13" "$BUILD/zlib-src"
(
    cd "$BUILD/zlib-src"
    CC="${CROSS}gcc" AR="${CROSS}ar" RANLIB="${CROSS}ranlib" \
        ./configure --static --prefix="$STAGE/usr"
    make -j"$(getconf _NPROCESSORS_ONLN)"
    make install
)

echo '[2/4] GLib 2.26.1 estatica para ARM'
mkdir "$BUILD/glib"
(
    cd "$BUILD/glib"
    CONFIG_SITE="$GLIB_CACHE" \
    CC="${CROSS}gcc" CXX="${CROSS}g++" AR="${CROSS}ar" RANLIB="${CROSS}ranlib" \
    CPPFLAGS="-I$STAGE/usr/include" \
    PKG_CONFIG=/usr/bin/pkg-config \
    PKG_CONFIG_LIBDIR="$STAGE/usr/lib/pkgconfig" \
    PKG_CONFIG_SYSROOT_DIR="$STAGE" \
    "$ROOT/third_party/glib-2.26.1/configure" \
        --build="$(gcc -dumpmachine)" \
        --host=arm-buildroot-linux-uclibcgnueabi \
        --prefix=/usr --disable-shared --enable-static \
        --disable-selinux --disable-fam --disable-xattr \
        --disable-rebuilds --disable-gtk-doc --disable-man --with-pcre=internal
    make -j"$(getconf _NPROCESSORS_ONLN)"
    make DESTDIR="$STAGE" install
    # GLib 2.26 pode deixar gregex.h de fora apos uma reconfiguracao.
    # Instala explicitamente o conjunto de cabecalhos publicos do GLib.
    make -C glib DESTDIR="$STAGE" install-glibsubincludeHEADERS
)

echo '[3/4] nbd-server 3.9.1 com GLib estatica'
cp -a "$ROOT/third_party/nbd-nbd-3.9.1" "$BUILD/nbd-src"
patch -d "$BUILD/nbd-src" -p1 < "$SCRIPT_DIR/patches/001-evitar-dns-para-cliente-numerico.patch"
patch -d "$BUILD/nbd-src" -p1 < "$SCRIPT_DIR/patches/002-cliente-leitura-tcp-completa.patch"
for page in nbd-server.1 nbd-server.5 nbd-client.8 nbd-trdump.1; do
    cp "$BUILD/nbd-src/man/sh.tmpl" "$BUILD/nbd-src/man/$page.sh.in"
done
(
    cd "$BUILD/nbd-src"
    autoreconf -f -i
)
mkdir "$BUILD/nbd"
(
    cd "$BUILD/nbd"
    CC="${CROSS}gcc" AR="${CROSS}ar" RANLIB="${CROSS}ranlib" \
    CFLAGS='-Os -ffunction-sections -fdata-sections' \
    LDFLAGS='-static -Wl,--gc-sections' \
    PKG_CONFIG=/usr/bin/pkg-config \
    PKG_CONFIG_LIBDIR="$STAGE/usr/lib/pkgconfig" \
    PKG_CONFIG_SYSROOT_DIR="$STAGE" \
    ../nbd-src/configure --build="$(gcc -dumpmachine)" \
        --host=arm-buildroot-linux-uclibcgnueabi \
        --prefix=/usr --sysconfdir=/etc --localstatedir=/var
    make -j"$(getconf _NPROCESSORS_ONLN)"
    # Libtool descarta -static na etapa final. Ligar diretamente garante que
    # a uClibc mais nova do toolchain nao vire dependencia da uClibc 2.6.x.
    "${CROSS}gcc" -static -Wl,--gc-sections -o nbd-server.static \
        nbd_server-nbd-server.o -L"$STAGE/usr/lib" -lglib-2.0 \
        ./.libs/libnbdsrv.a ./.libs/libcliserv.a -pthread -lz -ldl
    "${CROSS}gcc" -static -Wl,--gc-sections -o nbd-client.static \
        nbd-client.o ./.libs/libcliserv.a
    "${CROSS}strip" --strip-unneeded nbd-server.static -o "$OUT/nbd-server"
    "${CROSS}strip" --strip-unneeded nbd-client.static -o "$OUT/nbd-client"
)

echo '[4/4] verificacao'
file "$OUT/nbd-server" "$OUT/nbd-client"
"${CROSS}readelf" -d "$OUT/nbd-server" | grep NEEDED && {
    echo 'erro: nbd-server ARM deveria ser totalmente estatico' >&2
    exit 1
} || true
sha256sum "$OUT/nbd-server" "$OUT/nbd-client"
echo "pronto: $OUT"
