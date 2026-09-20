/*
 * onu-top -- monitor compacto para Linux 2.6/BusyBox.
 * Sem ncurses, sem alocacao dinamica e sem historico de amostras.
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MAX_PROCS 128
#define COMMAND_LEN 80
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_CYAN "\033[36m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_RED "\033[31m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define MAX_PID_SCAN 4096

struct cpu_total {
    unsigned long long total;
    unsigned long long idle;
};

struct process {
    int pid;
    char state;
    char command[COMMAND_LEN];
    unsigned long long ticks;
    unsigned long vsize;
    long rss_pages;
    unsigned int cpu_x10;
};

struct memory_info {
    unsigned long total;
    unsigned long free_kb;
    unsigned long buffers;
    unsigned long cached;
    unsigned long swap_total;
    unsigned long swap_free;
};

static volatile sig_atomic_t keep_running = 1;
static struct termios saved_termios;
static int termios_changed;

static void restore_terminal(void)
{
    if (termios_changed) {
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
        termios_changed = 0;
    }
}

static void stop_handler(int unused)
{
    (void)unused;
    keep_running = 0;
}

static void trim_command(char *text)
{
    size_t i;

    for (i = 0; text[i] != '\0'; ++i) {
        if (text[i] == '\n' || text[i] == '\r' || text[i] == '\t')
            text[i] = ' ';
    }
}

static void read_command_line(int pid, struct process *result)
{
    char path[64];
    char fallback[COMMAND_LEN];
    FILE *file;
    size_t count;
    size_t i;
    size_t fallback_len;

    (void)snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    file = fopen(path, "r");
    if (file == NULL)
        return;
    fallback_len = strlen(result->command);
    if (fallback_len > sizeof(fallback) - 3)
        fallback_len = sizeof(fallback) - 3;
    fallback[0] = '[';
    memcpy(fallback + 1, result->command, fallback_len);
    fallback[fallback_len + 1] = ']';
    fallback[fallback_len + 2] = '\0';
    count = fread(result->command, 1, sizeof(result->command) - 1, file);
    (void)fclose(file);
    if (count == 0) {
        (void)snprintf(result->command, sizeof(result->command), "%s", fallback);
        return;
    }
    for (i = 0; i < count; ++i) {
        if (result->command[i] == '\0')
            result->command[i] = ' ';
    }
    while (count > 0 && result->command[count - 1] == ' ')
        --count;
    result->command[count] = '\0';
    trim_command(result->command);
}

static int read_cpu_total(struct cpu_total *result)
{
    FILE *file;
    char line[256];
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;

    file = fopen("/proc/stat", "r");
    if (file == NULL) {
        return -1;
    }
    if (fgets(line, sizeof(line), file) == NULL) {
        (void)fclose(file);
        return -1;
    }
    (void)fclose(file);

    user = nice = system = idle = iowait = irq = softirq = steal = 0;
    if (sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq,
               &steal) < 4)
        return -1;

    result->idle = idle + iowait;
    result->total = user + nice + system + idle + iowait + irq + softirq + steal;
    return 0;
}

static int read_memory(struct memory_info *result)
{
    FILE *file;
    char key[32];
    unsigned long value;

    memset(result, 0, sizeof(*result));
    file = fopen("/proc/meminfo", "r");
    if (file == NULL)
        return -1;

    while (fscanf(file, "%31[^:]: %lu kB\n", key, &value) == 2) {
        if (strcmp(key, "MemTotal") == 0)
            result->total = value;
        else if (strcmp(key, "MemFree") == 0)
            result->free_kb = value;
        else if (strcmp(key, "Buffers") == 0)
            result->buffers = value;
        else if (strcmp(key, "Cached") == 0)
            result->cached = value;
        else if (strcmp(key, "SwapTotal") == 0)
            result->swap_total = value;
        else if (strcmp(key, "SwapFree") == 0)
            result->swap_free = value;
    }
    (void)fclose(file);
    return result->total == 0 ? -1 : 0;
}

static int read_process(int pid, struct process *result)
{
    char path[64];
    char data[1024];
    char *left_paren;
    char *right_paren;
    char *cursor;
    char *token;
    char *saveptr;
    FILE *file;
    int field;
    size_t command_len;

    (void)snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (file == NULL) {
        return -1;
    }
    if (fgets(data, sizeof(data), file) == NULL) {
        (void)fclose(file);
        return -1;
    }
    (void)fclose(file);

    left_paren = strchr(data, '(');
    right_paren = strrchr(data, ')');
    if (left_paren == NULL || right_paren == NULL || right_paren <= left_paren) {
        return -1;
    }

    memset(result, 0, sizeof(*result));
    result->pid = pid;
    *right_paren = '\0';
    command_len = strlen(left_paren + 1);
    if (command_len >= sizeof(result->command))
        command_len = sizeof(result->command) - 1;
    memcpy(result->command, left_paren + 1, command_len);
    result->command[command_len] = '\0';
    trim_command(result->command);
    read_command_line(pid, result);

    cursor = right_paren + 2; /* campo 3: estado */
    field = 3;
    token = strtok_r(cursor, " ", &saveptr);
    while (token != NULL) {
        if (field == 3)
            result->state = token[0];
        else if (field == 14)
            result->ticks = strtoull(token, NULL, 10);
        else if (field == 15)
            result->ticks += strtoull(token, NULL, 10);
        else if (field == 23)
            result->vsize = strtoul(token, NULL, 10);
        else if (field == 24)
            result->rss_pages = strtol(token, NULL, 10);
        token = strtok_r(NULL, " ", &saveptr);
        ++field;
    }
    return 0;
}

static int read_latest_pid(void)
{
    FILE *file;
    char line[128];
    char *last_space;
    int pid;

    file = fopen("/proc/loadavg", "r");
    if (file == NULL)
        return 0;
    if (fgets(line, sizeof(line), file) == NULL) {
        (void)fclose(file);
        return 0;
    }
    (void)fclose(file);
    last_space = strrchr(line, ' ');
    if (last_space == NULL)
        return 0;
    pid = atoi(last_space + 1);
    return pid > 0 ? pid : 0;
}

static size_t read_processes(struct process list[MAX_PROCS])
{
    int pid;
    int max_pid;
    size_t count;

    /* A uClibc atual pode usar getdents64, ausente neste kernel 2.6. */
    max_pid = read_latest_pid();
    if (max_pid > MAX_PID_SCAN)
        max_pid = MAX_PID_SCAN;
    count = 0;
    for (pid = 1; pid <= max_pid && count < MAX_PROCS; ++pid) {
        if (read_process(pid, &list[count]) == 0)
            ++count;
    }
    return count;
}

static const struct process *find_process(const struct process *list,
                                          size_t count, int pid)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        if (list[i].pid == pid)
            return &list[i];
    }
    return NULL;
}

static void calculate_cpu(struct process *current, size_t current_count,
                          const struct process *previous, size_t previous_count,
                          unsigned long long total_delta)
{
    size_t i;
    for (i = 0; i < current_count; ++i) {
        const struct process *old = find_process(previous, previous_count,
                                                 current[i].pid);
        unsigned long long delta;
        if (old == NULL || total_delta == 0 || current[i].ticks < old->ticks) {
            current[i].cpu_x10 = 0;
            continue;
        }
        delta = current[i].ticks - old->ticks;
        current[i].cpu_x10 = (unsigned int)((delta * 1000ULL) / total_delta);
    }
}

static int process_compare(const void *left, const void *right)
{
    const struct process *a = left;
    const struct process *b = right;
    if (a->cpu_x10 != b->cpu_x10)
        return a->cpu_x10 < b->cpu_x10 ? 1 : -1;
    if (a->rss_pages != b->rss_pages)
        return a->rss_pages < b->rss_pages ? 1 : -1;
    return a->pid - b->pid;
}

static void print_screen(const struct cpu_total *previous_cpu,
                         const struct cpu_total *current_cpu,
                         const struct memory_info *memory,
                         struct process *processes, size_t count,
                         int rows, int clear_screen, int cpu_valid, int color)
{
    unsigned long long total_delta = 0;
    unsigned long long idle_delta = 0;
    unsigned int cpu_x10 = 0;
    unsigned long used_kb;
    unsigned long swap_used_kb;
    FILE *load_file;
    char load[80];
    size_t i;
    long page_size;

    if (current_cpu->total >= previous_cpu->total)
        total_delta = current_cpu->total - previous_cpu->total;
    if (current_cpu->idle >= previous_cpu->idle)
        idle_delta = current_cpu->idle - previous_cpu->idle;
    if (cpu_valid && total_delta > 0 && idle_delta <= total_delta)
        cpu_x10 = (unsigned int)(((total_delta - idle_delta) * 1000ULL) /
                                 total_delta);

    used_kb = memory->total - memory->free_kb - memory->buffers - memory->cached;
    load[0] = '\0';
    load_file = fopen("/proc/loadavg", "r");
    if (load_file != NULL) {
        if (fgets(load, sizeof(load), load_file) == NULL)
            load[0] = '\0';
        (void)fclose(load_file);
    }
    trim_command(load);

    if (clear_screen)
        (void)fputs("\033[H\033[J", stdout);
    (void)printf("%s%sONU top%s  |  q: sair  r: atualizar\n",
                 color ? ANSI_BOLD ANSI_CYAN : "", color ? "" : "",
                 color ? ANSI_RESET : "");
    if (cpu_valid) {
        unsigned int cpu_percent = cpu_x10 / 10;
        unsigned int filled = (cpu_percent * 20U) / 100U;
        unsigned int j;
        const char *cpu_color;
        if (filled > 20U)
            filled = 20U;
        if (cpu_percent < 60U)
            cpu_color = ANSI_GREEN;
        else if (cpu_percent < 85U)
            cpu_color = ANSI_YELLOW;
        else
            cpu_color = ANSI_RED;
        (void)printf("CPU %s[", color ? cpu_color : "");
        for (j = 0; j < filled; ++j)
            (void)fputc('|', stdout);
        for (; j < 20U; ++j)
            (void)fputc(' ', stdout);
        (void)printf("]%s %u.%u%%  Load: %s\n", color ? ANSI_RESET : "",
                     cpu_x10 / 10, cpu_x10 % 10,
                     load[0] == '\0' ? "indisponivel" : load);
    } else {
        (void)printf("CPU %s[                    ]%s n/a  Load: %s\n",
                     color ? ANSI_GREEN : "", color ? ANSI_RESET : "",
                     load[0] == '\0' ? "indisponivel" : load);
    }
    {
        unsigned int filled = memory->total == 0 ? 0U :
            (unsigned int)((used_kb * 20UL) / memory->total);
        unsigned int j;
        if (filled > 20U)
            filled = 20U;
        (void)printf("MEM %s[", color ? ANSI_BLUE : "");
        for (j = 0; j < filled; ++j)
            (void)fputc('|', stdout);
        for (; j < 20U; ++j)
            (void)fputc(' ', stdout);
        (void)printf("]%s %lu/%lu MiB\n", color ? ANSI_RESET : "", used_kb / 1024UL,
                     memory->total / 1024UL);
    }
    swap_used_kb = memory->swap_total - memory->swap_free;
    {
        unsigned int filled = memory->swap_total == 0 ? 0U :
            (unsigned int)((swap_used_kb * 20UL) / memory->swap_total);
        unsigned int j;
        if (filled > 20U)
            filled = 20U;
        (void)printf("SWP %s[", color ? ANSI_MAGENTA : "");
        for (j = 0; j < filled; ++j)
            (void)fputc('|', stdout);
        for (; j < 20U; ++j)
            (void)fputc(' ', stdout);
        (void)printf("]%s %lu/%lu MiB\n", color ? ANSI_RESET : "", swap_used_kb / 1024UL,
                     memory->swap_total / 1024UL);
    }
    (void)printf("%s%-6s %-5s %-7s %-8s %s%s\n", color ? ANSI_YELLOW : "",
                 "PID", "STAT", "CPU", "RSS", "COMANDO", color ? ANSI_RESET : "");

    qsort(processes, count, sizeof(processes[0]), process_compare);
    page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0)
        page_size = 4096;
    if (rows < 1)
        rows = 1;
    for (i = 0; i < count && i < (size_t)rows; ++i) {
        unsigned long rss_kb = processes[i].rss_pages > 0 ?
            ((unsigned long)processes[i].rss_pages * (unsigned long)page_size) / 1024UL : 0;
        (void)printf("%-6d %-5c %3u.%u%% %6luK  %s\n", processes[i].pid,
                     processes[i].state, processes[i].cpu_x10 / 10,
                     processes[i].cpu_x10 % 10, rss_kb, processes[i].command);
    }
    (void)fflush(stdout);
}

static int wait_key(void)
{
    char key;

    return read(STDIN_FILENO, &key, 1) == 1 ? (unsigned char)key : 'q';
}

static void usage(const char *program)
{
    (void)printf("Uso: %s [-n linhas] [-1] [--no-clear]\n", program);
}

int main(int argc, char **argv)
{
    struct process previous[MAX_PROCS];
    struct process current[MAX_PROCS];
    struct cpu_total previous_cpu;
    struct cpu_total current_cpu;
    struct memory_info memory;
    size_t previous_count;
    size_t current_count;
    int rows = 16;
    int once = 0;
    int clear_screen;
    int color;
    int cpu_valid = 0;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            rows = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-1") == 0) {
            once = 1;
        } else if (strcmp(argv[i], "--no-clear") == 0) {
            clear_screen = 0;
            continue;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 2;
        }
    }
    if (rows < 1 || rows > MAX_PROCS) {
        usage(argv[0]);
        return 2;
    }

    clear_screen = isatty(STDOUT_FILENO) ? 1 : 0;
    color = isatty(STDOUT_FILENO) ? 1 : 0;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--no-clear") == 0)
            clear_screen = 0;
    }

    if (read_cpu_total(&previous_cpu) != 0) {
        (void)fputs("Erro: nao foi possivel ler /proc/stat\n", stderr);
        return 1;
    }
    previous_count = read_processes(previous);

    if (!once && isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &saved_termios) == 0) {
        struct termios raw = saved_termios;
        raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0)
            termios_changed = 1;
    }
    (void)signal(SIGINT, stop_handler);
    (void)signal(SIGTERM, stop_handler);

    do {
        if (read_cpu_total(&current_cpu) != 0 || read_memory(&memory) != 0)
            break;
        current_count = read_processes(current);
        if (cpu_valid) {
            calculate_cpu(current, current_count, previous, previous_count,
                          current_cpu.total - previous_cpu.total);
        }
        print_screen(&previous_cpu, &current_cpu, &memory, current,
                     current_count, rows, clear_screen, cpu_valid, color);
        memcpy(previous, current, sizeof(current));
        previous_count = current_count;
        previous_cpu = current_cpu;
        cpu_valid = 1;
        if (once)
            break;
        for (;;) {
            int key = wait_key();
            if (key == 'q' || key == 'Q') {
                keep_running = 0;
                break;
            }
            if (key == 'r' || key == 'R')
                break;
        }
    } while (keep_running);

    restore_terminal();
    if (clear_screen)
        (void)fputs("\n", stdout);
    return 0;
}
