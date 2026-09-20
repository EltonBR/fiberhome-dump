/** @file fh_i2c_lcd.h @brief Framebuffer SSD1306 I²C 128×64. */
#ifndef FH_I2C_LCD_H
#define FH_I2C_LCD_H
#include <stdint.h>
#include "fh_i2c.h"
#define LCD_WIDTH 128U
#define LCD_HEIGHT 64U
#define LCD_PAGES 8U
#define LCD_FRAME_BYTES 1024U
#define LCD_DEFAULT_ADDRESS 0x3cU
typedef struct fh_i2c_lcd { fh_i2c_t *bus; uint8_t address; uint8_t frame[LCD_FRAME_BYTES]; } fh_i2c_lcd_t;
int fh_i2c_lcd_init(fh_i2c_lcd_t *lcd,fh_i2c_t *bus,uint8_t address);
void fh_i2c_lcd_clear(fh_i2c_lcd_t *lcd);
void fh_i2c_lcd_pixel(fh_i2c_lcd_t *lcd,unsigned int x,unsigned int y,int on);
int fh_i2c_lcd_present(fh_i2c_lcd_t *lcd);
#endif
