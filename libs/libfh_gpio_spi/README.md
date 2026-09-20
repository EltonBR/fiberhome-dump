# libfh_gpio_spi

SPI software por bit-banging, MSB primeiro, sobre `libfh_gpio`. Suporta os
modos 0–3 e CS ativo-baixo. É uma biblioteca de experimento: não representa
um controlador SPI externo dedicado.

## Controle de clock SPI

`delay_us` é a pausa solicitada entre mudanças de borda. Não é frequência
exata: cada chamada ioctl de GPIO acrescenta latência variável do kernel.
Use os valores abaixo como ponto de partida e meça SCLK no osciloscópio.

| `delay_us` | Uso recomendado |
|---:|---|
| 100 | primeiro teste, muito conservador |
| 10 | teste após validar sinais |
| 1 | somente após medir estabilidade |
| 0 | sem pausa intencional; frequência indefinida |

Uma aproximação ideal, que **não inclui ioctl**, é `f ≈ 1 / (2 × delay_us)`.
Por exemplo, 100 µs sugeriria 5 kHz, mas a frequência real será menor.

## Exemplo: JEDEC dummy

```c
#include "fh_gpio_spi.h"

int main(void)
{
    fh_gpio_spi_t spi;
    uint8_t tx[] = {0x9f, 0x00, 0x00, 0x00};
    uint8_t rx[sizeof(tx)];

    if (fh_gpio_spi_open(&spi,
                         SPI_DEFAULT_CS,
                         SPI_DEFAULT_SCLK,
                         SPI_DEFAULT_MOSI,
                         SPI_DEFAULT_MISO,
                         SPI_MODE_0,
                         SPI_DELAY_SAFE_US) != 0)
        return 1;
    if (fh_gpio_spi_transfer(&spi, tx, rx, sizeof(tx)) != 0)
        return 1;
    fh_gpio_spi_close(&spi);
    return 0;
}
```

Parâmetros de `fh_gpio_spi_open()`:

| Ordem | Constante padrão | Significado |
|---:|---|---|
| 1 | `SPI_DEFAULT_CS` | seleção do periférico, ativa em `GPIO_LOW` |
| 2 | `SPI_DEFAULT_SCLK` | clock serial gerado por software |
| 3 | `SPI_DEFAULT_MOSI` | dados da ONU para o periférico |
| 4 | `SPI_DEFAULT_MISO` | dados do periférico para a ONU |
| 5 | `SPI_MODE_0` | modo CPOL/CPHA; há `SPI_MODE_0` até `SPI_MODE_3` |
| 6 | `SPI_DELAY_SAFE_US` | atraso mínimo solicitado entre bordas |

`fh_gpio_spi_transfer()` mantém CS baixo durante todo o buffer.

## Mapeamento padrão e segurança

`CS=6`, `SCLK=31`, `MOSI=30`, `MISO=12`. Esses GPIOs estão ligados a LEDs
ativos-baixos; a transferência dummy foi validada pelos LEDs, mas os sinais
não devem ser conectados diretamente a periféricos externos sem isolar LEDs e
resistores. Não usar para flash interna, óptica, SLIC/FXS ou BOSA.

Compile com `make -C libs/libfh_gpio_spi`; ao ligar, inclua as duas bibliotecas
na ordem: `libfh_gpio_spi.a libfh_gpio.a`.
