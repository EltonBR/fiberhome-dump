/**
 * @file fh_gpio.h
 * @brief GPIO da FiberHome SD5116 via /dev/fhdrv_kdrv_board.
 *
 * A ABI foi confirmada na unidade analisada. Todas as funções retornam 0 em
 * sucesso e -1 em erro; errno é preservado quando a chamada de sistema o faz.
 */
#ifndef FH_GPIO_H
#define FH_GPIO_H

#include <stdint.h>

/** Níveis elétricos da interface GPIO. */
#define GPIO_LOW   ((uint8_t)0U)
#define GPIO_HIGH  ((uint8_t)1U)

/** Direção do pino. */
#define GPIO_IN    0
#define GPIO_OUT   1

/** Pinos confirmados nesta PCB SD5116. */
#define PIN_LED_PHONE  ((uint32_t)6U)
#define PIN_LED_LAN1   ((uint32_t)12U)
#define PIN_LED_LAN2   ((uint32_t)13U)
#define PIN_BUTTON_LED ((uint32_t)14U)
#define PIN_LED_LOS    ((uint32_t)30U)
#define PIN_LED_PON    ((uint32_t)31U)
#define PIN_BUTTON_RESET ((uint32_t)32U)

/** LEDs conectados diretamente ao SoC acendem em GPIO_LOW. */
#define GPIO_LED_ON    GPIO_LOW
#define GPIO_LED_OFF   GPIO_HIGH

typedef struct fh_gpio {
    int fd; /**< Descritor interno; não modificar diretamente. */
} fh_gpio_t;

/** Abre o dispositivo GPIO. Inicialize a estrutura apenas com esta função. */
int fh_gpio_open(fh_gpio_t *gpio);
/** Fecha o dispositivo; pode ser chamada após falha parcial de inicialização. */
void fh_gpio_close(fh_gpio_t *gpio);
/** Configura mux e direção de @p pin. Use GPIO_IN ou GPIO_OUT. */
int fh_gpio_mode(fh_gpio_t *gpio, uint32_t pin, int direction);
/** Escreve GPIO_LOW ou GPIO_HIGH em pino previamente configurado como saída. */
int fh_gpio_write(fh_gpio_t *gpio, uint32_t pin, uint8_t level);
/** Lê GPIO_LOW ou GPIO_HIGH de pino previamente configurado como entrada. */
int fh_gpio_read(fh_gpio_t *gpio, uint32_t pin, uint8_t *level);

#endif
