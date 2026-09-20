# RAM excluída do Linux — investigação

Objetivo: identificar o intervalo `0x80000000–0x804fffff` antes de considerar
mais RAM para Linux. Esta investigação não autoriza alterar `mem=`, U-Boot ou
kernel.

## Confirmado

| Fato | Evidência |
|---|---|
| RAM que Linux administra | `/proc/iomem`: `0x80500000–0x83ffffff` (59 MiB). |
| Origem do limite | U-Boot imprime `Memory Start: 80500000`; o kernel imprime `PHYS_OFFSET = 80500000` e recebe `mem=59M`. Portanto o limite vem do layout de boot/kernel, não de um processo Linux. |
| Buffer de carga | U-Boot lê `uImage` para `0x80008000`, dentro dos 5 MiB excluídos, e depois o carrega no endereço da imagem `0x81008000`. |
| Reserva interna do kernel | Os `4396 kB reserved` do log são **dentro** dos 59 MiB administrados. Não explicam os 5 MiB inferiores. |
| DSP/VoIP | `kinit` configura firmware `/usr/sbin/zsp_min.out` com endereço físico `0x80a00000`, dentro da janela Linux. O firmware DSP tem segmentos até aproximadamente `0x52000` no espaço do DSP. Não há evidência de que ele ocupe `0x80000000–0x804fffff`. |

## Inferido

O intervalo inferior é provavelmente uma margem de bootstrap: abriga o buffer
de `uImage` e possivelmente estruturas transitórias de U-Boot/decompressor. O
tamanho de 5 MiB ainda não está explicado por completo; não há prova de que
fique em uso depois de `Starting kernel ...`.

O texto U-Boot `DRAM: 16 MB` conflita com o kernel que administra 59 MiB. Ele
não pode ser usado para concluir capacidade física: o Linux em execução acessa
uma janela maior. A capacidade física continua **inferida**, não confirmada.

## Seleção do início no U-Boot

**Confirmado por desmontagem e descompilação de `mtd1-ubootA.bin`:** na rotina
`0x81216ad0`, o U-Boot lê `0x10100800` e guarda o valor. A rotina
`0x81216a24` examina seus bits `12–15` e escolhe uma tabela de endereços:

| Campo mascarado | Início selecionado | Outros endereços montados pela rotina |
|---:|---:|---|
| `0x0000` | `0x80900000` | `0x80908000`, `0x80900100`, `0x80908001` |
| `0x3000` | `0x80500000` | `0x80508000`, `0x80500100`, `0x80508001` |
| `0x1000` | `0x80000000` | `0x80008000`, `0x80000100`, `0x80008001` |

O boot desta ONU imprime `Memory Start: 80500000`; logo o caminho `0x3000` foi
selecionado. O significado elétrico/funcional dos bits de `0x10100800` ainda é
**desconhecido**. Em particular, não há evidência suficiente para chamá-los de
"tamanho da DDR". Mas fica comprovado que `0x80500000` é uma decisão do
firmware de boot condicionada a um registrador do SoC, e não uma alocação de
usuário, VoIP ou Linux.

O carregamento bem-sucedido do `uImage` em `0x80008000` também confirma que ao
menos essa parte da faixa inferior é acessável pelo U-Boot antes da transferência
ao kernel. Isso **não** autoriza Linux a usá-la: ainda falta descobrir se algum
coprocessador, DMA ou estágio inicial continua dependendo dela depois do salto
para o kernel.

## Estado da análise do kernel

**Confirmado:** o `uImage` B contém Linux ARM 2.6.34.10 não comprimido, com
endereço de carga/entrada `0x81008000`. A imagem contém a plataforma
`arch/arm/mach-sd5116/core.c`, o log `PHYS_OFFSET = %x` e as rotinas genéricas
de bootmem. O log runtime mostra que ela efetivamente inicializa somente
`0x80500000–0x83ffffff`.

**Não encontrado:** não há, nos logs nem nas strings do kernel, uma reserva
Linux explícita de 5 MiB (`reserve_bootmem`, CMA ou equivalente) que explique a
faixa baixa. Isso é coerente com a exclusão feita antes do kernel pelo U-Boot.

**Desconhecido:** a imagem bruta não preserva símbolos nem o código-fonte de
`mach-sd5116`; ainda falta associar as estruturas de boot ARM ao motivo
funcional da seleção do registrador `0x10100800`. A próxima evidência útil é o
SDK/source correspondente ou uma reversão mais completa de `core.c`; não há
base segura para mudar `PHYS_OFFSET` antes disso.

## Consequência importante

Mudar somente `mem=59M` para `mem=64M` é incorreto: o kernel continua compilado
com `PHYS_OFFSET=0x80500000`. Ele passaria a mapear `0x80500000–0x844fffff`,
não os 5 MiB inferiores. Se a RAM física terminar em `0x83ffffff`, isso também
acessa 5 MiB inexistentes.

Para realmente recuperar o intervalo inferior é necessário um kernel novo com
base física `0x80000000`, além de revisar o endereço temporário de carga do
`uImage` e a área de descompressão. Isso é uma alteração de boot, não um ajuste
de environment.

## Próximas verificações seguras

1. Este U-Boot não oferece `bdinfo`. Reverter as rotinas que imprimem
   `Memory Start` e geram
   `tmp_cmd b =mem=59M`; determinar se `0x80500000` é constante, variável de
   board ou resultado de detecção.
2. Reverter `arch/arm/mach-sd5116/core.c` do kernel para localizar a origem de
   `PHYS_OFFSET`. Obter o source/SDK correspondente é preferível.
3. Antes de um kernel de teste, provar capacidade física por método
   não-destrutivo fora da área de código e manter UART e slot A intactos.

Não testar `mem=64M` na imagem atual.

## `memchk` não é seguro

**Confirmado por desmontagem de `mtd1-ubootA.bin`:** apesar da descrição
`simple RAM overlap check`, `memchk <início> <fim>` não é uma consulta passiva.
Ele exige início a partir de `0x80000000`, calcula o intervalo, grava nele uma
sequência incremental de palavras e bytes, e relê cada posição para comparar.
As mensagens internas incluem `memory overlap check begin!`, `check rang ...`
e `... != write vlaue ...`.

Não executar `memchk` em nenhuma faixa desta RAM: ele pode sobrescrever código,
pilha, variáveis do U-Boot ou o `uImage` carregado. O nome "overlap" descreve o
teste de escrita/leitura, não uma inspeção do mapa de regiões reservadas.
