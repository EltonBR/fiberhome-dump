#include "fh_i2c.h"
#include <dlfcn.h>
#include <string.h>
struct attr{uint32_t index,enable,address_mode,baud;};struct packet{uint32_t address;const uint8_t*data;uint32_t length,stop;};
int fh_i2c_open(fh_i2c_t*b){if(!b)return-1;memset(b,0,sizeof(*b));b->ubasic=dlopen("/lib/hsan/so/service/libhi_ubasic.so",RTLD_NOW|RTLD_GLOBAL);b->ioreactor=b->ubasic?dlopen("/lib/hsan/so/service/libhi_ioreactor.so",RTLD_NOW|RTLD_GLOBAL):0;b->ipc=b->ioreactor?dlopen("/lib/hsan/so/service/libhi_ipc.so",RTLD_NOW|RTLD_GLOBAL):0;b->hal=b->ipc?dlopen("/lib/hsan/so/service/libhi_hal.so",RTLD_NOW|RTLD_GLOBAL):0;if(!b->hal){fh_i2c_close(b);return-1;}b->set_attr=(int(*)(const void*))dlsym(b->hal,"hi_hal_i2c_attr_set");b->send=(int(*)(const void*))dlsym(b->hal,"hi_hal_i2c_data_send");b->receive=(int(*)(const void*))dlsym(b->hal,"hi_hal_i2c_data_receive");return b->set_attr&&b->send&&b->receive?0:-1;}
void fh_i2c_close(fh_i2c_t*b){if(!b)return;if(b->hal)dlclose(b->hal);if(b->ipc)dlclose(b->ipc);if(b->ioreactor)dlclose(b->ioreactor);if(b->ubasic)dlclose(b->ubasic);memset(b,0,sizeof(*b));}
int fh_i2c_speed(fh_i2c_t*b,unsigned int khz){struct attr a;if(!b||!b->set_attr||(khz!=I2C_SPEED_100KHZ&&khz!=I2C_SPEED_400KHZ))return-1;a.index=I2C_BUS_0;a.enable=1;a.address_mode=0;a.baud=khz==I2C_SPEED_400KHZ?1U:0U;return b->set_attr(&a);}
int fh_i2c_send(fh_i2c_t*b,uint8_t addr,const uint8_t*d,uint32_t len){struct packet p;if(!b||!b->send||!d||len==0||len>I2C_MAX_TRANSFER)return-1;p.address=addr;p.data=d;p.length=len;p.stop=1;return b->send(&p);}
int fh_i2c_receive(fh_i2c_t*b,uint8_t addr,uint8_t*d,uint32_t len){struct packet p;if(!b||!b->receive||!d||len==0||len>I2C_MAX_TRANSFER)return-1;p.address=addr;p.data=d;p.length=len;p.stop=1;return b->receive(&p);}
