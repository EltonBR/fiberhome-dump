/*
 * onu-led - exemplo mínimo de CLI C para a FiberHome SD5116.
 *
 * Este primeiro backend chama o utilitário do próprio firmware,
 * /fh/extend/kdrv_debug, por execv(). Não há shell, nem interpolação de
 * argumentos. Isto preserva a ABI proprietária do driver enquanto o ioctl
 * de /dev/fhdrv_kdrv_board ainda não está documentado.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define KDRV_DEBUG "/fh/extend/kdrv_debug"

struct gpio_name {
    const char *name;
    unsigned int pin;
    int active_low;
};

/* Mapeado em runtime em /proc/driver/fh_bsp_gpio_list, hwcfg=0x24. */
static const struct gpio_name leds[] = {
    { "pon",  31U, 1 },
    { "los",  30U, 1 },
    { "voip",  6U, 1 },
    { "lan1", 12U, 1 },
    { "lan2", 13U, 1 }
};

static const struct gpio_name buttons[] = {
    { "reset", 32U, 1 },
    { "led",   14U, 1 }
};

static void usage(const char *program)
{
    fprintf(stderr,
        "Uso:\n"
        "  %s list\n"
        "  %s led <pon|los|voip|lan1|lan2> <on|off>\n"
        "  %s button <reset|led>\n"
        "  %s buttons\n"
        "  %s --dry-run led <nome> <on|off>\n",
        program, program, program, program, program);
}

static const struct gpio_name *find_name(const struct gpio_name *items,
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

static int run_gpio(const char *operation, unsigned int pin,
                    const char *value, int dry_run)
{
    char pin_text[12];
    char *argv[6];
    pid_t pid;
    int status;

    (void)snprintf(pin_text, sizeof(pin_text), "%u", pin);
    if (dry_run) {
        if (value == NULL) {
            printf("%s gpio %s %s\n", KDRV_DEBUG, operation, pin_text);
        } else {
            printf("%s gpio %s %s %s\n", KDRV_DEBUG, operation, pin_text,
                   value);
        }
        return 0;
    }

    argv[0] = (char *)"kdrv_debug";
    argv[1] = (char *)"gpio";
    argv[2] = (char *)operation;
    argv[3] = pin_text;
    argv[4] = (char *)value;
    argv[5] = NULL;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        execv(KDRV_DEBUG, argv);
        perror(KDRV_DEBUG);
        _exit(127);
    }
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "kdrv_debug falhou (status %#x)\n", status);
        return 1;
    }
    return 0;
}

static int set_led(const struct gpio_name *led, const char *state, int dry_run)
{
    int enabled;
    const char *level;

    if (strcmp(state, "on") == 0) {
        enabled = 1;
    } else if (strcmp(state, "off") == 0) {
        enabled = 0;
    } else {
        fprintf(stderr, "Estado inválido: %s (use on ou off)\n", state);
        return 1;
    }

    level = (enabled == led->active_low) ? "0" : "1";
    return run_gpio("write", led->pin, level, dry_run);
}

static int read_button(const struct gpio_name *button, int dry_run)
{
    if (dry_run) {
        printf("%s gpio read %u\n", KDRV_DEBUG, button->pin);
        return 0;
    }
    /* O binário do fabricante imprime o nível; não o parseamos. */
    return run_gpio("read", button->pin, NULL, 0);
}

static void list_items(void)
{
    size_t i;

    puts("LEDs (ativos em nível baixo):");
    for (i = 0; i < sizeof(leds) / sizeof(leds[0]); ++i) {
        printf("  %-5s GPIO %u\n", leds[i].name, leds[i].pin);
    }
    puts("Botões (pressionados em nível baixo):");
    for (i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i) {
        printf("  %-5s GPIO %u\n", buttons[i].name, buttons[i].pin);
    }
}

int main(int argc, char **argv)
{
    const struct gpio_name *item;
    int dry_run = 0;

    if (argc > 1 && strcmp(argv[1], "--dry-run") == 0) {
        dry_run = 1;
        --argc;
        ++argv;
    }

    if (argc == 2 && strcmp(argv[1], "list") == 0) {
        list_items();
        return 0;
    }
    if (argc == 4 && strcmp(argv[1], "led") == 0) {
        item = find_name(leds, sizeof(leds) / sizeof(leds[0]), argv[2]);
        if (item == NULL) {
            fprintf(stderr, "LED desconhecido: %s\n", argv[2]);
            return 1;
        }
        return set_led(item, argv[3], dry_run);
    }
    if (argc == 3 && strcmp(argv[1], "button") == 0) {
        item = find_name(buttons, sizeof(buttons) / sizeof(buttons[0]), argv[2]);
        if (item == NULL) {
            fprintf(stderr, "Botão desconhecido: %s\n", argv[2]);
            return 1;
        }
        return read_button(item, dry_run);
    }
    if (argc == 2 && strcmp(argv[1], "buttons") == 0) {
        int result = 0;
        size_t i;

        for (i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i) {
            printf("%s: ", buttons[i].name);
            result |= read_button(&buttons[i], dry_run);
        }
        return result;
    }

    usage(argv[0]);
    return 1;
}
