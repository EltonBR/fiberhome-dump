/* Relé de shell serial para SSD1306 externo, SD5116. */
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define HI_UBASIC "/lib/hsan/so/service/libhi_ubasic.so"
#define HI_IOREACTOR "/lib/hsan/so/service/libhi_ioreactor.so"
#define HI_IPC "/lib/hsan/so/service/libhi_ipc.so"
#define HI_HAL "/lib/hsan/so/service/libhi_hal.so"
#define OLED_ADDRESS 0x3cU
#define WIDTH 128U
#define ROWS 8U
#define COLS 21U

struct i2c_attr { uint32_t index, enable, address_mode, baud_rate; };
struct i2c_send { uint32_t address; const uint8_t *data; uint32_t length, stop; };
typedef int (*attr_set_fn)(const struct i2c_attr *);
typedef int (*data_send_fn)(const struct i2c_send *);
static volatile sig_atomic_t interrupted;

static void on_signal(int ignored) { (void)ignored; interrupted = 1; }
static int write_all(int fd, const void *buffer, size_t length)
{
    const uint8_t *data = buffer;
    while (length != 0U) {
        ssize_t written = write(fd, data, length);
        if (written > 0) { data += written; length -= (size_t)written; }
        else if (written < 0 && errno == EINTR) continue;
        else return -1;
    }
    return 0;
}
static int set_baud(attr_set_fn set_attr, uint32_t baud)
{
    struct i2c_attr attr = {0U, 1U, 0U, baud};
    return set_attr(&attr);
}
static int send_bytes(data_send_fn send, const uint8_t *bytes, uint32_t length)
{
    struct i2c_send data = {OLED_ADDRESS, bytes, length, 1U};
    return send(&data);
}
static const uint8_t *glyph(char ch)
{
    static const uint8_t blank[5] = {0,0,0,0,0};
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
    static const uint8_t digits[10][5] = {
        {0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
        {0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
        {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1e}
    };
    static const uint8_t dot[5] = {0,0x60,0x60,0,0};
    static const uint8_t colon[5] = {0,0x36,0x36,0,0};
    static const uint8_t dash[5] = {0x08,0x08,0x08,0x08,0x08};
    static const uint8_t slash[5] = {0x20,0x10,0x08,0x04,0x02};
    static const uint8_t gt[5] = {0x00,0x41,0x22,0x14,0x08};
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    if (ch >= 'A' && ch <= 'Z') return letters[(unsigned int)(ch - 'A')];
    if (ch >= '0' && ch <= '9') return digits[(unsigned int)(ch - '0')];
    if (ch == '.') return dot;
    if (ch == ':') return colon;
    if (ch == '-') return dash;
    if (ch == '/') return slash;
    if (ch == '>') return gt;
    return blank;
}
static void scroll(char cells[ROWS][COLS])
{
    memmove(cells[0], cells[1], (ROWS - 1U) * COLS);
    memset(cells[ROWS - 1U], ' ', COLS);
}
static void consume(char cells[ROWS][COLS], unsigned int *row,
                    unsigned int *column, unsigned int *escape, uint8_t ch)
{
    if (*escape != 0U) {
        if (*escape == 1U && ch == '[') *escape = 2U;
        else if (ch >= 0x40U && ch <= 0x7eU) *escape = 0U;
        return;
    }
    if (ch == 0x1bU) { *escape = 1U; return; }
    if (ch == '\r') { *column = 0U; return; }
    if (ch == '\n') {
        *column = 0U;
        if (++*row == ROWS) { scroll(cells); *row = ROWS - 1U; }
        return;
    }
    if (ch == '\b' || ch == 0x7fU) { if (*column != 0U) --*column; return; }
    if (ch < 0x20U || ch > 0x7eU) return;
    cells[*row][*column] = (char)ch;
    if (++*column == COLS) {
        *column = 0U;
        if (++*row == ROWS) { scroll(cells); *row = ROWS - 1U; }
    }
}
static int oled_init(data_send_fn send)
{
    static const uint8_t init[] = {0x00,0xae,0xd5,0x80,0xa8,0x3f,0xd3,0,0x40,
        0x8d,0x14,0x20,0,0xa1,0xc8,0xda,0x12,0x81,0xcf,0xd9,0xf1,0xdb,0x40,
        0xa4,0xa6,0x2e,0xaf};
    return send_bytes(send, init, sizeof(init));
}
static int oled_draw(data_send_fn send, char cells[ROWS][COLS])
{
    static const uint8_t window[] = {0x00,0x21,0,0x7f,0x22,0,0x07};
    uint8_t frame[WIDTH * ROWS], part[WIDTH], tail[2];
    unsigned int row, col, index;
    memset(frame, 0, sizeof(frame));
    for (row = 0U; row < ROWS; ++row) for (col = 0U; col < COLS; ++col) {
        const uint8_t *pixels = glyph(cells[row][col]);
        for (index = 0U; index < 5U; ++index) frame[row * WIDTH + col * 6U + index] = pixels[index];
    }
    if (send_bytes(send, window, sizeof(window)) != 0) return -1;
    for (row = 0U; row < ROWS; ++row) {
        part[0] = 0x40U;
        memcpy(part + 1U, frame + row * WIDTH, WIDTH - 1U);
        tail[0] = 0x40U; tail[1] = frame[row * WIDTH + WIDTH - 1U];
        if (send_bytes(send, part, sizeof(part)) != 0 || send_bytes(send, tail, sizeof(tail)) != 0) return -1;
    }
    return 0;
}
static int open_slave(int master, const struct termios *term, const struct winsize *size)
{
    char *name; int slave;
    if (grantpt(master) != 0 || unlockpt(master) != 0 || (name = ptsname(master)) == NULL) return -1;
    slave = open(name, O_RDWR | O_NOCTTY);
    if (slave >= 0) { (void)tcsetattr(slave, TCSANOW, term); (void)ioctl(slave, TIOCSWINSZ, size); }
    return slave;
}
int main(int argc, char **argv)
{
    void *ubasic, *ioreactor, *ipc, *hal; attr_set_fn set_attr; data_send_fn send;
    struct termios term; struct winsize size; int master, slave; pid_t child; int status;
    char cells[ROWS][COLS]; unsigned int row = 0U, column = 0U, escape = 0U;
    if (argc != 2 || strcmp(argv[1], "--force") != 0) { fprintf(stderr, "Uso: %s --force\n", argv[0]); return 2; }
    if (tcgetattr(STDIN_FILENO, &term) != 0) { perror("stdin nao e terminal"); return 1; }
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &size) != 0) memset(&size, 0, sizeof(size));
    ubasic = dlopen(HI_UBASIC, RTLD_NOW | RTLD_GLOBAL);
    ioreactor = ubasic == NULL ? NULL : dlopen(HI_IOREACTOR, RTLD_NOW | RTLD_GLOBAL);
    ipc = ioreactor == NULL ? NULL : dlopen(HI_IPC, RTLD_NOW | RTLD_GLOBAL);
    hal = ipc == NULL ? NULL : dlopen(HI_HAL, RTLD_NOW | RTLD_GLOBAL);
    if (hal == NULL) { fprintf(stderr, "nao carregou HAL I2C: %s\n", dlerror()); return 1; }
    set_attr = (attr_set_fn)dlsym(hal, "hi_hal_i2c_attr_set"); send = (data_send_fn)dlsym(hal, "hi_hal_i2c_data_send");
    if (set_attr == NULL || send == NULL || set_baud(set_attr, 1U) != 0 || oled_init(send) != 0) { fprintf(stderr, "falha ao iniciar OLED\n"); return 1; }
    memset(cells, ' ', sizeof(cells));
    master = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (master < 0 || (slave = open_slave(master, &term, &size)) < 0) { perror("pty"); (void)set_baud(set_attr, 0U); return 1; }
    child = fork();
    if (child == 0) { setsid(); (void)ioctl(slave, TIOCSCTTY, 0); dup2(slave,0); dup2(slave,1); dup2(slave,2); close(master); close(slave); execl("/bin/sh", "sh", "-i", (char *)NULL); _exit(127); }
    close(slave); signal(SIGINT, on_signal); signal(SIGTERM, on_signal);
    while (!interrupted) {
        fd_set ready; int maximum = master > STDIN_FILENO ? master : STDIN_FILENO; uint8_t buffer[256]; ssize_t count; unsigned int index;
        FD_ZERO(&ready); FD_SET(STDIN_FILENO, &ready); FD_SET(master, &ready);
        if (select(maximum + 1, &ready, NULL, NULL, NULL) < 0) { if (errno == EINTR) continue; break; }
        if (FD_ISSET(STDIN_FILENO, &ready) && (count = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) (void)write_all(master, buffer, (size_t)count);
        if (FD_ISSET(master, &ready) && (count = read(master, buffer, sizeof(buffer))) > 0) {
            (void)write_all(STDOUT_FILENO, buffer, (size_t)count);
            for (index = 0U; index < (unsigned int)count; ++index) consume(cells, &row, &column, &escape, buffer[index]);
            if (oled_draw(send, cells) != 0) { fprintf(stderr, "\nfalha ao atualizar OLED\n"); break; }
        } else if (FD_ISSET(master, &ready)) break;
    }
    kill(child, SIGHUP); (void)waitpid(child, &status, 0); close(master); (void)set_baud(set_attr, 0U);
    (void)dlclose(hal); (void)dlclose(ipc); (void)dlclose(ioreactor); (void)dlclose(ubasic);
    return 0;
}
