/* Varredura I2C exclusivamente por transações de leitura (SLA+R). */
#include "fh_i2c.h"

#include <stdio.h>

#define I2C_FIRST_USABLE_ADDRESS 0x03U
#define I2C_LAST_USABLE_ADDRESS  0x77U

int main(void)
{
    fh_i2c_t bus;
    unsigned int address;
    unsigned int found = 0U;

    if (fh_i2c_open(&bus) != 0 ||
        fh_i2c_speed(&bus, I2C_SPEED_100KHZ) != 0) {
        fputs("ERRO: HAL I2C indisponivel.\n", stderr);
        fh_i2c_close(&bus);
        return 1;
    }

    puts("SCAN I2C0 100 kHz: somente leitura, faixa 0x03..0x77");
    for (address = I2C_FIRST_USABLE_ADDRESS;
         address <= I2C_LAST_USABLE_ADDRESS; ++address) {
        uint8_t value = 0U;

        if (fh_i2c_receive(&bus, (uint8_t)address, &value, 1U) == 0) {
            printf("ACK 0x%02x leitura=0x%02x\n", address, value);
            ++found;
        }
    }
    printf("SCAN_CONCLUIDO respostas=%u\n", found);
    fh_i2c_close(&bus);
    return 0;
}
