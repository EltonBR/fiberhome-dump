#include "fh_gpio.h"
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define DEV "/dev/fhdrv_kdrv_board"
#define MUX 0x40085300UL
#define MODE 0x40085301UL
#define WRITE 0x40085303UL
#define READ 0xc0085304UL
struct request { uint32_t pin; uint8_t level, debug, reserved[2]; };
static int call(fh_gpio_t *g,unsigned long cmd,uint32_t pin,uint8_t *level){struct request r;memset(&r,0,sizeof(r));r.pin=pin;r.level=*level;if(ioctl(g->fd,cmd,&r)<0)return -1;*level=r.level;return 0;}
int fh_gpio_open(fh_gpio_t *g){if(g==0)return -1;g->fd=open(DEV,O_RDWR);return g->fd<0?-1:0;}
void fh_gpio_close(fh_gpio_t *g){if(g&&g->fd>=0){(void)close(g->fd);g->fd=-1;}}
int fh_gpio_mode(fh_gpio_t *g,uint32_t pin,int direction){uint8_t x=GPIO_LOW;if(!g||g->fd<0||(direction!=GPIO_IN&&direction!=GPIO_OUT)||call(g,MUX,pin,&x))return -1;x=direction==GPIO_OUT?GPIO_HIGH:GPIO_LOW;return call(g,MODE,pin,&x);}
int fh_gpio_write(fh_gpio_t *g,uint32_t pin,uint8_t level){return !g||g->fd<0?-1:call(g,WRITE,pin,&level);}
int fh_gpio_read(fh_gpio_t *g,uint32_t pin,uint8_t *level){return !g||g->fd<0||!level?-1:call(g,READ,pin,level);}
