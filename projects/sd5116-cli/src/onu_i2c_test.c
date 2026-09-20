#include "fh_i2c.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int num(const char*s,unsigned long max,unsigned long*v){char*e;errno=0;*v=strtoul(s,&e,0);return errno||!*s||*e||*v>max?-1:0;}
static void usage(const char*p){fprintf(stderr,"Uso: %s tx --force [--speed 100|400] [--repeat N] <endereco-7bit> <byte...>\n",p);}
int main(int ac,char**av){fh_i2c_t b;unsigned long speed=I2C_SPEED_100KHZ,repeat=1,address,v,i,j;uint8_t tx[I2C_MAX_TRANSFER];int n=0;if(ac<5||strcmp(av[1],"tx")||strcmp(av[2],"--force")){usage(av[0]);return 2;}for(i=3;i<(unsigned long)ac;){if(!strcmp(av[i],"--speed")&&i+1<(unsigned long)ac&&(!strcmp(av[i+1],"100")||!strcmp(av[i+1],"400"))){speed=strtoul(av[i+1],0,10);i+=2;continue;}if(!strcmp(av[i],"--repeat")&&i+1<(unsigned long)ac&&num(av[i+1],1000000UL,&repeat)==0&&repeat){i+=2;continue;}break;}if(i>=(unsigned long)ac||num(av[i++],0x77,&address)||address<3){usage(av[0]);return 2;}for(;i<(unsigned long)ac;i++){if(n==(int)sizeof(tx)||num(av[i],255,&v)){usage(av[0]);return 2;}tx[n++]=(uint8_t)v;}if(!n){usage(av[0]);return 2;}if(fh_i2c_open(&b)||fh_i2c_speed(&b,(unsigned int)speed)){fprintf(stderr,"falha ao abrir/configurar I2C\n");fh_i2c_close(&b);return 1;}for(j=0;j<repeat;j++)if(fh_i2c_send(&b,(uint8_t)address,tx,(uint32_t)n)){fprintf(stderr,"falha I2C na transacao %lu\n",j+1);fh_i2c_close(&b);return 1;}fh_i2c_close(&b);printf("I2C0 %lukHz: %d byte(s) enviados a 0x%02lx, %lu vez(es).\n",speed,n,address,repeat);return 0;}
