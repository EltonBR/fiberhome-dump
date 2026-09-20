/* Recuperação visual conservadora do SSD1306 externo. Não escreve flash. */
#define _XOPEN_SOURCE 500
#include "fh_i2c.h"
#include "fh_i2c_lcd.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    static const uint8_t display_off[] = {0x00, 0xae};
    fh_i2c_t bus;
    fh_i2c_lcd_t lcd;
    unsigned int x;
    unsigned int y;

    if (fh_i2c_open(&bus) != 0 ||
        fh_i2c_speed(&bus, I2C_SPEED_100KHZ) != 0) {
        fputs("ERRO: HAL I2C indisponivel.\n", stderr);
        fh_i2c_close(&bus);
        return 1;
    }

    /* Sai de qualquer estado parcial antes da sequência completa da lib. */
    if (fh_i2c_send(&bus, LCD_DEFAULT_ADDRESS,
                    display_off, sizeof(display_off)) != 0) {
        fputs("ERRO: OLED nao aceitou DISPLAYOFF.\n", stderr);
        fh_i2c_close(&bus);
        return 1;
    }
    (void)usleep(50000U);

    if (fh_i2c_lcd_init(&lcd, &bus, LCD_DEFAULT_ADDRESS) != 0) {
        fputs("ERRO: sequencia de inicializacao recusada.\n", stderr);
        fh_i2c_close(&bus);
        return 1;
    }

    fh_i2c_lcd_clear(&lcd);
    for (y = 0; y < LCD_HEIGHT; ++y)
        for (x = 0; x < LCD_WIDTH; ++x)
            fh_i2c_lcd_pixel(&lcd, x, y, ((x + y) & 1U) == 0U);

    if (fh_i2c_lcd_present(&lcd) != 0) {
        fputs("ERRO: nao foi possivel gravar o padrao.\n", stderr);
        fh_i2c_close(&bus);
        return 1;
    }

    puts("RECOVERY_OK I2C0=100kHz padrao=xadrez");
    fh_i2c_close(&bus);
    return 0;
}
