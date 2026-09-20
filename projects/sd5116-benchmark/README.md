# `onu-benchmark`

Benchmark reproduzível para comparar a ONU antes e depois de instalar um
dissipador. Executa, em sequência, um teste de cálculo inteiro e um teste de
leitura/modificação sequencial de memória.

Não grava NAND, MTD ou configuração persistente. O buffer de memória é alocado
somente durante o teste e é liberado ao terminar.

## Compilação

```sh
make
make host-check
```

O resultado é `bin/onu-benchmark`, um ELF ARM EABI estático com uClibc-ng.

## Uso

```sh
# Padrão: 10 s de CPU e 10 s de memória, usando 8 MiB.
/tmp/onu-benchmark

# Recomendado para comparar dissipador: 60 s por teste, 8 MiB.
/tmp/onu-benchmark -s 60 -m 8

# Carga de memória menor se a ONU estiver com pouca RAM livre.
/tmp/onu-benchmark -s 60 -m 4
```

A saída usa `chave=valor`, adequada para salvar no PC. As métricas principais
são `cpu.million_iterations_per_second` e `memory.mib_per_second`. Como os
testes terminam por tempo, o número de iterações e os checksums podem variar
ligeiramente entre rodadas; compare as taxas, não os checksums. Um checksum
apenas registra que o loop executou cálculos e acessos de memória durante aquela
rodada.

## Protocolo de comparação

1. Use a mesma fonte de 12 V, cabos, firmware, serviços e temperatura ambiente.
2. Após cinco minutos em repouso, execute três vezes:

   ```sh
   /tmp/onu-benchmark -s 60 -m 8
   ```

3. Registre a mediana das três taxas de CPU e memória.
4. Para observar aquecimento sustentado, rode antes `onu-cpu-stress -d 300` e
   execute o benchmark imediatamente após ele; repita o procedimento idêntico
   antes e depois do dissipador.
5. Meça a corrente de 12 V externamente e, se possível, a temperatura do
   encapsulamento do SD5116 com sonda/termopar.

## Interpretação

**Confirmado:** o kernel atualmente expõe uma CPU lógica e não foi encontrada
interface térmica confirmada. **Inferido:** se o clock for fixo e não houver
throttling, um dissipador não mudará significativamente o benchmark; ele ainda
melhora a margem térmica e estabilidade. Uma melhora repetível após aquecimento
indica limitação térmica, de alimentação ou outra variação de estado, mas não
identifica a causa sozinha.
