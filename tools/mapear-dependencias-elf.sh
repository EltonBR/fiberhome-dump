#!/usr/bin/env bash
# Gera em stdout um relatório Markdown de dependências ELF diretas (DT_NEEDED).
# Uso: tools/mapear-dependencias-elf.sh > docs/MAPA_BIBLIOTECAS_APP_EXB.md
set -euo pipefail

cd "$(dirname "$0")/.."
for diretorio in extracted/kernel_rootfsB extracted/app_binB extracted/app_exB; do
    test -d "$diretorio" || { echo "Diretório ausente: $diretorio" >&2; exit 1; }
done

declare -A biblioteca_local=()
declare -A consumidores=()
declare -A consumidores_executaveis=()
declare -A dependencias=()

while IFS= read -r -d '' caminho; do
    biblioteca_local["$(basename "$caminho")"]=1
done < <(find extracted/app_exB -maxdepth 1 -type f -name '*.so*' -print0)

while IFS= read -r -d '' caminho; do
    readelf -h "$caminho" >/dev/null 2>&1 || continue
    relativo=${caminho#extracted/}
    while IFS= read -r necessidade; do
        nome=$(basename "$necessidade")
        test -n "${biblioteca_local[$nome]:-}" || continue
        consumidores["$nome"]+="$relativo"$'\n'
        case "$caminho" in
            *.so|*.so.*) ;;
            *) consumidores_executaveis["$nome"]+="$relativo"$'\n' ;;
        esac
        dependencias["$relativo"]+="$nome"$'\n'
    done < <(readelf -d "$caminho" 2>/dev/null | sed -n 's/.*Shared library: \[\(.*\)\].*/\1/p')
done < <(find extracted/kernel_rootfsB extracted/app_binB extracted/app_exB -type f -print0)

ordenar_lista() {
    if test -n "${1:-}"; then
        printf '%s' "$1" | sed '/^$/d' | sort -u | awk 'BEGIN { separador="" } { printf "%s%s", separador, $0; separador=", " }'
    else
        printf '%s' '—'
    fi
}

cat <<'EOF'
# Dependências ELF de `app_exB`

Relatório gerado automaticamente por `tools/mapear-dependencias-elf.sh`.

## Método e limite

O relatório lê `DT_NEEDED` com `readelf -d` de todos os ELF nas partições B e
relaciona apenas as bibliotecas que existem diretamente em `app_exB/`.

- A relação é **direta**: `A → B` significa que o ELF A declara B em
  `DT_NEEDED`.
- A relação não inclui dependências transitivas, `dlopen()`, scripts shell,
  módulos `.ko` ou bibliotecas carregadas por caminho construído em runtime.
- A lista principal é organizada por ELF consumidor. A repetição de uma
  biblioteca em vários binários é intencional.

## Por binário: bibliotecas locais declaradas

EOF

while IFS= read -r consumidor; do
    printf '### `%s`\n\n' "$consumidor"
    while IFS= read -r biblioteca; do
        printf -- '- `%s`\n' "$biblioteca"
    done < <(printf '%s' "${dependencias[$consumidor]}" | sed '/^$/d' | sort -u)
    printf '\n'
done < <(printf '%s\n' "${!dependencias[@]}" | sort)

cat <<'EOF'

## Bibliotecas declaradas por um único executável

Esta seção ignora consumidores que também são bibliotecas `.so`; ela responde
quais bibliotecas de `app_exB` são declaradas diretamente por exatamente um
executável ELF das três partições B. Isso não comprova exclusividade em runtime
nem ausência de `dlopen()`.

EOF

while IFS= read -r consumidor; do
    exclusivas=""
    while IFS= read -r biblioteca; do
        lista=${consumidores_executaveis[$biblioteca]:-}
        quantidade=$(printf '%s' "$lista" | sed '/^$/d' | sort -u | wc -l)
        if test "$quantidade" -eq 1 && test "$(printf '%s' "$lista" | sed '/^$/d' | sort -u)" = "$consumidor"; then
            exclusivas+="$biblioteca"$'\n'
        fi
    done < <(printf '%s' "${dependencias[$consumidor]}" | sed '/^$/d' | sort -u)
    if test -n "$exclusivas"; then
        printf '### `%s`\n\n' "$consumidor"
        while IFS= read -r biblioteca; do
            printf -- '- `%s`\n' "$biblioteca"
        done < <(printf '%s' "$exclusivas" | sed '/^$/d' | sort -u)
        printf '\n'
    fi
done < <(printf '%s\n' "${!dependencias[@]}" | sort)

cat <<'EOF'

## Apêndice: por biblioteca, quem a declara

Um `—` significa que nenhum ELF das três partições B declarou a biblioteca
diretamente pelo nome presente em `app_exB`; não significa que ela seja
descartável, pois ainda pode haver `dlopen()` ou nome construído em runtime.

| Biblioteca em `app_exB` | Consumidores ELF diretos |
| --- | --- |
EOF

while IFS= read -r biblioteca; do
    printf '| `%s` | %s |\n' "$biblioteca" "$(ordenar_lista "${consumidores[$biblioteca]:-}")"
done < <(printf '%s\n' "${!biblioteca_local[@]}" | sort)

cat <<'EOF'

## Reprodução

```sh
tools/mapear-dependencias-elf.sh > docs/MAPA_BIBLIOTECAS_APP_EXB.md
```

O script é destinado ao computador de análise (Bash e `readelf`), não à ONU.
EOF
