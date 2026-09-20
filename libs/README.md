# Bibliotecas SD5116

Ordem de dependência:

```text
libfh_gpio → libfh_gpio_spi
libfh_i2c  → libfh_i2c_lcd
```

São bibliotecas estáticas para a toolchain ARM/uClibc do repositório. Não
escrevem MTD, firmware nem configuração persistente.

- `libfh_gpio`: ABI `/dev/fhdrv_kdrv_board` confirmada.
- `libfh_gpio_spi`: SPI software sobre GPIO; experimental.
- `libfh_i2c`: HAL direta I²C, com 100/400 kHz.
- `libfh_i2c_lcd`: framebuffer SSD1306 128×64 sobre `libfh_i2c`.
