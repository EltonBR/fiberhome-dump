/* onu-benchmark: testes reprodutiveis de CPU e memoria para SD5116. */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_SECONDS 10
#define DEFAULT_MIB 8
#define MAX_SECONDS 3600
#define MAX_MIB 32

static double monotonic_seconds(void)
{
    struct timespec timestamp;

    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return (double)timestamp.tv_sec + (double)timestamp.tv_nsec / 1000000000.0;
}

static int parse_range(const char *text, int minimum, int maximum, int *result)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || *text == '\0' || *end != '\0' || value < minimum ||
        value > maximum || value > INT_MAX) {
        return -1;
    }
    *result = (int)value;
    return 0;
}

static uint32_t mix32(uint32_t value)
{
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value + 0x6d2b79f5U;
}

static void benchmark_cpu(int seconds)
{
    const double start = monotonic_seconds();
    const double deadline = start + (double)seconds;
    uint64_t iterations = 0;
    volatile uint32_t state = 0x13579bdfU;
    double elapsed;

    do {
        unsigned int index;

        for (index = 0; index < 65536U; ++index) {
            state = mix32(state);
        }
        iterations += 65536U;
    } while (monotonic_seconds() < deadline);

    elapsed = monotonic_seconds() - start;
    printf("cpu.seconds=%.6f\n", elapsed);
    printf("cpu.iterations=%" PRIu64 "\n", iterations);
    printf("cpu.million_iterations_per_second=%.3f\n",
           (double)iterations / elapsed / 1000000.0);
    printf("cpu.checksum=0x%08" PRIx32 "\n", (uint32_t)state);
}

static void benchmark_memory(int seconds, int mib)
{
    const size_t bytes = (size_t)mib * 1024U * 1024U;
    const size_t words = bytes / sizeof(uint32_t);
    uint32_t *buffer;
    const double start = monotonic_seconds();
    const double deadline = start + (double)seconds;
    uint64_t passes = 0;
    uint32_t state = 0x2468ace0U;
    uint32_t checksum = 0;
    double elapsed;
    size_t index;

    buffer = (uint32_t *)malloc(bytes);
    if (buffer == NULL) {
        fprintf(stderr, "malloc falhou para %d MiB\n", mib);
        exit(EXIT_FAILURE);
    }

    for (index = 0; index < words; ++index) {
        state = mix32(state);
        buffer[index] = state;
    }

    do {
        uint32_t pass_checksum = 0;

        for (index = 0; index < words; ++index) {
            uint32_t value;

            state = mix32(state);
            value = buffer[index] ^ state;
            value = mix32(value);
            buffer[index] = value;
            pass_checksum ^= value;
        }
        checksum ^= pass_checksum;
        ++passes;
    } while (monotonic_seconds() < deadline);

    elapsed = monotonic_seconds() - start;
    printf("memory.mib=%d\n", mib);
    printf("memory.seconds=%.6f\n", elapsed);
    printf("memory.passes=%" PRIu64 "\n", passes);
    printf("memory.mib_per_second=%.3f\n",
           ((double)passes * (double)mib) / elapsed);
    printf("memory.checksum=0x%08" PRIx32 "\n", checksum);
    free(buffer);
}

static void usage(const char *program)
{
    fprintf(stderr,
            "Uso: %s [-s segundos] [-m MiB]\n"
            "  -s segundos por teste (1-%d; padrao: %d)\n"
            "  -m memoria do teste em MiB (1-%d; padrao: %d)\n"
            "  -h mostra esta ajuda\n",
            program, MAX_SECONDS, DEFAULT_SECONDS, MAX_MIB, DEFAULT_MIB);
}

int main(int argc, char **argv)
{
    int seconds = DEFAULT_SECONDS;
    int mib = DEFAULT_MIB;
    int option;

    while ((option = getopt(argc, argv, "s:m:h")) != -1) {
        switch (option) {
        case 's':
            if (parse_range(optarg, 1, MAX_SECONDS, &seconds) != 0) {
                fprintf(stderr, "segundos deve estar entre 1 e %d\n", MAX_SECONDS);
                return EXIT_FAILURE;
            }
            break;
        case 'm':
            if (parse_range(optarg, 1, MAX_MIB, &mib) != 0) {
                fprintf(stderr, "MiB deve estar entre 1 e %d\n", MAX_MIB);
                return EXIT_FAILURE;
            }
            break;
        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;
        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (optind != argc) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    printf("onu-benchmark.version=1\n");
    printf("benchmark.seconds_per_test=%d\n", seconds);
    benchmark_cpu(seconds);
    benchmark_memory(seconds, mib);
    return EXIT_SUCCESS;
}
