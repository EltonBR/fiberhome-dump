/* Mostra a taxa I2C selecionada em caracteres ampliados no OLED SSD1306. */
#include "fh_i2c.h"
#include "fh_i2c_lcd.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define GLYPH_WIDTH 5U
#define GLYPH_HEIGHT 7U
#define GLYPH_SCALE_X 3U
#define GLYPH_SCALE_Y 5U
#define TEXT_X 8U
#define TEXT_Y 14U
#define ROTATE_SCALE_X 2U
#define ROTATE_SCALE_Y 3U
#define ROTATE_STEPS 16U

static const uint8_t *glyph(char c)
{
    static const uint8_t blank[GLYPH_HEIGHT] = {0, 0, 0, 0, 0, 0, 0};
    static const uint8_t digits[10][GLYPH_HEIGHT] = {
        {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e},
        {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e},
        {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f},
        {0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e},
        {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02},
        {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e},
        {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e},
        {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
        {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e},
        {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c}
    };
    static const uint8_t k[GLYPH_HEIGHT] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
    static const uint8_t h[GLYPH_HEIGHT] = {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11};
    static const uint8_t z[GLYPH_HEIGHT] = {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f};

    if (c >= '0' && c <= '9')
        return digits[(unsigned int)(c - '0')];
    if (c == 'K')
        return k;
    if (c == 'H')
        return h;
    if (c == 'Z')
        return z;
    return blank;
}

static void draw_text(fh_i2c_lcd_t *lcd, const char *text)
{
    unsigned int column = TEXT_X;

    while (*text != '\0') {
        const uint8_t *rows = glyph(*text++);
        unsigned int row;

        for (row = 0; row < GLYPH_HEIGHT; ++row) {
            unsigned int bit;
            for (bit = 0; bit < GLYPH_WIDTH; ++bit) {
                unsigned int dy;
                unsigned int dx;

                if ((rows[row] & (1U << (GLYPH_WIDTH - 1U - bit))) == 0U)
                    continue;
                for (dy = 0; dy < GLYPH_SCALE_Y; ++dy)
                    for (dx = 0; dx < GLYPH_SCALE_X; ++dx)
                        fh_i2c_lcd_pixel(lcd,
                                         column + bit * GLYPH_SCALE_X + dx,
                                         TEXT_Y + row * GLYPH_SCALE_Y + dy,
                                         1);
            }
        }
        column += (GLYPH_WIDTH + 1U) * GLYPH_SCALE_X;
    }
}

/* seno e cosseno em ponto fixo, escala 1024; não requer libm no alvo. */
static const int cosine[ROTATE_STEPS] = {
    1024, 946, 724, 392, 0, -392, -724, -946,
    -1024, -946, -724, -392, 0, 392, 724, 946
};
static const int sine[ROTATE_STEPS] = {
    0, 392, 724, 946, 1024, 946, 724, 392,
    0, -392, -724, -946, -1024, -946, -724, -392
};

static void draw_rotated_text(fh_i2c_lcd_t *lcd, const char *text,
                              unsigned int step)
{
    const int text_width = 6 * (int)(GLYPH_WIDTH + 1U) * (int)ROTATE_SCALE_X;
    const int text_height = (int)GLYPH_HEIGHT * (int)ROTATE_SCALE_Y;
    const int center_x = (int)LCD_WIDTH / 2;
    const int center_y = (int)LCD_HEIGHT / 2;
    const int cs = cosine[step % ROTATE_STEPS];
    const int sn = sine[step % ROTATE_STEPS];
    unsigned int character = 0;

    while (*text != '\0') {
        const uint8_t *rows = glyph(*text++);
        unsigned int row;

        for (row = 0; row < GLYPH_HEIGHT; ++row) {
            unsigned int bit;
            for (bit = 0; bit < GLYPH_WIDTH; ++bit) {
                unsigned int dy;
                unsigned int dx;

                if ((rows[row] & (1U << (GLYPH_WIDTH - 1U - bit))) == 0U)
                    continue;
                for (dy = 0; dy < ROTATE_SCALE_Y; ++dy) {
                    for (dx = 0; dx < ROTATE_SCALE_X; ++dx) {
                        const int x = (int)(character * (GLYPH_WIDTH + 1U) * ROTATE_SCALE_X + bit * ROTATE_SCALE_X + dx) - text_width / 2;
                        const int y = (int)(row * ROTATE_SCALE_Y + dy) - text_height / 2;
                        const int rotated_x = center_x + (x * cs - y * sn) / 1024;
                        const int rotated_y = center_y + (x * sn + y * cs) / 1024;

                        if (rotated_x >= 0 && rotated_x < (int)LCD_WIDTH &&
                            rotated_y >= 0 && rotated_y < (int)LCD_HEIGHT)
                            fh_i2c_lcd_pixel(lcd, (unsigned int)rotated_x,
                                             (unsigned int)rotated_y, 1);
                    }
                }
            }
        }
        ++character;
    }
}

static long milliseconds(void)
{
    struct timeval now;

    (void)gettimeofday(&now, NULL);
    /* long tem 32 bits na uClibc do alvo. A hora absoluta em milissegundos
     * estoura; a janela de uma hora abaixo é suficiente para este benchmark. */
    return (now.tv_sec % 3600L) * 1000L + now.tv_usec / 1000L;
}

static int spin_test(fh_i2c_lcd_t *lcd, unsigned long seconds)
{
    const long started = milliseconds();
    long last_report = started;
    unsigned long total_frames = 0;
    unsigned long report_frames = 0;
    unsigned int step = 0;

    printf("Teste de rotacao a 400 kHz por %lu s.\n", seconds);
    while (milliseconds() - started < (long)(seconds * 1000UL)) {
        const long frame_started = milliseconds();

        fh_i2c_lcd_clear(lcd);
        draw_rotated_text(lcd, "400KHZ", step++);
        if (fh_i2c_lcd_present(lcd) != 0)
            return -1;
        ++total_frames;
        ++report_frames;

        if (milliseconds() - last_report >= 1000L) {
            const long elapsed = milliseconds() - last_report;
            printf("FPS: %lu (%lu quadros em %ld ms)\n",
                   report_frames * 1000UL / (unsigned long)elapsed,
                   report_frames, elapsed);
            (void)fflush(stdout);
            report_frames = 0;
            last_report = milliseconds();
        }
        (void)frame_started;
    }
    printf("TESTE_CONCLUIDO quadros=%lu duracao_ms=%ld fps_medio=%lu\n",
           total_frames, milliseconds() - started,
           total_frames * 1000UL / (unsigned long)(milliseconds() - started));
    (void)fflush(stdout);
    return 0;
}

int main(int argc, char **argv)
{
    fh_i2c_t bus;
    fh_i2c_lcd_t lcd;
    unsigned long khz;
    unsigned long seconds = 0UL;
    const char *label;

    if (argc != 2 && argc != 3) {
        fputs("Uso: onu-i2c-oled-speed-test 100|400 | spin [segundos]\n", stderr);
        return 2;
    }
    if (argv[1][0] == 's') {
        seconds = argc == 3 ? strtoul(argv[2], NULL, 10) : 10UL;

        if (argc > 3 || seconds == 0UL || seconds > 120UL) {
            fputs("ERRO: spin aceita de 1 a 120 segundos.\n", stderr);
            return 2;
        }
        khz = I2C_SPEED_400KHZ;
        label = NULL;
    } else {
    khz = strtoul(argv[1], NULL, 10);
    if (khz == I2C_SPEED_100KHZ)
        label = "100KHZ";
    else if (khz == I2C_SPEED_400KHZ)
        label = "400KHZ";
    else {
        fputs("ERRO: a HAL desta ONU documenta somente 100 ou 400 kHz.\n", stderr);
        return 2;
    }
    }

    if (fh_i2c_open(&bus) != 0 ||
        fh_i2c_speed(&bus, (unsigned int)khz) != 0 ||
        fh_i2c_lcd_init(&lcd, &bus, LCD_DEFAULT_ADDRESS) != 0) {
        fputs("ERRO: falha ao preparar I2C/OLED.\n", stderr);
        fh_i2c_close(&bus);
        return 1;
    }

    if (label == NULL) {
        int rc = spin_test(&lcd, seconds);

        fh_i2c_close(&bus);
        return rc == 0 ? 0 : 1;
    }

    fh_i2c_lcd_clear(&lcd);
    draw_text(&lcd, label);
    if (fh_i2c_lcd_present(&lcd) != 0) {
        fputs("ERRO: falha ao atualizar o OLED.\n", stderr);
        fh_i2c_close(&bus);
        return 1;
    }

    printf("OLED atualizado em %lu kHz: %s\n", khz, label);
    fh_i2c_close(&bus);
    return 0;
}
