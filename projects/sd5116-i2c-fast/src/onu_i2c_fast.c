/*
 * onu-i2c-fast -- experimento I2C 400 kHz para o OLED externo da SD5116.
 *
 * Baseado em estruturas confirmadas por desmontagem de
 * libfhdrv_kdrv_board_impl.so e nos descritores CLI do firmware.  O helper
 * kdrv_debug sempre restaura 100 kHz; este programa chama a HAL diretamente.
 * Nao grava MTD, arquivos nem configuracao persistente.
 */

#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HI_UBASIC "/lib/hsan/so/service/libhi_ubasic.so"
#define HI_IOREACTOR "/lib/hsan/so/service/libhi_ioreactor.so"
#define HI_IPC "/lib/hsan/so/service/libhi_ipc.so"
#define HI_HAL "/lib/hsan/so/service/libhi_hal.so"
#define OLED_ADDRESS 0x3cU
#define OLED_WIDTH 128U
#define OLED_PAGES 8U

struct i2c_attr {
    uint32_t index;
    uint32_t enable;
    uint32_t address_mode;
    uint32_t baud_rate;
};

struct i2c_send {
    uint32_t address;
    const uint8_t *data;
    uint32_t length;
    uint32_t stop;
};

typedef int (*attr_set_fn)(const struct i2c_attr *attr);
typedef int (*data_send_fn)(const struct i2c_send *data);

static volatile sig_atomic_t stop_requested;

static void stop_handler(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

static void usage(const char *program)
{
    fprintf(stderr,
            "Uso: %s wave-write --force 0 0x3c <controle> <byte>\n"
            "  ou: %s demo --force [--khz 100|300|400]\n"
            "\n"
            "Uso limitado deliberadamente ao OLED SSD1306 externo confirmado.\n"
            "400 kHz e confirmado; 300 kHz e teste experimental. Ctrl+C restaura\n"
            "100 kHz antes de sair.\n",
            program, program);
}

static int parse_u8(const char *text, unsigned long minimum,
                    unsigned long maximum, unsigned long *value)
{
    char *end;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 0);
    if (errno != 0 || *text == '\0' || *end != '\0' ||
        parsed < minimum || parsed > maximum)
        return -1;
    *value = parsed;
    return 0;
}

static int set_baud(attr_set_fn set_attr, uint32_t baud_rate)
{
    struct i2c_attr attr;

    attr.index = 0U;
    attr.enable = 1U;
    attr.address_mode = 0U;
    attr.baud_rate = baud_rate;
    return set_attr(&attr);
}

static int send_bytes(data_send_fn send_data, const uint8_t *bytes,
                      uint32_t length)
{
    struct i2c_send send;

    send.address = OLED_ADDRESS;
    send.data = bytes;
    send.length = length;
    send.stop = 1U;
    return send_data(&send);
}

static const uint8_t *glyph(char character)
{
    static const uint8_t blank[5] = {0, 0, 0, 0, 0};
    static const uint8_t a[5] = {0x7e,0x11,0x11,0x11,0x7e};
    static const uint8_t c[5] = {0x3e,0x41,0x41,0x41,0x22};
    static const uint8_t d[5] = {0x7f,0x41,0x41,0x22,0x1c};
    static const uint8_t f[5] = {0x7f,0x09,0x09,0x09,0x01};
    static const uint8_t h[5] = {0x7f,0x08,0x08,0x08,0x7f};
    static const uint8_t i[5] = {0x00,0x41,0x7f,0x41,0x00};
    static const uint8_t k[5] = {0x7f,0x08,0x14,0x22,0x41};
    static const uint8_t s[5] = {0x46,0x49,0x49,0x49,0x31};
    static const uint8_t t[5] = {0x01,0x01,0x7f,0x01,0x01};
    static const uint8_t z[5] = {0x61,0x51,0x49,0x45,0x43};
    static const uint8_t zero[5] = {0x3e,0x51,0x49,0x45,0x3e};
    static const uint8_t one[5] = {0x00,0x42,0x7f,0x40,0x00};
    static const uint8_t two[5] = {0x42,0x61,0x51,0x49,0x46};
    static const uint8_t four[5] = {0x18,0x14,0x12,0x7f,0x10};
    static const uint8_t six[5] = {0x3c,0x4a,0x49,0x49,0x30};

    switch (character) {
    case 'A': return a;
    case 'C': return c;
    case 'D': return d;
    case 'F': return f;
    case 'H': return h;
    case 'I': return i;
    case 'K': return k;
    case 'S': return s;
    case 'T': return t;
    case 'Z': return z;
    case '0': return zero;
    case '1': return one;
    case '2': return two;
    case '4': return four;
    case '6': return six;
    default: return blank;
    }
}

static void put_text(uint8_t *frame, unsigned int column,
                     unsigned int page, const char *text)
{
    while (*text != '\0' && column < 21U && page < OLED_PAGES) {
        const uint8_t *pixels = glyph(*text++);
        unsigned int offset = page * OLED_WIDTH + column * 6U;
        unsigned int index;

        for (index = 0U; index < 5U; ++index)
            frame[offset + index] = pixels[index];
        frame[offset + 5U] = 0U;
        ++column;
    }
}

static int oled_demo(data_send_fn send_data)
{
    static const uint8_t initialization[] = {
        0x00, 0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0x00, 0x40,
        0x8d, 0x14, 0x20, 0x00, 0xa1, 0xc8, 0xda, 0x12, 0x81,
        0xcf, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6, 0x2e, 0xaf
    };
    static const uint8_t window[] = {
        0x00, 0x21, 0x00, 0x7f, 0x22, 0x00, 0x07
    };
    /* O driver HAL rejeita ui_length > 128. O byte de controle ocupa um
     * desses bytes, portanto cada página é enviada como 127 + 1 pixels. */
    uint8_t page[OLED_WIDTH];
    uint8_t tail[2];
    uint8_t frame[OLED_WIDTH * OLED_PAGES];
    unsigned int page_number;
    unsigned int column;

    if (send_bytes(send_data, initialization, sizeof(initialization)) != 0 ||
        send_bytes(send_data, window, sizeof(window)) != 0)
        return -1;

    memset(frame, 0, sizeof(frame));
    put_text(frame, 6U, 1U, "FAST I2C");
    put_text(frame, 7U, 3U, "SD5116");
    put_text(frame, 7U, 5U, "400 KHZ");

    for (page_number = 0U; page_number < OLED_PAGES; ++page_number) {
        page[0] = 0x40U;
        for (column = 0U; column < OLED_WIDTH - 1U; ++column)
            page[column + 1U] = frame[page_number * OLED_WIDTH + column];
        tail[0] = 0x40U;
        tail[1] = frame[page_number * OLED_WIDTH + OLED_WIDTH - 1U];
        if (send_bytes(send_data, page, sizeof(page)) != 0 ||
            send_bytes(send_data, tail, sizeof(tail)) != 0)
            return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    void *ioreactor;
    void *ubasic;
    void *ipc;
    void *hal;
    attr_set_fn set_attr;
    data_send_fn send_data;
    unsigned long control;
    unsigned long value;
    unsigned long requested_khz = 400U;
    int demo_requested;
    uint8_t bytes[2];
    unsigned long transactions = 0U;
    int result = 1;

    demo_requested = (argc == 3 && strcmp(argv[1], "demo") == 0 &&
                      strcmp(argv[2], "--force") == 0) ||
                     (argc == 5 && strcmp(argv[1], "demo") == 0 &&
                      strcmp(argv[2], "--force") == 0 &&
                      strcmp(argv[3], "--khz") == 0 &&
                      parse_u8(argv[4], 100U, 400U, &requested_khz) == 0 &&
                      (requested_khz == 100U || requested_khz == 300U ||
                       requested_khz == 400U));

    if (!(demo_requested ||
          (argc == 7 && strcmp(argv[1], "wave-write") == 0 &&
           strcmp(argv[2], "--force") == 0 && strcmp(argv[3], "0") == 0 &&
           strcmp(argv[4], "0x3c") == 0 &&
           parse_u8(argv[5], 0U, 255U, &control) == 0 &&
           parse_u8(argv[6], 0U, 255U, &value) == 0))) {
        usage(argv[0]);
        return 2;
    }

    /* Carregar a cadeia por caminho absoluto evita depender de
     * LD_LIBRARY_PATH no shell root. kdrv_debug declara esta mesma cadeia. */
    ubasic = dlopen(HI_UBASIC, RTLD_NOW | RTLD_GLOBAL);
    if (ubasic == NULL) {
        fprintf(stderr, "nao carregou %s: %s\n", HI_UBASIC, dlerror());
        return 1;
    }
    ioreactor = dlopen(HI_IOREACTOR, RTLD_NOW | RTLD_GLOBAL);
    if (ioreactor == NULL) {
        fprintf(stderr, "nao carregou %s: %s\n", HI_IOREACTOR, dlerror());
        (void)dlclose(ubasic);
        return 1;
    }
    ipc = dlopen(HI_IPC, RTLD_NOW | RTLD_GLOBAL);
    if (ipc == NULL) {
        fprintf(stderr, "nao carregou %s: %s\n", HI_IPC, dlerror());
        (void)dlclose(ubasic);
        (void)dlclose(ioreactor);
        return 1;
    }
    hal = dlopen(HI_HAL, RTLD_NOW | RTLD_GLOBAL);
    if (hal == NULL) {
        fprintf(stderr, "nao carregou %s: %s\n", HI_HAL, dlerror());
        (void)dlclose(ipc);
        (void)dlclose(ubasic);
        (void)dlclose(ioreactor);
        return 1;
    }

    set_attr = (attr_set_fn)dlsym(hal, "hi_hal_i2c_attr_set");
    send_data = (data_send_fn)dlsym(hal, "hi_hal_i2c_data_send");
    if (set_attr == NULL || send_data == NULL) {
        fprintf(stderr, "simbolo HAL I2C ausente: %s\n", dlerror());
        goto out;
    }

    if (set_baud(set_attr, requested_khz == 100U ? 0U :
                 requested_khz == 400U ? 1U : (uint32_t)requested_khz) != 0) {
        fprintf(stderr, "falha ao solicitar I2C0 em %lu kHz\n", requested_khz);
        goto out;
    }

    if (demo_requested) {
        fprintf(stderr, "I2C0 solicitado em %lu kHz; enviando demo SSD1306 em 18 transacoes.\n",
                requested_khz);
        if (oled_demo(send_data) != 0)
            fprintf(stderr, "falha HAL durante demo SSD1306\n");
        else {
            fprintf(stderr, "demo concluido; restaurando 100 kHz.\n");
            result = 0;
        }
        goto restore;
    }

    bytes[0] = (uint8_t)control;
    bytes[1] = (uint8_t)value;

    if (signal(SIGINT, stop_handler) == SIG_ERR ||
        signal(SIGTERM, stop_handler) == SIG_ERR) {
        perror("signal");
        goto restore;
    }

    fprintf(stderr, "I2C0 em 400 kHz; escrita direta repetida para OLED 0x3c. Ctrl+C restaura 100 kHz.\n");
    while (!stop_requested) {
        if (send_bytes(send_data, bytes, sizeof(bytes)) != 0) {
            fprintf(stderr, "falha HAL apos %lu transacoes\n", transactions);
            goto restore;
        }
        ++transactions;
    }
    fprintf(stderr, "interrompido apos %lu transacoes.\n", transactions);
    result = 0;

restore:
    if (set_baud(set_attr, 0U) != 0) {
        fprintf(stderr, "AVISO: nao foi possivel restaurar I2C0 para 100 kHz\n");
        result = 1;
    }
out:
    (void)dlclose(hal);
    (void)dlclose(ipc);
    (void)dlclose(ubasic);
    (void)dlclose(ioreactor);
    return result;
}
