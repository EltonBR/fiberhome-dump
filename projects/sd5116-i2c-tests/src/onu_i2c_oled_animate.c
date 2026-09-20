/* Animação visual conservadora para SSD1306 externo, I2C0 a 100/400 kHz. */
#define _XOPEN_SOURCE 500
#include "fh_i2c.h"
#include "fh_i2c_lcd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define GLYPH_WIDTH 5U
#define GLYPH_HEIGHT 7U
#define SCALE_X 3U
#define SCALE_Y 5U
#define FRAMES 30U

static const uint8_t *glyph(char character)
{
    static const uint8_t blank[GLYPH_HEIGHT] = {0, 0, 0, 0, 0, 0, 0};
    static const uint8_t zero[GLYPH_HEIGHT] = {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e};
    static const uint8_t one[GLYPH_HEIGHT] = {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e};
    static const uint8_t four[GLYPH_HEIGHT] = {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02};
    static const uint8_t k[GLYPH_HEIGHT] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};

    if (character == '0')
        return zero;
    if (character == '1')
        return one;
    if (character == '4')
        return four;
    if (character == 'K')
        return k;
    return blank;
}

static void draw_border(fh_i2c_lcd_t *lcd)
{
    unsigned int coordinate;

    for (coordinate = 0; coordinate < LCD_WIDTH; ++coordinate) {
        fh_i2c_lcd_pixel(lcd, coordinate, 0U, 1);
        fh_i2c_lcd_pixel(lcd, coordinate, LCD_HEIGHT - 1U, 1);
    }
    for (coordinate = 0; coordinate < LCD_HEIGHT; ++coordinate) {
        fh_i2c_lcd_pixel(lcd, 0U, coordinate, 1);
        fh_i2c_lcd_pixel(lcd, LCD_WIDTH - 1U, coordinate, 1);
    }
}

static void draw_text(fh_i2c_lcd_t *lcd, unsigned int start_x, const char *text)
{
    unsigned int character;

    for (character = 0U; text[character] != '\0'; ++character) {
        const uint8_t *rows = glyph(text[character]);
        unsigned int row;

        for (row = 0; row < GLYPH_HEIGHT; ++row) {
            unsigned int bit;
            for (bit = 0; bit < GLYPH_WIDTH; ++bit) {
                unsigned int dy;
                unsigned int dx;

                if ((rows[row] & (1U << (GLYPH_WIDTH - 1U - bit))) == 0U)
                    continue;
                for (dy = 0; dy < SCALE_Y; ++dy)
                    for (dx = 0; dx < SCALE_X; ++dx)
                        fh_i2c_lcd_pixel(lcd,
                                         start_x + character * 18U + bit * SCALE_X + dx,
                                         14U + row * SCALE_Y + dy, 1);
            }
        }
    }
}

static unsigned long elapsed_microseconds(const struct timeval *start,
                                          const struct timeval *end)
{
    return (unsigned long)(end->tv_sec - start->tv_sec) * 1000000UL +
           (unsigned long)(end->tv_usec - start->tv_usec);
}

int main(int argc, char **argv)
{
    fh_i2c_t bus;
    fh_i2c_lcd_t lcd;
    struct timeval started;
    struct timeval finished;
    unsigned long khz;
    unsigned long delay_us = 150000UL;
    const char *label;
    const char *mode = "animacao";
    unsigned int frame;

    if (argc != 2 && argc != 3) {
        fputs("Uso: onu-i2c-oled-animate 100|400 [--max]\n", stderr);
        return 2;
    }
    if (argc == 3) {
        if (strcmp(argv[2], "--max") != 0) {
            fputs("ERRO: unica opcao adicional e --max.\n", stderr);
            return 2;
        }
        delay_us = 0UL;
        mode = "maximo";
    }
    khz = strtoul(argv[1], NULL, 10);
    if (khz == I2C_SPEED_100KHZ)
        label = "100K";
    else if (khz == I2C_SPEED_400KHZ)
        label = "400K";
    else {
        fputs("ERRO: use 100 ou 400 kHz.\n", stderr);
        return 2;
    }

    if (fh_i2c_open(&bus) != 0 ||
        fh_i2c_speed(&bus, (unsigned int)khz) != 0 ||
        fh_i2c_lcd_init(&lcd, &bus, LCD_DEFAULT_ADDRESS) != 0) {
        fputs("ERRO: nao foi possivel iniciar OLED.\n", stderr);
        fh_i2c_close(&bus);
        return 1;
    }

    (void)gettimeofday(&started, NULL);
    for (frame = 0; frame < FRAMES; ++frame) {
        const unsigned int position = 4U + (frame <= 15U ? frame : 30U - frame) * 3U;

        fh_i2c_lcd_clear(&lcd);
        draw_border(&lcd);
        draw_text(&lcd, position, label);
        if (fh_i2c_lcd_present(&lcd) != 0) {
            fputs("ERRO: falha no quadro I2C.\n", stderr);
            fh_i2c_close(&bus);
            return 1;
        }
        if (delay_us != 0UL)
            (void)usleep((useconds_t)delay_us);
    }

    (void)gettimeofday(&finished, NULL);
    {
        const unsigned long elapsed = elapsed_microseconds(&started, &finished);
        const unsigned long fps_x100 = (unsigned long)FRAMES * 100000000UL / elapsed;

        printf("ANIMACAO_OK modo=%s clock=%lukHz quadros=%u tempo_ms=%lu fps=%lu.%02lu\n",
               mode, khz, FRAMES, elapsed / 1000UL,
               fps_x100 / 100UL, fps_x100 % 100UL);
    }
    fh_i2c_close(&bus);
    return 0;
}
