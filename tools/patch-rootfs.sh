#!/usr/bin/env bash
# Copia uma árvore extraída para modified/ e aplica nela os patches idempotentes.
set -euo pipefail
cd "$(dirname "$0")/.."
target=${1:?uso: $0 <kernel_rootfsA|kernel_rootfsB|app_binA|app_binB|app_exA|app_exB>}
base="extracted/$target"
root="modified/$target"
test -d "$base" || { echo "$base não existe; execute tools/extract.sh primeiro" >&2; exit 1; }
test ! -e "$root" || { echo "$root já existe; recusando substituir" >&2; exit 1; }
mkdir -p modified
cp -a "$base" "$root"

for patch in patches/*.sh; do
  test -e "$patch" || continue
  ROOTFS="$root" sh "$patch"
done
