/** @file fh_gpio_spi.h @brief SPI por software sobre libfh_gpio. */
#ifndef FH_GPIO_SPI_H
#define FH_GPIO_SPI_H
#include <stddef.h>
#include <stdint.h>
#include "fh_gpio.h"

#define SPI_MODE_0 0U
#define SPI_MODE_1 1U
#define SPI_MODE_2 2U
#define SPI_MODE_3 3U
#define SPI_DELAY_SAFE_US 100UL
#define SPI_DELAY_TEST_US 10UL
#define SPI_DEFAULT_CS PIN_LED_PHONE
#define SPI_DEFAULT_SCLK PIN_LED_PON
#define SPI_DEFAULT_MOSI PIN_LED_LOS
#define SPI_DEFAULT_MISO PIN_LED_LAN1

typedef struct fh_gpio_spi { fh_gpio_t gpio; uint32_t cs, sclk, mosi, miso; unsigned int mode; unsigned long delay_us; } fh_gpio_spi_t;
/** Abre SPI software. delay_us é mínimo entre bordas, não frequência exata. */
int fh_gpio_spi_open(fh_gpio_spi_t *spi,uint32_t cs,uint32_t sclk,uint32_t mosi,uint32_t miso,unsigned int mode,unsigned long delay_us);
/** Transfere @p count bytes, mantendo CS em GPIO_LOW até o fim. */
int fh_gpio_spi_transfer(fh_gpio_spi_t *spi,const uint8_t *tx,uint8_t *rx,size_t count);
/** Libera GPIOs e deixa CS em GPIO_HIGH. */
void fh_gpio_spi_close(fh_gpio_spi_t *spi);
#endif
