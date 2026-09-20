# Investigação de overclock — SD5116

Escopo: análise estática e leituras futuras cuidadosamente justificadas. Não há
autorização neste documento para escrever MMIO, U-Boot, environment ou NAND.

## Resultado atual

**DESCONHECIDO:** não existe ainda um caminho seguro demonstrado para mudar a
frequência. O SD5116 usa Cortex-A9 e o boot mediu `996.14 BogoMIPS`, mas isso é
uma calibração de loops, não a especificação de um registrador PLL nem prova de
que a CPU esteja exatamente em 1 GHz.

Não há texto, módulo ou interface `cpufreq`, `scaling_*`, governor ou DVFS no
kernel Linux 2.6.34.10 desta imagem. Logo não há mecanismo Linux suportado para
mudar a frequência.

## Evidência estática confirmada

| Item | Evidência |
|---|---|
| U-Boot A/B | `mtd1-ubootA.bin` e `mtd2-ubootB.bin` são idênticos: SHA-256 `1a584727eda858217e8abba20103b1f1393c733060328ef9af9138750f845219`. |
| Código inicial | A imagem contém vetor/código ligado em `0x81200000`; identifica-se como U-Boot 2010.03 de 2016. |
| System controller | O bootstrap lê `0x10100000 + 0x800` e compara com IDs `0x51161000`, `0x51160000`, `0x51163000` e `0x51162000`. Isto confirma uma janela de identificação do SoC em `0x10100000`; não identifica seu layout de clocks. |
| MMIO adjacente | O mesmo código referencia `0x1010011c` e `0x10100120`; a função desses endereços é desconhecida. Há escrita de `0x00000dd2` em `0x10100120` no bootstrap. Não ler/escrever aleatoriamente esses endereços. |
| DDR/periféricos | O bootstrap também programa blocos `0x14880000`, `0x14900000` e outros. Sem mapa, não é seguro separar DDR, reset, PHY e clocks apenas por proximidade. |

Os endereços acima são pistas de engenharia reversa, **não** candidatos a
overclock.

## Por que `devmem` não é caminho inicial

Um registrador de clock pode compartilhar domínio com DDR, AXI/AHB, UART,
NAND, Ethernet ou watchdog. Uma escrita incorreta pode congelar a CPU antes de
o UART imprimir algo; alterar NAND/DDR durante acesso pode corromper dados.
O comando U-Boot `mw` e qualquer helper de escrita MMIO ficam proibidos nesta
fase. O slot A não compensa um SoC que não chega a executar U-Boot.

## Caminho seguro obrigatório

1. **Preservar a linha de base.** Registrar hashes de U-Boot, `uImage`, saída
   de `cpuinfo`, boot log, temperatura se houver sensor, contador de falhas e
   consumo externo. Usar uma unidade reserva, UART conectado e slot A intacto.
2. **Mapear por código, não por tentativa.** Importar U-Boot em Ghidra/radare
   como ARM little-endian com base `0x81200000`. Rastrear todas as rotinas que
   usam `0x10100000–0x101001ff` e suas chamadas antes de DDR/UART. Procurar o
   código-fonte/SDK SD5116 correspondente para obter campos, sequência de
   bypass, reset e lock de PLL.
3. **Provar isolamento do domínio.** Só avançar se documentação/código
   demonstrar um divisor/PLL de CPU independente de DDR, barramento, UART,
   NAND, GPON e Ethernet; registrar máscara, valor original, valor novo,
   bit de lock e timeout. Se alterar DDR/AXI junto, parar.
4. **Leitura dirigida.** Depois do mapa, capturar somente os registradores
   comprovadamente de configuração/estado, antes e após reboot, para confirmar
   valores estáveis. Não varrer MMIO nem usar leitura em registradores com
   efeito de limpeza reconhecido.
5. **Teste volátil mínimo.** Reproduzir em RAM a sequência completa de
   bypass → divisor/PLL → espera de lock → retorno de bypass. Nunca gravar
   U-Boot. A primeira variação deve ser pequena e ter timeout independente.
   UART deve permanecer conectado; desligar alimentação é o plano de retorno.
6. **Validação.** Medir estabilidade por pelo menos 30 minutos: console,
   memória, NAND somente leitura, Ethernet, carga de CPU e temperatura. Só
   após repetição em unidade reserva considerar qualquer integração de boot.

## Critérios de parada

Parar imediatamente se faltar mapa de bits, se DDR/AXI não puder ser separado,
se o lock não puder ser observado, se UART depender do mesmo clock, ou se a
frequência original não puder ser restaurada em RAM. Não criar patch de
firmware/bootloader enquanto qualquer um desses itens permanecer desconhecido.

## Próxima tarefa estática

Criar uma listagem de referências cruzadas para os três endereços de system
controller e classificar cada acesso como leitura/escrita/máscara. Isso reduz o
alvo de pesquisa sem interagir com a ONU.
