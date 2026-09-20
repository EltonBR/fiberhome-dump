/* Demo 16x2: PCF8574T com backpack HD44780 no mapeamento mais comum. */
#define _XOPEN_SOURCE 500
#include "fh_i2c.h"

#include <stdio.h>
#include <unistd.h>

#define PCF8574T_FIRST_ADDRESS 0x20U
#define PCF8574T_LAST_ADDRESS  0x27U
#define LCD_COLUMNS 16U

/* Backpack comum: P0=RS, P1=RW, P2=E, P3=backlight, P4..P7=D4..D7. */
#define LCD_RS        0x01U
#define LCD_ENABLE    0x04U
#define LCD_BACKLIGHT 0x08U

static int expander_write(fh_i2c_t *bus, uint8_t address, uint8_t value)
{
    const int rc = fh_i2c_send(bus, address, &value, 1U);

    if (rc != 0)
        fprintf(stderr, "I2C_WRITE_FALHOU endereco=0x%02x byte=0x%02x rc=%d\n",
                address, value, rc);
    return rc;
}

static int lcd_nibble(fh_i2c_t *bus, uint8_t address, uint8_t nibble,
                      uint8_t flags)
{
    const uint8_t value = (uint8_t)((nibble << 4) | flags | LCD_BACKLIGHT);

    if (expander_write(bus, address, value) != 0 ||
        expander_write(bus, address, (uint8_t)(value | LCD_ENABLE)) != 0) {
        return -1;
    }
    (void)usleep(1U);
    if (expander_write(bus, address, value) != 0)
        return -1;
    (void)usleep(45U);
    return 0;
}

static int lcd_byte(fh_i2c_t *bus, uint8_t address, uint8_t value,
                    uint8_t flags)
{
    return lcd_nibble(bus, address, (uint8_t)(value >> 4), flags) ||
           lcd_nibble(bus, address, (uint8_t)(value & 0x0fU), flags);
}

static int lcd_command(fh_i2c_t *bus, uint8_t address, uint8_t command)
{
    return lcd_byte(bus, address, command, 0U);
}

static int lcd_text(fh_i2c_t *bus, uint8_t address, const char *text)
{
    while (*text != '\0')
        if (lcd_byte(bus, address, (uint8_t)*text++, LCD_RS) != 0)
            return -1;
    return 0;
}

static int lcd_init(fh_i2c_t *bus, uint8_t address)
{
    /* Sequência obrigatória para passar do reset de 8 para 4 bits. */
    (void)usleep(50000U);
    if (lcd_nibble(bus, address, 0x03U, 0U) != 0)
        return -1;
    (void)usleep(4500U);
    if (lcd_nibble(bus, address, 0x03U, 0U) != 0)
        return -1;
    (void)usleep(150U);
    if (lcd_nibble(bus, address, 0x03U, 0U) != 0 ||
        lcd_nibble(bus, address, 0x02U, 0U) != 0 ||
        lcd_command(bus, address, 0x28U) != 0 || /* 4-bit, 2 linhas */
        lcd_command(bus, address, 0x08U) != 0 || /* display desligado */
        lcd_command(bus, address, 0x01U) != 0)   /* limpa */
        return -1;
    (void)usleep(2000U);
    if (lcd_command(bus, address, 0x06U) != 0 || /* cursor avança */
        lcd_command(bus, address, 0x0cU) != 0)   /* display ligado */
        return -1;
    return 0;
}

static int find_address(fh_i2c_t *bus, uint8_t *address)
{
    uint8_t candidate;
    unsigned int matches = 0U;

    for (candidate = PCF8574T_FIRST_ADDRESS;
         candidate <= PCF8574T_LAST_ADDRESS; ++candidate) {
        uint8_t received;

        if (fh_i2c_receive(bus, candidate, &received, 1U) == 0) {
            printf("PCF8574T possivel em 0x%02x (leu 0x%02x)\n",
                   candidate, received);
            *address = candidate;
            ++matches;
        }
    }
    return matches == 1U ? 0 : -1;
}

int main(void)
{
    static const char first_line[LCD_COLUMNS + 1U] = "0123456789ABCDEF";
    static const char second_line[LCD_COLUMNS + 1U] = "abcdefghijklmnop";
    fh_i2c_t bus;
    uint8_t address = 0U;

    if (fh_i2c_open(&bus) != 0 ||
        fh_i2c_speed(&bus, I2C_SPEED_100KHZ) != 0) {
        fputs("ERRO: HAL I2C indisponivel.\n", stderr);
        fh_i2c_close(&bus);
        return 1;
    }
    if (find_address(&bus, &address) != 0) {
        fputs("ERRO: esperado exatamente um PCF8574T em 0x20..0x27; sem escrita.\n",
              stderr);
        fh_i2c_close(&bus);
        return 2;
    }
    if (lcd_init(&bus, address) != 0 ||
        lcd_command(&bus, address, 0x80U) != 0 ||
        lcd_text(&bus, address, first_line) != 0 ||
        lcd_command(&bus, address, 0xc0U) != 0 ||
        lcd_text(&bus, address, second_line) != 0) {
        fputs("ERRO: falha ao escrever no LCD.\n", stderr);
        fh_i2c_close(&bus);
        return 3;
    }
    printf("DEMO_OK endereco=0x%02x clock=100kHz caracteres=32\n", address);
    fh_i2c_close(&bus);
    return 0;
}
