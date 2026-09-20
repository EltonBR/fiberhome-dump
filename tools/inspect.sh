#!/usr/bin/env bash
# Relatório de inspeção estática. As imagens de entrada são evidência imutável.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p docs
report=docs/inspection.txt
{
  echo '# Inspeção estática'
  date -u '+Gerado (UTC): %Y-%m-%dT%H:%M:%SZ'
  echo
  echo '## SHA-256'; sha256sum firmware-original/mtd*.bin
  echo; echo '## SHA-1'; sha1sum firmware-original/mtd*.bin
  echo; echo '## MD5'; md5sum firmware-original/mtd*.bin
  echo; echo '## Identificação de arquivos'; file firmware-original/mtd*.bin
  echo; echo '## Assinaturas do Binwalk'
  for image in firmware-original/mtd*.bin; do
    binwalk "$image" || true
  done
  echo; echo '## Environment do U-Boot (strings imprimíveis)'
  strings -a -n 6 firmware-original/mtd3-envA.bin
} > "$report"
echo "Relatório gravado em $report"
