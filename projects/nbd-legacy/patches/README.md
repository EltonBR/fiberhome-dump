# Patches do NBD de referência

Os patches deste diretório são aplicados somente em cópias descartáveis em
`build/`. O fonte em `third_party/nbd-nbd-3.9.1/` permanece intacto.

| Patch | Finalidade | Estado |
| --- | --- | --- |
| `001-evitar-dns-para-cliente-numerico.patch` | Corrige `sockaddr` não inicializado e evita resolução DNS desnecessária no caminho de cliente numérico. | Aplicado pelo build ARM; insuficiente para aprovar o servidor de referência no kernel da ONU. |
| `002-cliente-leitura-tcp-completa.patch` | Substitui leituras únicas do cabeçalho NBD por leitura completa, evitando que fragmentação TCP deixe zeros pendentes no socket. | Aplicado pelo build ARM e host; corrigiu um defeito real, mas o cliente 3.9.1 do PC ainda não foi aprovado no driver NBD moderno. |

Validação antes de aplicar:

```sh
patch --dry-run -d third_party/nbd-nbd-3.9.1 -p1 \
  < projects/nbd-legacy/patches/001-evitar-dns-para-cliente-numerico.patch
```

O servidor que passou nos testes não é um patch do upstream: é a implementação
minimalista e independente em `../src/nbd-oldstyle-server.c`.
