# libfh_i2c_lcd

Framebuffer para controladores SSD1306 128×64 em I²C, construído sobre
`libfh_i2c`.

## API

| Função | Efeito |
|---|---|
| `fh_i2c_lcd_init(&lcd, &bus, endereco)` | inicializa o controlador e zera o framebuffer local |
| `fh_i2c_lcd_clear(&lcd)` | limpa somente o framebuffer local |
| `fh_i2c_lcd_pixel(&lcd, x, y, ligado)` | altera um pixel no framebuffer |
| `fh_i2c_lcd_present(&lcd)` | transmite os 1024 bytes para o display |

`LCD_DEFAULT_ADDRESS` é `0x3c`, mas `fh_i2c_lcd_init()` aceita qualquer
endereço de sete bits. `fh_i2c_lcd_present()` divide cada página em mensagens
de 128 e 2 bytes para respeitar `I2C_MAX_TRANSFER`.

## Exemplo completo

```c
#include "fh_i2c.h"
#include "fh_i2c_lcd.h"

int main(void)
{
    fh_i2c_t bus;
    fh_i2c_lcd_t lcd;
    unsigned int x;

    if (fh_i2c_open(&bus) != 0 ||
        fh_i2c_speed(&bus, I2C_SPEED_400KHZ) != 0 ||
        fh_i2c_lcd_init(&lcd, &bus, LCD_DEFAULT_ADDRESS) != 0)
        return 1;

    fh_i2c_lcd_clear(&lcd);
    for (x = 0; x < LCD_WIDTH; ++x)
        fh_i2c_lcd_pixel(&lcd, x, x / 2U, 1);
    if (fh_i2c_lcd_present(&lcd) != 0)
        return 1;
    fh_i2c_close(&bus);
    return 0;
}
```
