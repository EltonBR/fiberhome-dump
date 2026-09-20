#include "fh_i2c_lcd.h"
#include <string.h>
int fh_i2c_lcd_init(fh_i2c_lcd_t*l,fh_i2c_t*b,uint8_t a){static const uint8_t init[]={0,0xae,0xd5,0x80,0xa8,0x3f,0xd3,0,0x40,0x8d,0x14,0x20,0,0xa1,0xc8,0xda,0x12,0x81,0xcf,0xd9,0xf1,0xdb,0x40,0xa4,0xa6,0x2e,0xaf};if(!l||!b||a<3U||a>0x77U)return-1;l->bus=b;l->address=a;memset(l->frame,0,sizeof(l->frame));return fh_i2c_send(b,a,init,sizeof(init));}
void fh_i2c_lcd_clear(fh_i2c_lcd_t*l){if(l)memset(l->frame,0,sizeof(l->frame));}
void fh_i2c_lcd_pixel(fh_i2c_lcd_t*l,unsigned int x,unsigned int y,int on){uint8_t mask;if(!l||x>=LCD_WIDTH||y>=LCD_HEIGHT)return;mask=(uint8_t)(1U<<(y%8U));if(on)l->frame[(y/8U)*LCD_WIDTH+x]|=mask;else l->frame[(y/8U)*LCD_WIDTH+x]&=(uint8_t)~mask;}
int fh_i2c_lcd_present(fh_i2c_lcd_t*l){static const uint8_t win[]={0,0x21,0,0x7f,0x22,0,7};uint8_t p[128],tail[2];unsigned int page;if(!l||fh_i2c_send(l->bus,l->address,win,sizeof(win)))return-1;for(page=0;page<8U;page++){p[0]=0x40;memcpy(p+1,l->frame+page*128U,127U);tail[0]=0x40;tail[1]=l->frame[page*128U+127U];if(fh_i2c_send(l->bus,l->address,p,sizeof(p))||fh_i2c_send(l->bus,l->address,tail,sizeof(tail)))return-1;}return 0;}
