# NBD na ONU SD5116 — engenharia, compatibilidade e testes

## Escopo e segurança

Esta investigação usou exclusivamente arquivos temporários de 1 MiB em
`/tmp`, sempre exportados em modo somente leitura. Não foram usados MTDs,
partições, arquivos de configuração nem qualquer área persistente da ONU.

Os endereços nos exemplos são variáveis propositalmente. Não fixe endereço,
MAC, número de série ou credencial em scripts.

## 1. Evidência do cliente existente

### Confirmado

- BusyBox 1.18.3 contém o applet `nbd-client`.
- O kernel possui o driver NBD: major 43 e `/dev/nbd0` até `/dev/nbd15`.
- A ajuda do cliente nativo é:

  ```text
  nbd-client HOST PORT BLOCKDEV
  ```

- A análise do BusyBox encontrou `NBDMAGIC` e `NBD_SET_SOCK`, sem evidência de
  `IHAVEOPT`.

### Inferido

O cliente nativo usa NBD **oldstyle**, baseado em porta TCP. Esse modo começa
com o cabeçalho `NBDMAGIC` e o magic `0x00420281861253`; não seleciona export
por nome como o protocolo newstyle.

## 2. Fonte de referência e GLib

Foi obtido `nbd-3.9.1`, que ainda implementa oldstyle. O `nbd-server` dessa
versão exige GLib >= 2.26, portanto o build cruzado usa:

| Componente | Versão | Uso |
| --- | --- | --- |
| zlib | 1.2.13 | dependência da GLib |
| GLib | 2.26.1 | dependência do NBD 3.9.1 |
| NBD | 3.9.1 | cliente e servidor de referência |

Os fontes e os tarballs estão em `third_party/`. Os hashes SHA-256 estão no
histórico de build e podem ser recalculados com:

```sh
sha256sum third_party/nbd-3.9.1.tar.gz \
          third_party/glib-2.26.1.tar.gz \
          third_party/zlib-1.2.13.tar.gz
```

### Cache de configuração da GLib

O configure da GLib 2.26.1 tenta executar pequenos binários para decidir
características da plataforma. Isso não é possível durante compilação cruzada.
As quatro respostas necessárias ficam versionadas em
[config/glib-2.26.1-arm.cache](config/glib-2.26.1-arm.cache), carregado por
`CONFIG_SITE` no `build-arm.sh`:

| Variável | Valor | Motivo |
| --- | --- | --- |
| `glib_cv_stack_grows` | `no` | pilha ARM cresce para endereços menores |
| `glib_cv_uscore` | `no` | símbolos ELF ARM não têm `_` inicial |
| `ac_cv_func_posix_getpwuid_r` | `yes` | API POSIX presente na uClibc do toolchain |
| `ac_cv_func_posix_getgrgid_r` | `yes` | API POSIX presente na uClibc do toolchain |

Esse cache é configuração de build, não patch no fonte da GLib.

### Compatibilidade da libc

O primeiro `nbd-server` ARM, ligado dinamicamente, falhou na ONU com símbolos
ausentes da libc, incluindo `__ctype_tolower_loc`, `__ctype_b_loc` e
`fallocate64`. Isso demonstra incompatibilidade entre a uClibc do toolchain e
a uClibc mais antiga presente no firmware.

**Correção aplicada:** o build ARM liga diretamente com `-static`; a GLib,
zlib e a uClibc do toolchain ficam dentro do executável. O teste
`nbd-server -V` na ONU confirmou que o binário estático executa.

O `libtool` do projeto elimina `-static` da ligação final. Por isso
`build-arm.sh` refaz explicitamente a ligação final com `${CROSS}gcc -static`
para `nbd-server` e `nbd-client`.

## 3. Patch salvo para o nbd-server 3.9.1

O patch [001-evitar-dns-para-cliente-numerico.patch](patches/001-evitar-dns-para-cliente-numerico.patch)
é aplicado automaticamente por `build-arm.sh` à cópia de build, nunca ao
fonte em `third_party/`.

Ele corrige dois problemas em `set_peername()`:

1. `addr` apontava para `netaddr`, que não foi preenchido por `getpeername()`;
   agora aponta para `client->clientaddr`, o destino real da chamada.
2. A resolução `getaddrinfo()` era executada até para um IP já numérico. Ela
   agora ocorre somente no modo `VIRT_CIDR` e usa `AI_NUMERICHOST`.

O patch passa em `patch --dry-run` sobre o fonte puro 3.9.1.

### Limitação confirmada

Mesmo estático e com esse patch, o servidor de referência não foi confiável na
ONU: aceitou a conexão, mas bloqueou antes de fornecer uma sessão NBD útil.
Ele também não conseguiu daemonizar normalmente (`Error: daemon`); foi
necessário o modo `-d` sob `nohup` apenas para investigá-lo. Portanto o patch
é preservado como correção e evidência, mas **não torna o servidor de
referência aprovado para uso**.

## 4. Servidor que funcionou

O arquivo [src/nbd-oldstyle-server.c](src/nbd-oldstyle-server.c) foi criado
para a plataforma. Não usa GLib, threads, daemonização ou DNS.

Propriedades:

- IPv4 TCP, com negociação oldstyle ou newstyle (`--newstyle`);
- estático para ARM;
- um cliente por vez;
- somente leitura: `NBD_CMD_WRITE` responde `EPERM`;
- suporta `NBD_CMD_READ` e `NBD_CMD_DISC`;
- limita uma leitura a 1 MiB;
- exige explicitamente o tamanho da exportação.

O tamanho explícito é necessário porque `fstat()` no binário estático retornou
`Function not implemented` no kernel antigo. A sintaxe é:

```sh
nbd-oldstyle-server [--newstyle] PORTA ARQUIVO TAMANHO_BYTES
```

Compile-o para PC e ONU com:

```sh
sh projects/nbd-legacy/build-oldstyle-server.sh
```

Os resultados são `bin/host/nbd-oldstyle-server` e
`bin/arm/nbd-oldstyle-server`.

## 5. Teste funcional ONU → PC

### Preparação no PC

```sh
PC_IP=IP_DO_PC
PORTA=40110
IMAGEM=/tmp/nbd-teste.img
printf 'NBD-ONU-READONLY-OK\n' > "$IMAGEM"
dd if=/dev/zero bs=1024 count=1024 >> "$IMAGEM"
projects/nbd-legacy/bin/host/nbd-oldstyle-server "$PORTA" "$IMAGEM" 1048576
```

### Cliente nativo da ONU

Use um dispositivo comprovadamente livre, aqui `nbd14`:

```sh
busybox nbd-client "$PC_IP" "$PORTA" /dev/nbd14 &
sleep 2
cat /sys/block/nbd14/size
dd if=/dev/nbd14 bs=1 count=21
```

### Resultado confirmado

- `/sys/block/nbd14/size` mostrou `2048` setores, equivalentes a 1 MiB;
- a leitura retornou `NBD-ONU-READONLY-OK`;
- nenhuma escrita foi emitida.

Para desconectar, o `nbd-client` de referência estático possui `-d` e foi
usado somente como helper de ioctl:

```sh
/tmp/nbd-client-static -d /dev/nbd14
```

O kernel pode manter a geometria do dispositivo em sysfs depois de remover o
socket; confirme a desconexão pelo retorno `disconnect, sock, done` e pela
ausência de cliente/processo, não apenas pelo tamanho exibido.

## 6. Teste funcional PC → ONU

### Preparação na ONU

```sh
ONU_PORTA=40112
dd if=/dev/zero of=/tmp/nbd-teste.img bs=1024 count=1024
printf 'NBD-PC-NEWSTYLE-READONLY-OK\n' | \
  dd of=/tmp/nbd-teste.img conv=notrunc
nohup /tmp/nbd-oldstyle-server --newstyle "$ONU_PORTA" /tmp/nbd-teste.img 1048576 \
    >/tmp/nbd-teste.log 2>&1 &
```

### Cliente do PC

O `nbd-client` do PC é 3.26.1 e usa newstyle. Por padrão ele envia
`NBD_OPT_GO`, opção que o servidor mínimo deliberadamente não implementa.
`-g` força o `NBD_OPT_EXPORT_NAME`, que é suportado. Com o módulo `nbd` do
PC carregado, execute:

```sh
PORTA=40112
sudo modprobe nbd nbds_max=4 max_part=0
sudo nbd-client -g -N teste IP_DA_ONU "$PORTA" /dev/nbd0
cat /sys/block/nbd0/size
sudo dd if=/dev/nbd0 bs=1 count=28 status=none
sudo nbd-client -d /dev/nbd0
sudo modprobe -r nbd
```

Resultado confirmado:

```text
Connected /dev/nbd0
setores: 2048
leitura: NBD-PC-NEWSTYLE-READONLY-OK
```

O log do servidor confirmou várias leituras de 4096 bytes, a leitura final de
16384 bytes iniciada no offset zero e `NBD_CMD_DISC`. Depois de desconectar,
`/sys/block/nbd0/size` voltou a zero e o módulo `nbd` foi descarregado.

Assim há prova de negociação, mapeamento pelo driver do PC, leitura NBD real e
desconexão em ambos os sentidos.

## 7. Limpeza e incidente do nbd15

Os servidores, imagens e arquivos PID de teste foram encerrados/removidos.
Os binários temporários em `/tmp` foram preservados para inspeção.

`/dev/nbd14` foi desconectado e voltou a tamanho zero. Um teste anterior com
o `nbd-server` de referência deixou `/dev/nbd15` com geometria de 2048 setores
sem socket/servidor ativo; não é NAND, MTD nem armazenamento persistente. Um
reboot da ONU limpa esse estado de teste antes de reutilizar `nbd15`.

## 8. Estado final

| Item | Estado |
| --- | --- |
| Applet BusyBox `nbd-client` | Confirmado e funcional |
| Driver e nós NBD da ONU | Confirmados |
| Servidor mínimo ARM oldstyle | Confirmado: cliente BusyBox da ONU → PC |
| Servidor mínimo ARM newstyle | Confirmado: cliente 3.26.1 do PC → ONU |
| Leitura read-only ponta a ponta | Confirmada nos dois sentidos |
| Escrita NBD | Não implementada intencionalmente |
| `nbd-server` 3.9.1 na ONU | Não aprovado; investigação pendente |
| `nbd-client` 3.9.1 no PC com `/dev/nbd0` | Removido do fluxo; use o cliente NBD nativo do PC |
| Uso de MTD/flash via NBD | Não testado e fora de escopo |
