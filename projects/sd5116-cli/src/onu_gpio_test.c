#include "fh_gpio.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static volatile sig_atomic_t stopped;
static void stop(int sig){(void)sig;stopped=1;}
static int num(const char*s,unsigned long max,unsigned long*v){char*e;errno=0;*v=strtoul(s,&e,0);return errno||!*s||*e||*v>max?-1:0;}
static void wait_us(unsigned long us){struct timespec t;t.tv_sec=(time_t)(us/1000000UL);t.tv_nsec=(long)((us%1000000UL)*1000UL);(void)nanosleep(&t,0);}
static void usage(const char*p){fprintf(stderr,"Uso:\n  %s read <pino>\n  %s write --force <pino> <low|high>\n  %s tx --force <pino> <repeticoes> <intervalo-us>\n",p,p,p);}
int main(int ac,char**av){fh_gpio_t g;unsigned long pin,count,delay,i;uint8_t level;if(ac<3){usage(av[0]);return 2;}
if(!strcmp(av[1],"read")&&ac==3&&num(av[2],255,&pin)==0){if(fh_gpio_open(&g)||fh_gpio_mode(&g,(uint32_t)pin,GPIO_IN)||fh_gpio_read(&g,(uint32_t)pin,&level)){perror("GPIO");return 1;}printf("GPIO %lu = %s\n",pin,level?"high":"low");fh_gpio_close(&g);return 0;}
if(!strcmp(av[1],"write")&&ac==5&&!strcmp(av[2],"--force")&&num(av[3],255,&pin)==0&&(!strcmp(av[4],"low")||!strcmp(av[4],"high"))){level=!strcmp(av[4],"high")?GPIO_HIGH:GPIO_LOW;if(fh_gpio_open(&g)||fh_gpio_mode(&g,(uint32_t)pin,GPIO_OUT)||fh_gpio_write(&g,(uint32_t)pin,level)){perror("GPIO");return 1;}fh_gpio_close(&g);return 0;}
if(!strcmp(av[1],"tx")&&ac==6&&!strcmp(av[2],"--force")&&num(av[3],255,&pin)==0&&num(av[4],1000000UL,&count)==0&&count>0&&num(av[5],10000000UL,&delay)==0){if(fh_gpio_open(&g)||fh_gpio_mode(&g,(uint32_t)pin,GPIO_OUT)){perror("GPIO");return 1;}signal(SIGINT,stop);for(i=0;i<count&&!stopped;i++){if(fh_gpio_write(&g,(uint32_t)pin,GPIO_LOW)){perror("GPIO");break;}wait_us(delay);if(fh_gpio_write(&g,(uint32_t)pin,GPIO_HIGH)){perror("GPIO");break;}wait_us(delay);}(void)fh_gpio_write(&g,(uint32_t)pin,GPIO_HIGH);fh_gpio_close(&g);return i==count||stopped?0:1;}usage(av[0]);return 2;}
