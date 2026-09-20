#!/usr/bin/env bash
# Produz apenas uma imagem lógica de partição JFFS2. NÃO cria um dump NAND
# completo flashável e nunca toca em bootloader, env ou cfg.
set -euo pipefail
cd "$(dirname "$0")/.."
target=${1:?uso: $0 <kernel_rootfsA|kernel_rootfsB|app_binA|app_binB|app_exA|app_exB>}
case "$target" in
  kernel_rootfsA) size=$((18*1024*1024));;
  app_binA) size=$((18*1024*1024));;
  app_exA) size=$((20*1024*1024));;
  kernel_rootfsB) size=$((18*1024*1024));;
  app_binB) size=$((18*1024*1024));;
  app_exB) size=$((20*1024*1024));;
  *) echo "Destino não suportado: $target" >&2; exit 2;;
esac
root="modified/$target"
test -d "$root" || { echo "$root não existe; execute tools/patch-rootfs.sh $target primeiro" >&2; exit 1; }
mkdir -p rebuilt
output="rebuilt/$target.jffs2"
test ! -e "$output" || { echo "Recusando substituir $output" >&2; exit 1; }
# Parâmetros confirmados: bloco NAND de 128 KiB, página de 2 KiB, JFFS2 LE.
mkfs.jffs2 --little-endian --no-cleanmarkers --eraseblock=0x20000 \
  --pagesize=0x800 --root="$root" --pad="$size" --output="$output"
echo "Gerado $output (somente partição lógica; não grave sem validação)."
