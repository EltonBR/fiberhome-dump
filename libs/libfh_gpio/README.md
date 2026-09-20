# libfh_gpio

Biblioteca de acesso direto a GPIO pela ABI confirmada de
`/dev/fhdrv_kdrv_board`. Ela encapsula mux, direção, escrita e leitura; não
usa `kdrv_debug` nem bibliotecas proprietárias de userspace.

## API e ciclo de vida

1. Declare `fh_gpio_t`.
2. Abra com `fh_gpio_open()`.
3. Para cada pino, chame `fh_gpio_mode()` antes de ler/escrever.
4. Feche com `fh_gpio_close()`.

`output=1` configura saída; `output=0`, entrada. `fh_gpio_mode()` sempre
executa a configuração de mux antes da direção.

```c
#include "fh_gpio.h"

int main(void)
{
    fh_gpio_t gpio;

    if (fh_gpio_open(&gpio) != 0)
        return 1;
    if (fh_gpio_mode(&gpio, PIN_LED_PON, GPIO_OUT) != 0)
        return 1;
    /* LED PON é ativo-baixo: nível 0 acende. */
    (void)fh_gpio_write(&gpio, PIN_LED_PON, GPIO_LED_ON);
    fh_gpio_close(&gpio);
    return 0;
}
```

### Leitura de botão

```c
uint8_t level;
fh_gpio_mode(&gpio, PIN_BUTTON_LED, GPIO_IN);
fh_gpio_read(&gpio, PIN_BUTTON_LED, &level);
/* GPIO_LOW = botão LED pressionado; GPIO_HIGH = solto. */
```

## Pinos confirmados

| Função | GPIO | Polaridade |
|---|---:|---|
| PON | 31 | LED ativo-baixo |
| LOS | 30 | LED ativo-baixo |
| PHONE/VoIP | 6 | LED ativo-baixo |
| LAN1 | 12 | LED ativo-baixo |
| LAN2 | 13 | LED ativo-baixo |
| botão LED | 14 | pressionado em baixo |
| Reset | 32 | pressionado em baixo |

Pull-up/pull-down interno é **DESCONHECIDO** e não é configurável pela API.
Não use GPIO32 como saída e não suponha exclusividade de LEDs se serviços
proprietários forem reativados.

## Compilação

```sh
make -C libs/libfh_gpio
```

Ao ligar uma aplicação: `-Ilibs/libfh_gpio/include` e
`libs/libfh_gpio/lib/libfh_gpio.a`.
