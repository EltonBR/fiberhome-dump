/* SSD1306 128x64 por I2C para a FiberHome SD5116. */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define KDRV_DEBUG "/fh/extend/kdrv_debug"
#define WIDTH 128U
#define PAGES 8U
#define FRAME_SIZE (WIDTH * PAGES)

static uint8_t frame[FRAME_SIZE];

static void usage(const char *program)
{
    fprintf(stderr,
        "Uso:\n"
        "  %s --force [--channel 0|1] [--address 0x3c|0x3d] init\n"
        "  %s --force [--channel 0|1] [--address 0x3c|0x3d] clear\n"
        "  %s --force [--channel 0|1] [--address 0x3c|0x3d] text <coluna> <pagina> <texto>\n"
        "  %s --force [--channel 0|1] [--address 0x3c|0x3d] demo\n",
        program, program, program, program);
}

static int parse_number(const char *text, unsigned long min, unsigned long max,
                        unsigned long *value)
{
    char *end;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 0);
    if (errno != 0 || *text == '\0' || *end != '\0' || parsed < min || parsed > max)
        return -1;
    *value = parsed;
    return 0;
}

static int run_write(unsigned long channel, unsigned long address,
                     uint8_t control, uint8_t value)
{
    char channel_text[16];
    char address_text[16];
    char control_text[16];
    char value_text[16];
    char *arguments[8];
    int status;
    pid_t pid;

    (void)snprintf(channel_text, sizeof(channel_text), "0x%lx", channel);
    (void)snprintf(address_text, sizeof(address_text), "0x%lx", address);
    (void)snprintf(control_text, sizeof(control_text), "0x%x", control);
    (void)snprintf(value_text, sizeof(value_text), "0x%x", value);
    arguments[0] = (char *)KDRV_DEBUG;
    arguments[1] = (char *)"i2c";
    arguments[2] = (char *)"write";
    arguments[3] = channel_text;
    arguments[4] = address_text;
    arguments[5] = control_text;
    arguments[6] = value_text;
    arguments[7] = NULL;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        int null_fd = open("/dev/null", O_WRONLY);

        if (null_fd >= 0) {
            (void)dup2(null_fd, STDOUT_FILENO);
            (void)dup2(null_fd, STDERR_FILENO);
            (void)close(null_fd);
        }
        execv(KDRV_DEBUG, arguments);
        _exit(127);
    }
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return -1;
        }
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}

static int command(unsigned long channel, unsigned long address, uint8_t value)
{
    return run_write(channel, address, 0x00U, value);
}

static int data(unsigned long channel, unsigned long address, uint8_t value)
{
    return run_write(channel, address, 0x40U, value);
}

static int initialize(unsigned long channel, unsigned long address)
{
    static const uint8_t sequence[] = {
        0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0x00, 0x40,
        0x8d, 0x14, 0x20, 0x00, 0xa1, 0xc8, 0xda, 0x12,
        0x81, 0xcf, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6,
        0x2e, 0xaf
    };
    size_t index;

    for (index = 0; index < sizeof(sequence); ++index)
        if (command(channel, address, sequence[index]) != 0)
            return -1;
    return 0;
}

static int refresh(unsigned long channel, unsigned long address)
{
    size_t index;

    if (command(channel, address, 0x21) || command(channel, address, 0x00) ||
        command(channel, address, 0x7f) || command(channel, address, 0x22) ||
        command(channel, address, 0x00) || command(channel, address, 0x07))
        return -1;
    for (index = 0; index < FRAME_SIZE; ++index)
        if (data(channel, address, frame[index]) != 0)
            return -1;
    return 0;
}

/* Fonte 5x7 compacta: letras, numeros e pontuacao de diagnostico. */
static const uint8_t *glyph(char input)
{
    static const uint8_t blank[5] = {0, 0, 0, 0, 0};
    static const uint8_t digits[10][5] = {
        {0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
        {0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
        {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1e}
    };
    static const uint8_t letters[26][5] = {
        {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},
        {0x3e,0x41,0x41,0x41,0x22},{0x7f,0x41,0x41,0x22,0x1c},
        {0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},
        {0x3e,0x41,0x49,0x49,0x7a},{0x7f,0x08,0x08,0x08,0x7f},
        {0x00,0x41,0x7f,0x41,0x00},{0x20,0x40,0x41,0x3f,0x01},
        {0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
        {0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},
        {0x3e,0x41,0x41,0x41,0x3e},{0x7f,0x09,0x09,0x09,0x06},
        {0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
        {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7f,0x01,0x01},
        {0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},
        {0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,0x08,0x14,0x63},
        {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43}
    };
    static const uint8_t dot[5] = {0x00,0x60,0x60,0x00,0x00};
    static const uint8_t colon[5] = {0x00,0x36,0x36,0x00,0x00};
    static const uint8_t dash[5] = {0x08,0x08,0x08,0x08,0x08};
    static const uint8_t slash[5] = {0x20,0x10,0x08,0x04,0x02};

    if (input >= 'a' && input <= 'z')
        input = (char)(input - 'a' + 'A');
    if (input >= 'A' && input <= 'Z')
        return letters[(unsigned int)(input - 'A')];
    if (input >= '0' && input <= '9')
        return digits[(unsigned int)(input - '0')];
    if (input == '.') return dot;
    if (input == ':') return colon;
    if (input == '-') return dash;
    if (input == '/') return slash;
    return blank;
}

static void put_text(unsigned long column, unsigned long page, const char *text)
{
    while (*text != '\0' && column < 21UL && page < PAGES) {
        const uint8_t *image = glyph(*text++);
        size_t index;
        size_t position = (size_t)page * WIDTH + (size_t)column * 6U;

        for (index = 0; index < 5U; ++index)
            frame[position + index] = image[index];
        frame[position + 5U] = 0U;
        ++column;
    }
}

int main(int argc, char **argv)
{
    unsigned long channel = 0UL;
    unsigned long address = 0x3cUL;
    unsigned long column;
    unsigned long page;
    int index = 1;

    if (argc < 3 || strcmp(argv[index++], "--force") != 0) {
        usage(argv[0]);
        return 2;
    }
    while (index + 1 < argc && argv[index][0] == '-') {
        if (strcmp(argv[index], "--channel") == 0 &&
            parse_number(argv[index + 1], 0UL, 1UL, &channel) == 0) {
            index += 2;
        } else if (strcmp(argv[index], "--address") == 0 &&
                   parse_number(argv[index + 1], 0x03UL, 0x77UL, &address) == 0) {
            index += 2;
        } else {
            usage(argv[0]);
            return 2;
        }
    }
    if (index >= argc) {
        usage(argv[0]);
        return 2;
    }
    if (strcmp(argv[index], "init") == 0 && index + 1 == argc)
        return initialize(channel, address) == 0 ? 0 : 1;
    if (strcmp(argv[index], "clear") == 0 && index + 1 == argc) {
        memset(frame, 0, sizeof(frame));
        return refresh(channel, address) == 0 ? 0 : 1;
    }
    if (strcmp(argv[index], "text") == 0 && index + 4 == argc &&
        parse_number(argv[index + 1], 0UL, 20UL, &column) == 0 &&
        parse_number(argv[index + 2], 0UL, 7UL, &page) == 0) {
        memset(frame, 0, sizeof(frame));
        put_text(column, page, argv[index + 3]);
        return refresh(channel, address) == 0 ? 0 : 1;
    }
    if (strcmp(argv[index], "demo") == 0 && index + 1 == argc) {
        if (initialize(channel, address) != 0)
            return 1;
        memset(frame, 0, sizeof(frame));
        put_text(1UL, 1UL, "FIBERHOME");
        put_text(1UL, 3UL, "SD5116");
        put_text(1UL, 5UL, "I2C OLED");
        return refresh(channel, address) == 0 ? 0 : 1;
    }
    usage(argv[0]);
    return 2;
}
