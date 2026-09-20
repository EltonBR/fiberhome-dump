#!/usr/bin/env bash
# Cria modified/<particao> a partir de extracted/<particao> e aplica somente
# os patches indicados. O host pode usar Bash; os patches devem ser POSIX sh.
set -euo pipefail

cd "$(dirname "$0")/.."
particao=${1:?uso: $0 <particao> <patch> [<patch>...]}
shift
test "$#" -gt 0 || { echo "Informe ao menos um patch." >&2; exit 1; }

base="extracted/$particao"
destino="modified/$particao"
test -d "$base" || { echo "Partição extraída não encontrada: $base" >&2; exit 1; }
test ! -e "$destino" || { echo "Destino já existe e não será substituído: $destino" >&2; exit 1; }

mkdir -p modified
cp -a "$base" "$destino"

for patch in "$@"; do
    case "$patch" in
        patches/*.sh) ;;
        *) patch="patches/$patch" ;;
    esac
    test -f "$patch" || { echo "Patch não encontrado: $patch" >&2; exit 1; }
    ROOTFS="$destino" sh "$patch"
done

echo "Resultado criado em: $destino"
