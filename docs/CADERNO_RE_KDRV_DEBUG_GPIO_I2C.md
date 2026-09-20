# Caderno de engenharia reversa: `kdrv_debug`, GPIO e I²C

Este documento registra o caminho que produziu os acessos usados neste projeto.
Não é um guia genérico de GPIO/I²C: cada passo abaixo partiu dos binários da
imagem `app_exB` e terminou em uma validação no hardware.

Arquivos analisados, sem alteração:

```text
extracted/app_exB/kdrv_debug
extracted/app_exB/libfhdrv_kdrv_board.so
extracted/app_exB/libfhdrv_kdrv_board_impl.so
extracted/app_exB/libi2c_interface.so
extracted/app_exB/libled_interface.so
```

As cópias em `modified/` não foram usadas como fonte desta análise.

## 1. Ponto de entrada: `kdrv_debug`

O primeiro artefato escolhido foi `app_exB/kdrv_debug`, pois ele já oferecia
comandos de diagnóstico no rootfs e permitia observar a interface sem escrever
um driver novo.

```sh
readelf -d extracted/app_exB/kdrv_debug
readelf -Ws extracted/app_exB/kdrv_debug | rg -i 'gpio|i2c|board'
strings -a extracted/app_exB/kdrv_debug | rg -i 'gpio|i2c|read|write|usage'
```

`readelf -d` mostrou que o binário depende diretamente de
`libfhdrv_kdrv_board_impl.so`, `libfhdrv_kdrv_board.so`,
`libfhdrv_kdrv_hi_adapter.so` e das bibliotecas HAL `libhi_*`.

A tabela dinâmica revelou as funções importadas que delimitam a análise:

```text
fhdrv_kdrv_gpio_config_mux
fhdrv_kdrv_gpio_set_mode
fhdrv_kdrv_gpio_read
fhdrv_kdrv_gpio_write
fhdrv_kdrv_i2c_read
fhdrv_kdrv_i2c_write
```

As strings reconstruíram a gramática exposta pelo helper:

```text
kdrv_debug gpio read pin1 pin2 ...
kdrv_debug gpio write pin val
kdrv_debug i2c read channel address offset length
kdrv_debug i2c write channel address offset value
```

O helper foi útil como oráculo de entrada e saída, não como biblioteca final.
Em particular, falhas I²C aparecem em texto (`failled!`, `fail.`), e o código
de saída do processo não foi usado isoladamente para declarar sucesso.

## 2. Rota GPIO: do subcomando ao ioctl

O símbolo local `gpio_debug` de `kdrv_debug` está em `0x8ed0`. A desmontagem
foi feita assim:

```sh
arm-buildroot-linux-uclibcgnueabi-objdump -d \
  --start-address=0x8ed0 --stop-address=0x93c0 \
  extracted/app_exB/kdrv_debug
```

O fluxo recuperado foi:

| Subcomando | Chamadas observadas | Parâmetros recuperados |
|---|---|---|
| `gpio read PIN...` | `config_mux`, `set_mode`, `read` | mux `0`; modo `0`; ponteiro para byte de retorno |
| `gpio write PIN 0|1` | `config_mux`, `set_mode`, `write` | mux `0`; modo `1`; valor `0` ou `1` |

Em seguida foi desmontada a biblioteca que implementa essas chamadas:

```sh
arm-buildroot-linux-uclibcgnueabi-objdump -d \
  --start-address=0x12a04 --stop-address=0x13300 \
  extracted/app_exB/libfhdrv_kdrv_board.so
```

### 2.1 Estrutura e chamadas

As funções `*_ioctl` zeram uma estrutura de 8 bytes, preenchem o pino no
primeiro `u32` e usam bytes seguintes para nível e debug. A representação C
reproduzida é:

```c
struct gpio_request {
    uint32_t pin;
    uint8_t level;
    uint8_t debug;
    uint8_t reserved[2];
};
```

Os valores abaixo aparecem como literais na desmontagem da biblioteca:

| Operação | ioctl |
|---|---:|
| configurar mux | `0x40085300` |
| definir direção | `0x40085301` |
| escrever nível | `0x40085303` |
| ler nível | `0xc0085304` |

A função interna abre o dispositivo de placa, chama `ioctl(fd, comando,
&request)` e devolve o resultado. O dispositivo observado no runtime foi:

```text
/dev/fhdrv_kdrv_board
```

Isto levou diretamente à implementação de
[`libs/libfh_gpio/src/fh_gpio.c`](../libs/libfh_gpio/src/fh_gpio.c). A
biblioteca executa a mesma sequência do helper: mux, direção e leitura ou
escrita. Ela não chama `kdrv_debug` e não depende de shell.

### 2.2 Como os LEDs e botões receberam função

Os números não foram deduzidos do nome da ONU. O mapa inicial veio de:

```sh
cat /proc/driver/fh_bsp_gpio_list
```

Depois cada candidato foi associado ao componente físico por teste visual ou
de pressão, usando a ABI recuperada. O teste decisivo para LED foi escrever os
dois níveis e observar a placa; para botão foi configurar entrada, ler em
repouso, pressionar e ler novamente.

O binário criado durante essa etapa é
[`projects/sd5116-cli/src/onu_led_direct.c`](../projects/sd5116-cli/src/onu_led_direct.c).
Ele contém a ABI acima e a tabela de pinos medida nesta unidade. A tabela não
deve ser copiada para outra PCB sem repetir o procedimento.

No LED LAN1, a escrita direta em GPIO 12 manteve o LED aceso mesmo após evento
de link. Isso também exigiu verificar processos de boot: `fh_bsp_led_act` e
`detectHwEvent` estavam desativados no `initialize.sh` usado no teste. A
conclusão é limitada àquele boot; reativar serviços ou módulos pode retomar o
controle do LED.

## 3. Rota I²C: helper, implementação e HAL

O símbolo local `i2c_debug` em `kdrv_debug` está em `0x9c90`; as strings já
mostravam seus quatro argumentos. A implementação efetiva, porém, não está em
`libi2c_interface.so`: os símbolos importados pelo helper são resolvidos por
`libfhdrv_kdrv_board_impl.so`.

```sh
readelf -Ws extracted/app_exB/libfhdrv_kdrv_board_impl.so | \
  rg 'fhdrv_kdrv_i2c|hi_hal_i2c'

arm-buildroot-linux-uclibcgnueabi-objdump -d \
  --start-address=0x2a28 --stop-address=0x2ed4 \
  extracted/app_exB/libfhdrv_kdrv_board_impl.so
```

Essa desmontagem expôs:

```text
fhdrv_kdrv_i2c_read  em 0x2a28
fhdrv_kdrv_i2c_write em 0x2ca8
hi_hal_i2c_attr_set
hi_hal_i2c_data_send
hi_hal_i2c_data_receive
```

### 3.1 Assinaturas recuperadas

Pela convenção ARM e pelos acessos à pilha, as funções do helper foram
reconstruídas assim:

```c
int fhdrv_kdrv_i2c_read(uint32_t channel, uint8_t address,
                        uint8_t offset, uint8_t *result,
                        uint32_t length);

int fhdrv_kdrv_i2c_write(uint32_t channel, uint8_t address,
                         uint8_t offset, const uint8_t *data,
                         uint32_t length);
```

Para uma leitura, a implementação:

1. configura o atributo do controlador;
2. envia uma mensagem de um byte contendo `offset`;
3. recebe `length` bytes no mesmo endereço;
4. copia o buffer temporário para `result`.

Para escrita, ela aloca `length + 1`, coloca `offset` no primeiro byte, copia
os dados após ele e envia o conjunto em uma transação.

As duas estruturas passadas à HAL foram obtidas pelos offsets escritos antes
das chamadas:

```c
struct i2c_attr {
    uint32_t index;
    uint32_t enable;
    uint32_t address_mode;
    uint32_t baud_rate;
};

struct i2c_packet {
    uint32_t address;
    uint8_t *data;
    uint32_t length;
    uint32_t stop;
};
```

No caminho do helper, a estrutura de atributo é preenchida com:

```text
index = channel
enable = 1
address_mode = 0
baud_rate = 0
```

Assim foi identificado que `kdrv_debug` sempre usa a seleção de 100 kHz. A
interface HAL direta foi então encapsulada em
[`libs/libfh_i2c/src/fh_i2c.c`](../libs/libfh_i2c/src/fh_i2c.c), que carrega
as bibliotecas necessárias nesta ordem:

```text
libhi_ubasic.so → libhi_ioreactor.so → libhi_ipc.so → libhi_hal.so
```

O valor de `baud_rate` não foi extrapolado por tentativa: o descritor CLI
`i2c_attr_set` da imagem limita o campo a `0..1` e documenta os valores como
100 e 400 kHz. A biblioteca expõe somente essas duas opções.

### 3.2 Validação runtime usada

O primeiro wrapper, `projects/sd5116-cli/src/onu_i2c.c`, continua usando
`/fh/extend/kdrv_debug`. Ele captura stdout/stderr, procura `hex data:` e
rejeita mensagens de erro do helper. Isso tornou a leitura reproduzível antes
de chamar a HAL diretamente.

Com a HAL direta, foram validados no hardware:

```text
I²C0, OLED externo em 0x3c, 100 kHz e 400 kHz
framebuffer SSD1306 completo: 10,00 FPS a 100 kHz; 30,00 FPS a 400 kHz
```

Os binários usados para essas verificações ficam em
[`projects/sd5116-i2c-tests`](../projects/sd5116-i2c-tests/). Eles são
transferidos para `/tmp`; não fazem alterações persistentes.

## 4. Papel das demais bibliotecas encontradas

| Biblioteca | Relação observada |
|---|---|
| `libfhdrv_kdrv_board.so` | wrappers de GPIO por ioctl e outra implementação de I²C; contém os literais de ioctl GPIO |
| `libfhdrv_kdrv_board_impl.so` | implementação I²C usada pelo `kdrv_debug` e ponte para `hi_hal_i2c_*` |
| `libi2c_interface.so` | comandos de configuração/CLI de I²C; não é a rota chamada diretamente por `kdrv_debug` |
| `libled_interface.so` | camadas de serviço e tabelas de LED por índice; não é necessária ao acesso GPIO direto |
| `libfhdrv_gpio.so` | não é ELF nesta extração; não foi usada para definir a ABI |

## 5. Como repetir em uma variante

Repita a cadeia, sem copiar endereços ou ioctls:

```text
helper disponível
  → readelf -d / -Ws
  → strings para gramática e mensagens
  → objdump da função local do helper
  → objdump do símbolo importado na biblioteca fornecedora
  → estrutura, ioctl ou pacote HAL reconstruído
  → programa C mínimo
  → teste reversível em componente físico conhecido
```

Se `kdrv_debug` não existir, procure outro helper que abra o nó proprietário.
Se o kernel expuser `/dev/gpiochip*` ou `/dev/i2c-*`, prefira a API Linux e
registre a diferença em vez de aplicar esta ABI. Se o helper, a biblioteca ou
o kernel mudarem, todos os valores acima devem ser considerados específicos da
imagem anterior até nova desmontagem e teste físico.
