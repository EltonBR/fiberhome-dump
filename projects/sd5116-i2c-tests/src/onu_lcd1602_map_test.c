/* Teste visual de mapeamentos PCF8574 -> HD44780. */
#define _XOPEN_SOURCE 500
#include "fh_i2c.h"

#include <stdio.h>
#include <unistd.h>

#define PCF_FIRST 0x20U
#define PCF_LAST  0x27U
#define MAP_COUNT 4U

typedef struct wiring {
    const char *name;
    uint8_t rs;
    uint8_t enable;
    uint8_t backlight;
    uint8_t data[4];
} wiring_t;

static const wiring_t mappings[MAP_COUNT] = {
    { "A", 0U, 2U, 3U, { 4U, 5U, 6U, 7U } },
    { "B", 7U, 5U, 4U, { 0U, 1U, 2U, 3U } },
    { "C", 2U, 0U, 3U, { 4U, 5U, 6U, 7U } },
    { "D", 4U, 6U, 7U, { 0U, 1U, 2U, 3U } }
};

static uint8_t compose(const wiring_t *map, uint8_t nibble, int rs, int enable)
{
    uint8_t value = (uint8_t)(1U << map->backlight);
    unsigned int bit;

    if (rs)
        value |= (uint8_t)(1U << map->rs);
    if (enable)
        value |= (uint8_t)(1U << map->enable);
    for (bit = 0U; bit < 4U; ++bit)
        if ((nibble & (1U << bit)) != 0U)
            value |= (uint8_t)(1U << map->data[bit]);
    return value;
}

static int write_byte(fh_i2c_t *bus, uint8_t address, uint8_t value)
{
    return fh_i2c_send(bus, address, &value, 1U);
}

static int nibble(fh_i2c_t *bus, uint8_t address, const wiring_t *map,
                  uint8_t value, int rs)
{
    uint8_t output = compose(map, value, rs, 0);

    if (write_byte(bus, address, output) != 0 ||
        write_byte(bus, address, compose(map, value, rs, 1)) != 0)
        return -1;
    (void)usleep(2U);
    if (write_byte(bus, address, output) != 0)
        return -1;
    (void)usleep(50U);
    return 0;
}

static int lcd_byte(fh_i2c_t *bus, uint8_t address, const wiring_t *map,
                    uint8_t value, int rs)
{
    return nibble(bus, address, map, (uint8_t)(value >> 4), rs) ||
           nibble(bus, address, map, (uint8_t)(value & 0x0fU), rs);
}

static int text(fh_i2c_t *bus, uint8_t address, const wiring_t *map,
                const char *value)
{
    while (*value != '\0')
        if (lcd_byte(bus, address, map, (uint8_t)*value++, 1) != 0)
            return -1;
    return 0;
}

static int init_display(fh_i2c_t *bus, uint8_t address, const wiring_t *map)
{
    (void)usleep(50000U);
    if (nibble(bus, address, map, 0x03U, 0) != 0)
        return -1;
    (void)usleep(4500U);
    if (nibble(bus, address, map, 0x03U, 0) != 0)
        return -1;
    (void)usleep(150U);
    if (nibble(bus, address, map, 0x03U, 0) != 0 ||
        nibble(bus, address, map, 0x02U, 0) != 0 ||
        lcd_byte(bus, address, map, 0x28U, 0) != 0 ||
        lcd_byte(bus, address, map, 0x08U, 0) != 0 ||
        lcd_byte(bus, address, map, 0x01U, 0) != 0)
        return -1;
    (void)usleep(2000U);
    return lcd_byte(bus, address, map, 0x06U, 0) ||
           lcd_byte(bus, address, map, 0x0cU, 0) ? -1 : 0;
}

static int find_pcf(fh_i2c_t *bus, uint8_t *address)
{
    unsigned int matches = 0U;
    uint8_t candidate;

    for (candidate = PCF_FIRST; candidate <= PCF_LAST; ++candidate) {
        uint8_t value;
        if (fh_i2c_receive(bus, candidate, &value, 1U) == 0) {
            *address = candidate;
            ++matches;
        }
    }
    return matches == 1U ? 0 : -1;
}

int main(void)
{
    fh_i2c_t bus;
    uint8_t address = 0U;
    unsigned int index;

    if (fh_i2c_open(&bus) != 0 ||
        fh_i2c_speed(&bus, I2C_SPEED_100KHZ) != 0 ||
        find_pcf(&bus, &address) != 0) {
        fputs("ERRO: PCF8574T unico nao encontrado; sem escrita.\n", stderr);
        fh_i2c_close(&bus);
        return 1;
    }

    for (index = 0U; index < MAP_COUNT; ++index) {
        const wiring_t *map = &mappings[index];
        char line[17] = "MAP X 012345678";

        line[4] = map->name[0];
        printf("TESTANDO_MAPA_%s\n", map->name);
        if (init_display(&bus, address, map) != 0 ||
            lcd_byte(&bus, address, map, 0x80U, 0) != 0 ||
            text(&bus, address, map, line) != 0 ||
            lcd_byte(&bus, address, map, 0xc0U, 0) != 0 ||
            text(&bus, address, map, "abcdefghijklmnop") != 0) {
            fputs("ERRO: escrita I2C falhou.\n", stderr);
            fh_i2c_close(&bus);
            return 2;
        }
        (void)usleep(4000000U);
    }

    puts("MAP_TEST_OK");
    fh_i2c_close(&bus);
    return 0;
}
