/* SPI por bit-banging nos GPIOs de LED da FiberHome SD5116. */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define BOARD_DEVICE "/dev/fhdrv_kdrv_board"
#define MUX 0x40085300UL
#define MODE 0x40085301UL
#define WRITE 0x40085303UL
#define READ 0xc0085304UL
#define MAX_BYTES 64U
#define DEFAULT_CS   6U
#define DEFAULT_SCLK 31U
#define DEFAULT_MOSI 30U
#define DEFAULT_MISO 12U

struct gpio_request { uint32_t pin; uint8_t level, debug, reserved[2]; };

static void usage(const char *p)
{
    fprintf(stderr, "Uso:\n  %s info\n  %s xfer --force <modo> <atraso-us> <cs> <sclk> <mosi> <miso> <byte> [byte...]\n", p, p);
    fprintf(stderr, "  %s xfer-default --force <modo> <atraso-us> <byte> [byte...]\n", p);
}

static int number(const char *text, unsigned long min, unsigned long max,
                  unsigned long *result)
{
    char *end;
    unsigned long value;
    errno = 0;
    value = strtoul(text, &end, 0);
    if (text == end || *end != '\0' || errno != 0 || value < min || value > max)
        return -1;
    *result = value;
    return 0;
}

static int request(int fd, unsigned long command, uint32_t pin, uint8_t *level)
{
    struct gpio_request arg;
    memset(&arg, 0, sizeof(arg));
    arg.pin = pin;
    arg.level = *level;
    if (ioctl(fd, command, &arg) < 0) { perror("ioctl GPIO"); return -1; }
    *level = arg.level;
    return 0;
}

static int prepare(int fd, uint32_t pin, int output)
{
    uint8_t value = 0U;
    if (request(fd, MUX, pin, &value) != 0) return -1;
    value = output ? 1U : 0U;
    return request(fd, MODE, pin, &value);
}

static int write_gpio(int fd, uint32_t pin, uint8_t value)
{ return request(fd, WRITE, pin, &value); }

static int read_gpio(int fd, uint32_t pin, uint8_t *value)
{ return request(fd, READ, pin, value); }

static void pause_us(unsigned long value)
{
    struct timespec delay;
    if (value == 0UL) return;
    delay.tv_sec = (time_t)(value / 1000000UL);
    delay.tv_nsec = (long)((value % 1000000UL) * 1000UL);
    (void)nanosleep(&delay, NULL);
}

/* Todos os GPIOs abaixo são LEDs ativos-baixos: alto os deixa apagados. */
static void restore(int fd, uint32_t cs, uint32_t clk, uint32_t mosi, uint32_t miso)
{
    uint8_t high = 1U;
    (void)prepare(fd, cs, 1);   (void)write_gpio(fd, cs, high);
    (void)prepare(fd, clk, 1);  (void)write_gpio(fd, clk, high);
    (void)prepare(fd, mosi, 1); (void)write_gpio(fd, mosi, high);
    (void)prepare(fd, miso, 1); (void)write_gpio(fd, miso, high);
}

static int byte_xfer(int fd, uint32_t clk, uint32_t mosi, uint32_t miso,
                     unsigned int cpol, unsigned int cpha, unsigned long delay,
                     uint8_t tx, uint8_t *rx)
{
    unsigned int bit;
    uint8_t data;
    uint8_t leading = cpol == 0U ? 1U : 0U;
    uint8_t trailing = cpol == 0U ? 0U : 1U;
    *rx = 0U;
    for (bit = 0U; bit < 8U; ++bit) {
        data = (uint8_t)((tx & (uint8_t)(0x80U >> bit)) != 0U);
        if (cpha == 0U) {
            if (write_gpio(fd, mosi, data)) return -1;
            pause_us(delay);
            if (write_gpio(fd, clk, leading)) return -1;
            pause_us(delay); data = 0U;
            if (read_gpio(fd, miso, &data) || write_gpio(fd, clk, trailing)) return -1;
        } else {
            if (write_gpio(fd, clk, leading)) return -1;
            pause_us(delay);
            if (write_gpio(fd, mosi, data)) return -1;
            pause_us(delay);
            if (write_gpio(fd, clk, trailing)) return -1;
            pause_us(delay); data = 0U;
            if (read_gpio(fd, miso, &data)) return -1;
        }
        *rx = (uint8_t)((*rx << 1) | (data != 0U ? 1U : 0U));
        pause_us(delay);
    }
    return 0;
}

static int transfer(uint32_t cs, uint32_t clk, uint32_t mosi, uint32_t miso,
                    unsigned int mode, unsigned long delay,
                    const uint8_t *tx, uint8_t *rx, size_t count)
{
    uint8_t idle = (mode & 2U) == 0U ? 0U : 1U;
    size_t i;
    int fd = open(BOARD_DEVICE, O_RDWR);
    int ret = -1;
    if (fd < 0) { perror(BOARD_DEVICE); return -1; }
    if (prepare(fd, cs, 1) || prepare(fd, clk, 1) || prepare(fd, mosi, 1) ||
        prepare(fd, miso, 0) || write_gpio(fd, cs, 1U) || write_gpio(fd, clk, idle))
        goto done;
    pause_us(delay);
    if (write_gpio(fd, cs, 0U)) goto done;
    for (i = 0U; i < count; ++i)
        if (byte_xfer(fd, clk, mosi, miso, mode >> 1, mode & 1U, delay, tx[i], &rx[i]))
            goto done;
    if (write_gpio(fd, cs, 1U)) goto done;
    ret = 0;
done:
    restore(fd, cs, clk, mosi, miso);
    (void)close(fd);
    return ret;
}

static void info(void)
{
    puts("SPI GPIO direto, CS ativo-baixo, modos 0-3, MSB primeiro.");
    puts("Padrao: CS=GPIO6 (PHONE), SCLK=GPIO31 (PON), MOSI=GPIO30 (LOS), MISO=GPIO12 (LAN1).");
    puts("Os LEDs sao conectados diretamente ao SoC e devem ser isolados antes de ligar um periferico externo.");
}

int main(int argc, char **argv)
{
    unsigned long mode, delay, pin[4], value;
    uint8_t tx[MAX_BYTES], rx[MAX_BYTES];
    size_t i, count, data_start;
    if (argc == 2 && strcmp(argv[1], "info") == 0) { info(); return 0; }
    if (argc < 5 || strcmp(argv[2], "--force") ||
        number(argv[3], 0UL, 3UL, &mode) || number(argv[4], 0UL, 1000000UL, &delay)) {
        usage(argv[0]); return 2;
    }
    if (strcmp(argv[1], "xfer-default") == 0) {
        if (argc < 6) { usage(argv[0]); return 2; }
        pin[0] = DEFAULT_CS;
        pin[1] = DEFAULT_SCLK;
        pin[2] = DEFAULT_MOSI;
        pin[3] = DEFAULT_MISO;
        data_start = 5U;
    } else if (strcmp(argv[1], "xfer") == 0) {
        if (argc < 10) { usage(argv[0]); return 2; }
        for (i = 0U; i < 4U; ++i)
            if (number(argv[5 + i], 0UL, UINT_MAX, &pin[i])) { usage(argv[0]); return 2; }
        data_start = 9U;
    } else {
        usage(argv[0]); return 2;
    }
    if (pin[0] == pin[1] || pin[0] == pin[2] || pin[0] == pin[3] ||
        pin[1] == pin[2] || pin[1] == pin[3] || pin[2] == pin[3]) {
        fputs("CS, SCLK, MOSI e MISO precisam ser distintos\n", stderr); return 2;
    }
    count = (size_t)(argc - (int)data_start);
    if (count > MAX_BYTES) { fprintf(stderr, "maximo: %u bytes\n", MAX_BYTES); return 2; }
    for (i = 0U; i < count; ++i) {
        if (number(argv[data_start + i], 0UL, UCHAR_MAX, &value)) { usage(argv[0]); return 2; }
        tx[i] = (uint8_t)value;
    }
    if (transfer((uint32_t)pin[0], (uint32_t)pin[1], (uint32_t)pin[2],
                 (uint32_t)pin[3], (unsigned int)mode, delay, tx, rx, count)) return 1;
    fputs("TX:", stdout); for (i = 0U; i < count; ++i) printf(" %02x", tx[i]);
    fputs("\nRX:", stdout); for (i = 0U; i < count; ++i) printf(" %02x", rx[i]);
    putchar('\n'); return 0;
}
