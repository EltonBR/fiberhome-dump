# Ambiente portátil de desenvolvimento SD5116

Este projeto fornece um gerador de aplicações C para a ONU. Cada aplicação
criada fica isolada em seu próprio diretório e pode ser compilada para ARM
com uClibc ou apenas verificada com o compilador do PC.

## Preparação

O gerador grava caminhos absolutos padrão para a toolchain local e o alvo
`192.168.1.245`. Assim, `./project-generator.sh nome` funciona sem variáveis.
O prefixo esperado é `arm-buildroot-linux-uclibcgnueabi-`.

```sh
SD5116_TOOLCHAIN_DIR=/opt/toolchains/sd5116-arm-uclibc \
SD5116_KERNEL_HEADERS=/opt/headers/linux-2.6.34.10/include \
SD5116_TARGET=192.168.1.245 \
./project-generator.sh meu-app
```

## Criar um projeto

```sh
./project-generator.sh hello-cli
cd hello-cli
make
```

O resultado é `bin/hello-cli`, pronto para copiar à ONU. Para validar apenas
a sintaxe no PC:

```sh
make host-check
```

## Adicionar bibliotecas

Edite o `Makefile` gerado. Inclua os diretórios de cabeçalho em `CPPFLAGS`,
os arquivos `.a` em `LIBS` e, se necessário, dependências do sistema em
`LDLIBS`. A ordem é importante: uma biblioteca que usa outra deve aparecer
antes da biblioteca fornecida.

Exemplo:

```make
GPIO_DIR := ../../libs/libfh_gpio
CPPFLAGS += -I$(GPIO_DIR)/include
LIBS += $(GPIO_DIR)/lib/libfh_gpio.a
```

O gerador cria a biblioteca `hello` em `lib/hello/`:

```text
lib/
└── hello/
    ├── libhello.a
    ├── hello.c
    └── hello.h
```

Para adicionar uma biblioteca `minha_lib`, replique essa estrutura em
`lib/minha_lib/`, mantenha `-Ilib` no `CPPFLAGS` e
`build/libminha_lib.a` ao `LIBS`. Acrescente também as regras de compilação e
arquivamento correspondentes no Makefile.

## Headers e VS Code

O Makefile usa automaticamente os headers C da sysroot da toolchain. Para
headers exportados pelo kernel 2.6.34.10, informe `SD5116_KERNEL_HEADERS` ao
gerar o projeto ou substitua `KERNEL_HEADERS_DIR` no Makefile:

```sh
make KERNEL_HEADERS_DIR=/caminho/para/linux-2.6.34.10-headers
```

O diretório deve conter `include/linux` e `include/asm`. No VS Code, abra o
diretório do projeto gerado; `.vscode/tasks.json` compila para ARM,
`.vscode/launch.json` está preparado para `gdbserver` na ONU e
`.vscode/c_cpp_properties.json` configura o compilador ARM, a sysroot e os
headers do kernel.

## Segurança

O ambiente não grava firmware nem MTD. A instalação na ONU é manual e deve
ser feita somente depois de testar o executável em um equipamento reserva.
