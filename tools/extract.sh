#!/usr/bin/env bash
# Extrai somente partições lógicas JFFS2; nunca grava em firmware-original/.
set -euo pipefail
cd "$(dirname "$0")/.."
command -v jefferson >/dev/null

declare -A image=(
  [kernel_rootfsA]=mtd5-kernel_rootfsA.bin
  [app_binA]=mtd6-app_binA.bin
  [app_exA]=mtd7-app_exA.bin
  [kernel_rootfsB]=mtd8-kernel_rootfsB.bin
  [app_binB]=mtd9-app_binB.bin
  [app_exB]=mtd10-app_exB.bin
)

for name in "${!image[@]}"; do
  dest="extracted/$name"
  test ! -e "$dest" || { echo "Recusando substituir $dest" >&2; exit 1; }
  jefferson -d "$dest" "firmware-original/${image[$name]}"
done

# cfg começa com dados que não são filesystem. 0x740000 foi confirmado pelo binwalk.
cfg_raw="extracted/cfg-jffs2.raw"
test ! -e "$cfg_raw" || { echo "Recusando substituir $cfg_raw" >&2; exit 1; }
dd if=firmware-original/mtd11-cfg.bin of="$cfg_raw" bs=1 skip=$((0x740000)) status=none
jefferson -d extracted/cfg "$cfg_raw"
