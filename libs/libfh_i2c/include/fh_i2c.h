/** @file fh_i2c.h @brief I²C0 HAL direta da SD5116. */
#ifndef FH_I2C_H
#define FH_I2C_H
#include <stdint.h>
#define I2C_BUS_0 0U
#define I2C_SPEED_100KHZ 100U
#define I2C_SPEED_400KHZ 400U
#define I2C_MAX_TRANSFER 128U
typedef struct fh_i2c { void *ubasic,*ioreactor,*ipc,*hal; int (*set_attr)(const void *); int (*send)(const void *); int (*receive)(const void *); } fh_i2c_t;
int fh_i2c_open(fh_i2c_t *bus);
void fh_i2c_close(fh_i2c_t *bus);
/** Seleciona I2C_SPEED_100KHZ ou I2C_SPEED_400KHZ. */
int fh_i2c_speed(fh_i2c_t *bus,unsigned int khz);
/** Envia até I2C_MAX_TRANSFER bytes, incluindo controle do periférico. */
int fh_i2c_send(fh_i2c_t *bus,uint8_t address,const uint8_t *data,uint32_t length);
/** Recebe até I2C_MAX_TRANSFER bytes do endereço atual do periférico. */
int fh_i2c_receive(fh_i2c_t *bus,uint8_t address,uint8_t *data,uint32_t length);
#endif
