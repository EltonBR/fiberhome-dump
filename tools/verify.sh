#!/usr/bin/env bash
# Verifica uma partição lógica JFFS2 reconstruída por extração independente.
set -euo pipefail
cd "$(dirname "$0")/.."
target=${1:?uso: $0 <partição>}
image="rebuilt/$target.jffs2"
source="modified/$target"
test -f "$image" && test -d "$source" || { echo "Execute o patch e o rebuild primeiro" >&2; exit 1; }
tmp=$(mktemp -d "${TMPDIR:-/tmp}/fiberhome-verify.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
jefferson -d "$tmp/root" "$image"
diff -qr --no-dereference "$source" "$tmp/root"
echo "O conteúdo do filesystem confere: $target"
sha256sum "$image"
