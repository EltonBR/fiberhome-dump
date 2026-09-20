/* Autoteste reversível: LEDs, botões e I2C do OLED SSD1306 externo. */
#include "fh_gpio.h"
#include "fh_i2c.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define OLED_ADDRESS 0x3cU
#define LED_DELAY_MS 200L

struct led { const char *name; uint32_t pin; };
static const struct led leds[] = {
    { "PON", PIN_LED_PON }, { "LOS", PIN_LED_LOS },
    { "PHONE", PIN_LED_PHONE }, { "LAN1", PIN_LED_LAN1 },
    { "LAN2", PIN_LED_LAN2 }
};
static volatile sig_atomic_t interrupted;

static void stop(int signal_number) { (void)signal_number; interrupted = 1; }
static void delay_ms(long milliseconds)
{
    struct timespec wait_time;
    wait_time.tv_sec = milliseconds / 1000L;
    wait_time.tv_nsec = (milliseconds % 1000L) * 1000000L;
    (void)nanosleep(&wait_time, NULL);
}
static void leds_off(fh_gpio_t *gpio)
{
    size_t index;
    for (index = 0U; index < sizeof(leds) / sizeof(leds[0]); ++index) {
        (void)fh_gpio_mode(gpio, leds[index].pin, GPIO_OUT);
        (void)fh_gpio_write(gpio, leds[index].pin, GPIO_LED_OFF);
    }
}
static int test_leds(fh_gpio_t *gpio)
{
    size_t index;
    int failed = 0;
    puts("LEDs: sequencia visual PON, LOS, PHONE, LAN1, LAN2.");
    leds_off(gpio);
    for (index = 0U; index < sizeof(leds) / sizeof(leds[0]); ++index) {
        if (interrupted || fh_gpio_mode(gpio, leds[index].pin, GPIO_OUT) != 0 ||
            fh_gpio_write(gpio, leds[index].pin, GPIO_LED_ON) != 0) {
            fprintf(stderr, "ERRO LED %s\n", leds[index].name);
            failed = 1;
            break;
        }
        printf("  %s\n", leds[index].name);
        delay_ms(LED_DELAY_MS);
        if (fh_gpio_write(gpio, leds[index].pin, GPIO_LED_OFF) != 0) {
            fprintf(stderr, "ERRO ao desligar %s\n", leds[index].name);
            failed = 1;
            break;
        }
    }
    leds_off(gpio);
    return failed ? -1 : 0;
}
static int test_button(fh_gpio_t *gpio, const char *name, uint32_t pin)
{
    uint8_t level = GPIO_HIGH;
    if (fh_gpio_mode(gpio, pin, GPIO_IN) != 0 ||
        fh_gpio_read(gpio, pin, &level) != 0) {
        fprintf(stderr, "ERRO botao %s\n", name);
        return -1;
    }
    printf("Botao %-5s: %s (nivel=%u)\n", name,
           level == GPIO_LOW ? "pressionado" : "solto", (unsigned int)level);
    return 0;
}
static int test_i2c(void)
{
    fh_i2c_t bus;
    static const uint8_t ssd1306_nop[] = { 0x00U, 0xe3U };
    int result;
    memset(&bus, 0, sizeof(bus));
    if (fh_i2c_open(&bus) != 0 ||
        fh_i2c_speed(&bus, I2C_SPEED_400KHZ) != 0) {
        fprintf(stderr, "ERRO I2C: HAL/400kHz indisponivel\n");
        fh_i2c_close(&bus);
        return -1;
    }
    result = fh_i2c_send(&bus, OLED_ADDRESS, ssd1306_nop,
                         (uint32_t)sizeof(ssd1306_nop));
    fh_i2c_close(&bus);
    if (result != 0) {
        fprintf(stderr, "ERRO I2C: OLED 0x3c nao confirmou NOP\n");
        return -1;
    }
    puts("I2C0: OLED 0x3c confirmou comando NOP a 400kHz.");
    return 0;
}
int main(int argc, char **argv)
{
    fh_gpio_t gpio;
    int failed = 0;
    if (argc != 2 || strcmp(argv[1], "--force") != 0) {
        fprintf(stderr, "Uso: %s --force\n", argv[0]);
        return 2;
    }
    memset(&gpio, 0, sizeof(gpio));
    signal(SIGINT, stop);
    signal(SIGTERM, stop);
    if (fh_gpio_open(&gpio) != 0) {
        perror("/dev/fhdrv_kdrv_board");
        return 1;
    }
    if (test_leds(&gpio) != 0) failed = 1;
    if (test_button(&gpio, "RESET", PIN_BUTTON_RESET) != 0) failed = 1;
    if (test_button(&gpio, "LED", PIN_BUTTON_LED) != 0) failed = 1;
    fh_gpio_close(&gpio);
    if (!interrupted && test_i2c() != 0) failed = 1;
    if (interrupted) {
        puts("INTERROMPIDO: LEDs foram desligados.");
        return 130;
    }
    puts(failed ? "AUTOTESTE: FALHOU" : "AUTOTESTE: OK");
    return failed;
}
