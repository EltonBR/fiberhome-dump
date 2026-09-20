# NBD legado para a ONU

Este projeto fixa o `nbd-server`/`nbd-client` na versão **3.9.1** e
produz dois conjuntos de binários:

- `bin/host/`: PC anfitrião;
- `bin/arm/`: ARM EABI5/uClibc da ONU.

Também há `nbd-oldstyle-server`: um servidor pequeno, estático e somente
leitura, feito para o kernel/uClibc antigos da ONU. Apesar do nome histórico,
ele oferece dois modos de negociação: `oldstyle`, necessário para o applet
BusyBox da ONU, e `--newstyle`, necessário para o `nbd-client` atual do PC.
Ele é o servidor usado nos testes funcionais abaixo.

## Por que esta versão

O applet BusyBox da ONU contém `NBDMAGIC` e a ioctl `NBD_SET_SOCK`, mas não
há evidência de `IHAVEOPT`. Isso é compatível com o protocolo NBD **oldstyle**
(baseado em porta). O `nbd-server` 3.9.1 ainda o implementa. A verificação
local confirmou o cabeçalho oldstyle:

```
NBDMAGIC + 00 00 42 02 81 86 12 53
```

O segundo valor é o magic oldstyle `0x00420281861253`.

## Compilação

Os fontes permanecem em `third_party/`; produtos intermediários, inclusive o
staging cruzado da GLib, ficam sob `build/`.

```sh
sh projects/nbd-legacy/build-host.sh
sh projects/nbd-legacy/build-arm.sh
sh projects/nbd-legacy/build-oldstyle-server.sh
```

O script ARM compila, nesta ordem:

1. zlib 1.2.13 estática;
2. GLib 2.26.1 estática, com PCRE interno;
3. NBD 3.9.1.

As respostas de configuração cruzada da GLib estão salvas em
[`config/glib-2.26.1-arm.cache`](config/glib-2.26.1-arm.cache); elas são
carregadas automaticamente pelo script ARM.

Os binários ARM finais são **totalmente estáticos**. A GLib e a uClibc do
toolchain são ligadas ao executável; portanto não há dependência dinâmica de
`libglib-2.0.so` nem da `libc.so.0` da ONU.

## Servidor mínimo para a ONU

```sh
nbd-oldstyle-server [--newstyle] PORTA ARQUIVO TAMANHO_BYTES
```

Ele aceita um cliente por vez, implementa NBD oldstyle e responde somente a
leituras. Escritas recebem `EPERM`. O tamanho é passado explicitamente para
não depender de `fstat`, syscall que falhou com a combinação atual de kernel
2.6 e uClibc estática.

Sem `--newstyle`, emite o cabeçalho oldstyle para o `busybox nbd-client` da
ONU. Com `--newstyle`, aceita `NBD_OPT_EXPORT_NAME` de um cliente atual do PC,
ignora o nome solicitado e exporta o único arquivo informado. Esse modo não
implementa `NBD_OPT_GO`; portanto o cliente NBD 3.26.1 do PC deve ser chamado
com `-g`, que seleciona `NBD_OPT_EXPORT_NAME`.

## Uso seguro para o primeiro teste

No PC, crie uma imagem descartável e exporte-a somente para leitura em uma
porta alta. O modo de linha de comando do servidor é oldstyle.

```sh
truncate -s 16M /tmp/nbd-teste.img
projects/nbd-legacy/bin/host/nbd-server 40109 /tmp/nbd-teste.img -r
```

Antes de conectar a ONU, confirme nela a existência de `/dev/nbd0` e do
suporte de kernel NBD. Não use uma MTD, uma partição do firmware, nem qualquer
arquivo importante como exportação inicial.

Para configurar vários exports oldstyle, use um arquivo com:

```ini
[generic]
oldstyle = true

[teste]
exportname = /tmp/nbd-teste.img
port = 40109
readonly = true
```

e inicie com `nbd-server -C /caminho/absoluto/config`.

## Estado de validação

- Confirmado: build x86-64 do PC e handshake oldstyle local.

## Alvos permitidos e incidente `romblock0`

O teste deve usar exclusivamente um arquivo regular (por exemplo,
`/tmp/nbd-teste.img`) como exportação e um nó NBD livre (`/dev/nbdN`). Não
aponte o cliente para `/dev/romblock0`, `/dev/mtd*`, `romblock*` ou qualquer
partição NAND. Esses nós pertencem ao driver de flash da ONU; um acesso
inválido pode produzir `map_over_bad_blocks(): no more good blocks!` e erros de
I/O, sem relação com a compatibilidade do NBD.

Os binários são separados por arquitetura:

* `bin/host/nbd-server`: PC x86-64 (o cliente do PC não faz parte deste
  fluxo; use o `nbd-client` nativo da distribuição);
* `bin/arm/nbd-client` e `bin/arm/nbd-server`: ARM/uClibc da ONU;
* `bin/arm/nbd-oldstyle-server`: servidor mínimo ARM estático usado nos
  testes da ONU.
- Confirmado: build ARM EABI5 totalmente estático.
- Confirmado: `nbd-server -V` executado na ONU a partir de `/tmp`.
- Confirmado: `nbd-client` estático executado na ONU e exibiu sua sintaxe;
  essa versão não oferece a opção `-V`.
- Confirmado: `nbd-client` BusyBox nativo mapeou uma exportação read-only do
  PC em `/dev/nbd14`, informou 2048 setores e leu a assinatura de teste.
- Confirmado: o `nbd-client` 3.26.1 nativo do PC mapeou uma exportação
  temporária da ONU em `/dev/nbd0`, recebeu 2048 setores, leu a assinatura e
  desconectou. O servidor foi executado com `--newstyle` e o cliente com `-g`.
- Inferido: o `nbd-server` 3.9.1 de referência não é confiável neste kernel:
  negociou tamanho, mas bloqueou ao atender requisições. Use o servidor mínimo
  para testes até investigar esse caminho separadamente.

O relato completo, comandos, falhas observadas e critérios de limpeza está em
[ENGENHARIA_E_TESTES.md](ENGENHARIA_E_TESTES.md). Os patches estão documentados
em [patches/README.md](patches/README.md).
