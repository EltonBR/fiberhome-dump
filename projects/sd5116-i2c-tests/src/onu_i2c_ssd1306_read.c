/*
 * Teste não destrutivo do caminho de leitura I2C da SD5116.
 *
 * O SSD1306 não define registros legíveis úteis pela interface I2C. Esta
 * operação envia apenas SLA+R para 0x3c e coleta quatro bytes; portanto os
 * bytes não são interpretados como estado do display. Um retorno zero prova
 * que a chamada hi_hal_i2c_data_receive foi resolvida e aceita pelo barramento.
 */
#include "fh_i2c.h"

#include <stdio.h>

#define SSD1306_ADDRESS 0x3cU

int main(void)
{
    fh_i2c_t bus;
    uint8_t data[4] = {0, 0, 0, 0};
    unsigned int i;
    int rc;

    if (fh_i2c_open(&bus) != 0) {
        fputs("ERRO: nao foi possivel carregar a HAL I2C.\n", stderr);
        return 1;
    }

    rc = fh_i2c_speed(&bus, I2C_SPEED_400KHZ);
    if (rc != 0) {
        fprintf(stderr, "ERRO: nao foi possivel selecionar 400 kHz (rc=%d).\n", rc);
        fh_i2c_close(&bus);
        return 2;
    }

    rc = fh_i2c_receive(&bus, SSD1306_ADDRESS, data, sizeof(data));
    if (rc != 0) {
        fprintf(stderr, "RECEIVE_FALHOU rc=%d\n", rc);
        fh_i2c_close(&bus);
        return 3;
    }

    printf("RECEIVE_OK endereco=0x%02x bytes=", SSD1306_ADDRESS);
    for (i = 0; i < sizeof(data); ++i)
        printf("%s%02x", i == 0 ? "" : ":", data[i]);
    putchar('\n');

    fh_i2c_close(&bus);
    return 0;
}
