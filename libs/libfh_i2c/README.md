# libfh_i2c

Biblioteca C para I²C0 pela HAL dinâmica da SD5116. Ela fornece transporte
genérico: não conhece endereços, registros ou protocolos de periféricos.

## API

| Função | Uso | Retorno |
|---|---|---|
| `fh_i2c_open(&bus)` | carrega a cadeia de bibliotecas HAL e prepara `bus` | `0` ou `-1` |
| `fh_i2c_speed(&bus, velocidade)` | seleciona `I2C_SPEED_100KHZ` ou `I2C_SPEED_400KHZ` | `0` ou `-1` |
| `fh_i2c_send(&bus, addr, dados, tamanho)` | envia uma transação I²C | `0` ou `-1` |
| `fh_i2c_receive(&bus, addr, dados, tamanho)` | recebe uma transação I²C | `0` ou `-1` |
| `fh_i2c_close(&bus)` | libera os handles da HAL | nenhum |

`addr` é um endereço I²C de sete bits. `tamanho` deve estar entre 1 e
`I2C_MAX_TRANSFER` (128). Cada chamada de envio ou recepção termina com STOP.
`fh_i2c_close()` não reconfigura a velocidade do controlador.

## Exemplo completo: escrita

```c
#include "fh_i2c.h"

int main(void)
{
    fh_i2c_t bus;
    uint8_t bytes[] = { 0x00, 0x55 };

    if (fh_i2c_open(&bus) != 0)
        return 1;
    if (fh_i2c_speed(&bus, I2C_SPEED_400KHZ) != 0)
        return 1;
    if (fh_i2c_send(&bus, 0x20U, bytes, sizeof(bytes)) != 0)
        return 1;
    fh_i2c_close(&bus);
    return 0;
}
```

O formato de `bytes` é definido pelo periférico. A biblioteca não acrescenta
byte de controle, offset nem checksum.

## Exemplo completo: leitura após offset

```c
#include "fh_i2c.h"

int main(void)
{
    fh_i2c_t bus;
    uint8_t offset = 0x10;
    uint8_t data[16];

    if (fh_i2c_open(&bus) != 0)
        return 1;
    if (fh_i2c_speed(&bus, I2C_SPEED_100KHZ) != 0)
        return 1;
    if (fh_i2c_send(&bus, 0x50U, &offset, sizeof(offset)) != 0)
        return 1;
    if (fh_i2c_receive(&bus, 0x50U, data, sizeof(data)) != 0)
        return 1;
    fh_i2c_close(&bus);
    return 0;
}
```

Esse padrão de offset seguido de leitura serve apenas para dispositivos que o
definem em seu próprio protocolo.

Compile a biblioteca com `make -C libs/libfh_i2c`. Ao ligar um binário do alvo,
use `libfh_i2c.a` e `libdl.so.0` do rootfs.
