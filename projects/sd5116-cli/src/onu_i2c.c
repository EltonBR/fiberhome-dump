/*
 * onu-i2c -- ferramenta I2C para a FiberHome SD5116.
 *
 * Esta primeira versão usa a interface confirmada do firmware:
 *     /fh/extend/kdrv_debug i2c {read,write} ...
 *
 * Ela não chama um shell e passa cada argumento por execv().  Não usa a
 * interface Linux i2c-dev porque esta imagem não expõe /dev/i2c-0 ou
 * /dev/i2c-1, mesmo com i2cdev.ko carregado.
 */

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define KDRV_DEBUG "/fh/extend/kdrv_debug"
#define MAX_OUTPUT 4096U
#define MAX_READ   32UL

static volatile sig_atomic_t stop_requested;

static void stop_wave(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

static void usage(const char *program)
{
    fprintf(stderr,
        "Uso:\n"
        "  %s detect <canal> <endereco> [offset]\n"
        "  %s read   <canal> <endereco> <offset> <tamanho>\n"
        "  %s write  --force <canal> <endereco> <offset> <byte>\n"
        "  %s scan   <canal> [inicio fim]\n"
        "  %s wave   --force <canal> <endereco> <offset> <tamanho>\n"
        "  %s wave-write --force <canal> <endereco> <offset> <byte>\n"
        "\n"
        "Todos os numeros aceitam decimal ou 0x hexadecimal.\n"
        "Canais confirmados pelo firmware: 0 e 1. Enderecos: 7 bits (0x03..0x77).\n"
        "scan e detect fazem uma leitura de registrador; nao sao passivos.\n",
        program, program, program, program, program, program);
}

static int parse_number(const char *text, unsigned long minimum,
                        unsigned long maximum, unsigned long *value)
{
    char *end;
    unsigned long parsed;

    if (text == NULL || *text == '\0')
        return -1;

    errno = 0;
    parsed = strtoul(text, &end, 0);
    if (errno != 0 || *end != '\0' || parsed < minimum || parsed > maximum)
        return -1;

    *value = parsed;
    return 0;
}

static void number_to_text(unsigned long value, char *destination,
                           size_t destination_size)
{
    (void)snprintf(destination, destination_size, "0x%lx", value);
}

/* Executa o helper e devolve a sua saida. Retorna zero somente com exit(0). */
static int run_kdrv(char *const arguments[], char *output, size_t output_size)
{
    int pipefd[2];
    pid_t pid;
    int status;
    ssize_t count;
    size_t used = 0U;

    if (pipe(pipefd) != 0) {
        perror("pipe");
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        (void)close(pipefd[0]);
        (void)close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        (void)close(pipefd[0]);
        (void)dup2(pipefd[1], STDOUT_FILENO);
        (void)dup2(pipefd[1], STDERR_FILENO);
        (void)close(pipefd[1]);
        execv(KDRV_DEBUG, arguments);
        _exit(127);
    }

    (void)close(pipefd[1]);
    while (used + 1U < output_size) {
        count = read(pipefd[0], output + used, output_size - used - 1U);
        if (count > 0) {
            used += (size_t)count;
            continue;
        }
        if (count < 0 && errno == EINTR)
            continue;
        break;
    }
    output[used] = '\0';
    (void)close(pipefd[0]);

    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return -1;
        }
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}

/*
 * kdrv_debug tem um defeito importante: o processo termina com exit(0) mesmo
 * se fhdrv_kdrv_i2c_{read,write} retornar erro. A sua propria mensagem e a
 * unica indicacao confiavel que temos nessa interface.
 */
static int output_reports_failure(const char *output)
{
    return strstr(output, "failled!") != NULL ||
           strstr(output, "fail.") != NULL ||
           strstr(output, "error") != NULL ||
           strstr(output, "invalid paras") != NULL;
}

static int i2c_read(unsigned long channel, unsigned long address,
                    unsigned long offset, unsigned long length,
                    char *output, size_t output_size)
{
    char channel_text[16];
    char address_text[16];
    char offset_text[16];
    char length_text[16];
    char *arguments[8];

    number_to_text(channel, channel_text, sizeof(channel_text));
    number_to_text(address, address_text, sizeof(address_text));
    number_to_text(offset, offset_text, sizeof(offset_text));
    number_to_text(length, length_text, sizeof(length_text));
    arguments[0] = (char *)KDRV_DEBUG;
    arguments[1] = (char *)"i2c";
    arguments[2] = (char *)"read";
    arguments[3] = channel_text;
    arguments[4] = address_text;
    arguments[5] = offset_text;
    arguments[6] = length_text;
    arguments[7] = NULL;
    if (run_kdrv(arguments, output, output_size) != 0)
        return -1;

    /* A implementacao de kdrv_debug so imprime este cabecalho apos sucesso. */
    if (output_reports_failure(output) || strstr(output, "hex data:") == NULL)
        return -1;
    return 0;
}

/* Executa a escrita, sem interpretar a mensagem proprietaria do helper. */
static int i2c_write_raw(unsigned long channel, unsigned long address,
                         unsigned long offset, unsigned long value,
                         char *output, size_t output_size)
{
    char channel_text[16];
    char address_text[16];
    char offset_text[16];
    char value_text[16];
    char *arguments[8];

    number_to_text(channel, channel_text, sizeof(channel_text));
    number_to_text(address, address_text, sizeof(address_text));
    number_to_text(offset, offset_text, sizeof(offset_text));
    number_to_text(value, value_text, sizeof(value_text));
    arguments[0] = (char *)KDRV_DEBUG;
    arguments[1] = (char *)"i2c";
    arguments[2] = (char *)"write";
    arguments[3] = channel_text;
    arguments[4] = address_text;
    arguments[5] = offset_text;
    arguments[6] = value_text;
    arguments[7] = NULL;
    return run_kdrv(arguments, output, output_size);
}

static int i2c_write(unsigned long channel, unsigned long address,
                     unsigned long offset, unsigned long value,
                     char *output, size_t output_size)
{
    if (i2c_write_raw(channel, address, offset, value, output,
                      output_size) != 0)
        return -1;
    return output_reports_failure(output) ? -1 : 0;
}

static int parse_bus(unsigned long *channel, const char *text)
{
    return parse_number(text, 0UL, 1UL, channel);
}

static int parse_address(unsigned long *address, const char *text)
{
    return parse_number(text, 0x03UL, 0x77UL, address);
}

int main(int argc, char **argv)
{
    unsigned long channel;
    unsigned long address;
    unsigned long offset;
    unsigned long length;
    unsigned long value;
    unsigned long first;
    unsigned long last;
    char output[MAX_OUTPUT];
    int result;

    if (argc < 2) {
        usage(argv[0]);
        return 2;
    }

    if (strcmp(argv[1], "read") == 0) {
        if (argc != 6 || parse_bus(&channel, argv[2]) != 0 ||
            parse_address(&address, argv[3]) != 0 ||
            parse_number(argv[4], 0UL, UCHAR_MAX, &offset) != 0 ||
            parse_number(argv[5], 1UL, MAX_READ, &length) != 0) {
            usage(argv[0]);
            return 2;
        }
        result = i2c_read(channel, address, offset, length, output, sizeof(output));
        if (output[0] != '\0')
            fputs(output, result == 0 ? stdout : stderr);
        if (result != 0)
            fprintf(stderr, "falha I2C: canal %lu endereco 0x%02lx\n", channel, address);
        return result == 0 ? 0 : 1;
    }

    if (strcmp(argv[1], "detect") == 0) {
        if ((argc != 4 && argc != 5) || parse_bus(&channel, argv[2]) != 0 ||
            parse_address(&address, argv[3]) != 0 ||
            (argc == 5 && parse_number(argv[4], 0UL, UCHAR_MAX, &offset) != 0)) {
            usage(argv[0]);
            return 2;
        }
        if (argc == 4)
            offset = 0UL;
        result = i2c_read(channel, address, offset, 1UL, output, sizeof(output));
        if (result == 0) {
            printf("detectado: canal %lu endereco 0x%02lx (offset 0x%02lx)\n",
                   channel, address, offset);
            if (output[0] != '\0')
                fputs(output, stdout);
            return 0;
        }
        fprintf(stderr, "nao respondeu: canal %lu endereco 0x%02lx\n", channel, address);
        return 1;
    }

    if (strcmp(argv[1], "write") == 0) {
        if (argc != 7 || strcmp(argv[2], "--force") != 0 ||
            parse_bus(&channel, argv[3]) != 0 ||
            parse_address(&address, argv[4]) != 0 ||
            parse_number(argv[5], 0UL, UCHAR_MAX, &offset) != 0 ||
            parse_number(argv[6], 0UL, UCHAR_MAX, &value) != 0) {
            usage(argv[0]);
            return 2;
        }
        result = i2c_write(channel, address, offset, value, output, sizeof(output));
        if (output[0] != '\0')
            fputs(output, result == 0 ? stdout : stderr);
        if (result != 0)
            fprintf(stderr, "falha na escrita I2C: canal %lu endereco 0x%02lx\n",
                    channel, address);
        return result == 0 ? 0 : 1;
    }

    if (strcmp(argv[1], "scan") == 0) {
        if ((argc != 3 && argc != 5) || parse_bus(&channel, argv[2]) != 0) {
            usage(argv[0]);
            return 2;
        }
        first = 0x03UL;
        last = 0x77UL;
        if (argc == 5 && (parse_address(&first, argv[3]) != 0 ||
                          parse_address(&last, argv[4]) != 0 || first > last)) {
            usage(argv[0]);
            return 2;
        }
        fprintf(stderr, "AVISO: scan faz uma leitura em cada endereco e pode demorar.\n");
        printf("Canal %lu; enderecos que responderam:\n", channel);
        for (address = first; address <= last; ++address) {
            result = i2c_read(channel, address, 0UL, 1UL, output, sizeof(output));
            if (result == 0)
                printf("  0x%02lx\n", address);
        }
        return 0;
    }

    if (strcmp(argv[1], "wave") == 0) {
        unsigned long transactions = 0UL;

        if (argc != 7 || strcmp(argv[2], "--force") != 0 ||
            parse_bus(&channel, argv[3]) != 0 ||
            parse_address(&address, argv[4]) != 0 ||
            parse_number(argv[5], 0UL, UCHAR_MAX, &offset) != 0 ||
            parse_number(argv[6], 1UL, MAX_READ, &length) != 0) {
            usage(argv[0]);
            return 2;
        }
        if (signal(SIGINT, stop_wave) == SIG_ERR ||
            signal(SIGTERM, stop_wave) == SIG_ERR) {
            perror("signal");
            return 1;
        }

        fprintf(stderr,
                "I2C wave: leituras repetidas no canal %lu, endereco 0x%02lx; "
                "Ctrl+C para parar.\n", channel, address);
        while (!stop_requested) {
            result = i2c_read(channel, address, offset, length,
                               output, sizeof(output));
            if (result != 0) {
                fprintf(stderr,
                        "wave interrompido apos %lu transacoes: falha I2C no "
                        "canal %lu endereco 0x%02lx\n",
                        transactions, channel, address);
                return 1;
            }
            ++transactions;
        }
        fprintf(stderr, "I2C wave encerrado: %lu transacoes.\n", transactions);
        return 0;
    }

    if (strcmp(argv[1], "wave-write") == 0) {
        unsigned long transactions = 0UL;

        if (argc != 7 || strcmp(argv[2], "--force") != 0 ||
            parse_bus(&channel, argv[3]) != 0 ||
            parse_address(&address, argv[4]) != 0 ||
            parse_number(argv[5], 0UL, UCHAR_MAX, &offset) != 0 ||
            parse_number(argv[6], 0UL, UCHAR_MAX, &value) != 0) {
            usage(argv[0]);
            return 2;
        }
        if (signal(SIGINT, stop_wave) == SIG_ERR ||
            signal(SIGTERM, stop_wave) == SIG_ERR) {
            perror("signal");
            return 1;
        }

        fprintf(stderr,
                "I2C wave-write: escrita repetida no canal %lu, endereco "
                "0x%02lx; Ctrl+C para parar.\n", channel, address);
        while (!stop_requested) {
            /*
             * O SSD1306 ja foi confirmado com o mesmo helper pelo onu-oled.
             * Para esse gerador de onda, uma mensagem "failled!" do helper
             * nao e criterio de parada: ele a produz mesmo quando o OLED
             * recebe a escrita. A execucao do processo ainda precisa fechar
             * com sucesso.
             */
            result = i2c_write_raw(channel, address, offset, value,
                                   output, sizeof(output));
            if (result != 0) {
                fprintf(stderr,
                        "wave-write interrompido apos %lu transacoes: falha "
                        "I2C no canal %lu endereco 0x%02lx\n",
                        transactions, channel, address);
                return 1;
            }
            ++transactions;
        }
        fprintf(stderr, "I2C wave-write encerrado: %lu transacoes.\n", transactions);
        return 0;
    }

    usage(argv[0]);
    return 2;
}
