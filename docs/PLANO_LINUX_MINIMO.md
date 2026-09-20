# Plano — Linux mínimo no slot B

## Regras imutáveis

- O slot A é recuperação histórica e nunca será alterado: `mtd5` (`kernel_rootfsA`), `mtd6` (`app_binA`) e `mtd7` (`app_exA`).
- Nunca alterar `startcode`, `u-bootA/B`, `envA/B` ou `cfg`.
- Os dumps em `firmware-original/` e as árvores em `extracted/` são imutáveis. Cada experimento começa da árvore extraída, cria uma cópia em `modified/` e é registrado por hash.
- Não criar nem gravar um firmware NAND agregado. Patches modificam somente árvores JFFS2 extraídas e produzem imagens lógicas B separadas.
- A gravação real não usará `dd` diretamente em MTD/NAND. Antes dela será necessário validar UART, recuperação e o procedimento correto de apagar/gravar NAND com tratamento de bad blocks.

## Alvo modificável

| Partição | Função | Ação planejada |
|---|---|---|
| `mtd8` | `kernel_rootfsB`, raiz ativa | manter kernel, BusyBox, init, UART e bibliotecas básicas |
| `mtd9` | `app_binB`, montada em `/fh/bin` | reduzir ou eliminar após inventário de dependências |
| `mtd10` | `app_exB`, montada em `/fh/extend` | substituir inicialização FiberHome por ambiente mínimo |

## Fases

1. Documentar integralmente o boot e a montagem dinâmica de `/fhcfg`, `/fh/bin` e `/fh/extend`.
2. Produzir um patch de diagnóstico B, sem remover módulos, que habilite console root previsível e rede Ethernet estática.
3. Confirmar boot completo, UART e uma porta Ethernet física. Só então transformar essa configuração em baseline.
4. Desabilitar serviços por grupos: WebUI/TR-069/UPnP; inicializadores PON/OMCI; PPPoE/IGMP; VoIP. Cada grupo terá patch reversível e relatório de arquivos modificados. Os drivers `hi_gpon`/`hi_epon` permanecem carregados: removê-los já causou falha de Ethernet/CFE.
5. Reduzir a imagem para um Linux mínimo mantendo somente os módulos necessários à rede Ethernet e UART.
6. Investigar SLIC/FXS e óptica separadamente. A óptica não será usada como transmissor genérico; primeiro será mapeada para diagnóstico seguro.

## Critérios antes de cada instalação B

- confirmação runtime de `Hardware: SD5116`, `hwcfg=0x24`, tamanho NAND e layout MTD;
- hashes da imagem de origem e das três imagens B geradas;
- reextração JFFS2 e comparação de conteúdo;
- UART funcional e dump completo da ONU de teste;
- lista explícita de alterações, sem arquivos de `cfg` ou identificadores por MAC/serial fixos.
