/*
 * onu-led-direct - acesso GPIO sem userspace proprietário.
 *
 * ABI confirmada por desassembly de libfhdrv_kdrv_board.so:
 *   struct: u32 pin; u8 level; u8 debug; u8 reserved[2]
 *   config_mux: ioctl 0x40085300
 *   set_mode:   ioctl 0x40085301 (0=entrada, 1=saída)
 *   write: ioctl 0x40085303 (_IOW('S', 3, struct gpio_request))
 *   read : ioctl 0xc0085304 (_IOWR('S', 4, struct gpio_request))
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define BOARD_DEVICE "/dev/fhdrv_kdrv_board"
#define FHDRV_GPIO_CONFIG_MUX 0x40085300UL
#define FHDRV_GPIO_SET_MODE   0x40085301UL
#define FHDRV_GPIO_WRITE 0x40085303UL
#define FHDRV_GPIO_READ  0xc0085304UL

struct gpio_request {
    uint32_t pin;
    uint8_t level;
    uint8_t debug;
    uint8_t reserved[2];
};

struct gpio_name {
    const char *name;
    uint32_t pin;
    int active_low;
};

static const struct gpio_name leds[] = {
    { "pon",  31U, 1 }, { "los", 30U, 1 }, { "voip", 6U, 1 },
    { "lan1", 12U, 1 }, { "lan2", 13U, 1 }
};
static const struct gpio_name buttons[] = {
    { "reset", 32U, 1 }, { "led", 14U, 1 }
};

static void usage(const char *program)
{
    fprintf(stderr,
        "Uso: %s list | %s led <nome> <on|off> | %s button <reset|led>\n",
        program, program, program);
}

static const struct gpio_name *find_gpio(const struct gpio_name *items,
                                         size_t count, const char *name)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        if (strcmp(items[i].name, name) == 0) {
            return &items[i];
        }
    }
    return NULL;
}

static int gpio_request(unsigned long command, uint32_t pin, uint8_t *level)
{
    struct gpio_request request;
    int fd;
    int result;

    memset(&request, 0, sizeof(request));
    request.pin = pin;
    request.level = *level;
    fd = open(BOARD_DEVICE, O_RDWR);
    if (fd < 0) {
        perror(BOARD_DEVICE);
        return -1;
    }
    result = ioctl(fd, command, &request);
    if (result < 0) {
        perror("ioctl GPIO");
        (void)close(fd);
        return -1;
    }
    (void)close(fd);
    *level = request.level;
    return 0;
}

/* Mesma sequência usada por `kdrv_debug gpio read/write`. */
static int gpio_prepare(uint32_t pin, int output)
{
    uint8_t argument = 0;

    if (gpio_request(FHDRV_GPIO_CONFIG_MUX, pin, &argument) != 0) {
        return -1;
    }
    argument = output ? 1U : 0U;
    return gpio_request(FHDRV_GPIO_SET_MODE, pin, &argument);
}

static void list_items(void)
{
    size_t i;
    puts("LEDs ativos em nível baixo:");
    for (i = 0; i < sizeof(leds) / sizeof(leds[0]); ++i) {
        printf("  %-5s GPIO %u\n", leds[i].name, (unsigned int)leds[i].pin);
    }
    puts("Botões pressionados em nível baixo:");
    for (i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i) {
        printf("  %-5s GPIO %u\n", buttons[i].name,
               (unsigned int)buttons[i].pin);
    }
}

int main(int argc, char **argv)
{
    const struct gpio_name *gpio;
    uint8_t level;

    if (argc == 2 && strcmp(argv[1], "list") == 0) {
        list_items();
        return 0;
    }
    if (argc == 4 && strcmp(argv[1], "led") == 0) {
        gpio = find_gpio(leds, sizeof(leds) / sizeof(leds[0]), argv[2]);
        if (gpio == NULL || (strcmp(argv[3], "on") != 0 &&
                             strcmp(argv[3], "off") != 0)) {
            usage(argv[0]);
            return 1;
        }
        level = (strcmp(argv[3], "on") == 0) ?
            (gpio->active_low ? 0U : 1U) : (gpio->active_low ? 1U : 0U);
        if (gpio_prepare(gpio->pin, 1) != 0) {
            return 1;
        }
        return gpio_request(FHDRV_GPIO_WRITE, gpio->pin, &level) == 0 ? 0 : 1;
    }
    if (argc == 3 && strcmp(argv[1], "button") == 0) {
        gpio = find_gpio(buttons, sizeof(buttons) / sizeof(buttons[0]), argv[2]);
        if (gpio == NULL) {
            usage(argv[0]);
            return 1;
        }
        level = 0;
        if (gpio_prepare(gpio->pin, 0) != 0) {
            return 1;
        }
        if (gpio_request(FHDRV_GPIO_READ, gpio->pin, &level) != 0) {
            return 1;
        }
        printf("%s: %s (nivel=%u)\n", gpio->name,
               (level == 0U) ? "pressionado" : "solto", (unsigned int)level);
        return 0;
    }
    usage(argv[0]);
    return 1;
}
