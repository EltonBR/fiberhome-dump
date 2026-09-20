/*
 * onu-cpu-stress: carga determinística de CPU para a ONU FiberHome SD5116.
 * Não acessa NAND, GPIO, rede ou arquivos persistentes.
 */

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_WORKERS 8

static volatile sig_atomic_t stop_requested;

static void request_stop(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

static void show_usage(const char *program)
{
    fprintf(stderr,
            "Uso: %s [-w workers] [-d segundos]\n"
            "\n"
            "Gera carga contínua de CPU sem gravar dados.\n"
            "  -w workers   processos de carga (padrão: 1; máximo: %d)\n"
            "  -d segundos  encerra após este tempo; sem -d, use Ctrl+C\n"
            "  -h           mostra esta ajuda\n",
            program, MAX_WORKERS);
}

static int parse_positive(const char *text, int maximum, int *result)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || *text == '\0' || *end != '\0' || value < 1 ||
        value > maximum || value > INT_MAX) {
        return -1;
    }

    *result = (int)value;
    return 0;
}

static void worker_loop(unsigned int seed)
{
    volatile uint32_t state = 0x9e3779b9U ^ seed;

    while (!stop_requested) {
        unsigned int index;

        /* Operações inteiras independentes: mantém o Cortex-A9 ocupado. */
        for (index = 0; index < 16384U; ++index) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            state += 0x6d2b79f5U;
        }
    }

    /* Impede que um compilador elimine o loop como cálculo sem efeito. */
    if (state == 0U) {
        (void)write(STDOUT_FILENO, "", 0);
    }
}

int main(int argc, char **argv)
{
    int duration = 0;
    int workers = 1;
    int option;
    int index;
    int exit_status = EXIT_SUCCESS;
    pid_t children[MAX_WORKERS];

    while ((option = getopt(argc, argv, "w:d:h")) != -1) {
        switch (option) {
        case 'w':
            if (parse_positive(optarg, MAX_WORKERS, &workers) != 0) {
                fprintf(stderr, "workers deve estar entre 1 e %d\n", MAX_WORKERS);
                return EXIT_FAILURE;
            }
            break;
        case 'd':
            if (parse_positive(optarg, INT_MAX, &duration) != 0) {
                fprintf(stderr, "segundos deve ser um inteiro positivo\n");
                return EXIT_FAILURE;
            }
            break;
        case 'h':
            show_usage(argv[0]);
            return EXIT_SUCCESS;
        default:
            show_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (optind != argc) {
        show_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (signal(SIGINT, request_stop) == SIG_ERR ||
        signal(SIGTERM, request_stop) == SIG_ERR ||
        signal(SIGALRM, request_stop) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    for (index = 0; index < workers; ++index) {
        pid_t child = fork();

        if (child < 0) {
            perror("fork");
            stop_requested = 1;
            workers = index;
            exit_status = EXIT_FAILURE;
            break;
        }
        if (child == 0) {
            worker_loop((unsigned int)getpid());
            return EXIT_SUCCESS;
        }
        children[index] = child;
    }

    printf("Carga iniciada: %d worker(s)", workers);
    if (duration > 0) {
        printf(", duração: %d s", duration);
        (void)alarm((unsigned int)duration);
    } else {
        printf(", duração: contínua (Ctrl+C para encerrar)");
    }
    printf(".\n");
    fflush(stdout);

    while (!stop_requested) {
        pause();
    }

    (void)alarm(0);
    for (index = 0; index < workers; ++index) {
        (void)kill(children[index], SIGTERM);
    }
    for (index = 0; index < workers; ++index) {
        int status;

        while (waitpid(children[index], &status, 0) < 0) {
            if (errno != EINTR) {
                perror("waitpid");
                exit_status = EXIT_FAILURE;
                break;
            }
        }
    }

    printf("Carga encerrada.\n");
    return exit_status;
}
