#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UBASIC "/lib/hsan/so/service/libhi_ubasic.so"
#define IOREACTOR "/lib/hsan/so/service/libhi_ioreactor.so"
#define IPC "/lib/hsan/so/service/libhi_ipc.so"
#define HAL "/lib/hsan/so/service/libhi_hal.so"
#define W 128U
#define H 64U
#define PAGES 8U
struct attr { uint32_t index, enable, address_mode, baud; };
struct send { uint32_t address; const uint8_t *data; uint32_t length, stop; };
typedef int (*set_fn)(const struct attr *);
typedef int (*send_fn)(const struct send *);
static uint8_t frame[W * PAGES];
static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t le16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static int read_all(int fd, void *ptr, size_t size) { uint8_t *p = ptr; while (size) { ssize_t n = read(fd,p,size); if (n > 0) { p += n; size -= (size_t)n; } else if (n < 0 && errno == EINTR) continue; else return -1; } return 0; }
static int baud(set_fn set, uint32_t value) { struct attr a = {0U,1U,0U,value}; return set(&a); }
static int sendbuf(send_fn send, const uint8_t *p, uint32_t n) { struct send s = {0x3cU,p,n,1U}; return send(&s); }
static const uint8_t *glyph(char c)
{
 static const uint8_t b[5]={0,0,0,0,0};
 static const uint8_t l[26][5]={{0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},{0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},{0x3e,0x41,0x49,0x49,0x7a},{0x7f,0x08,0x08,0x08,0x7f},{0,0x41,0x7f,0x41,0},{0x20,0x40,0x41,0x3f,1},{0x7f,8,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},{0x7f,2,0x0c,2,0x7f},{0x7f,4,8,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},{0x7f,9,9,9,6},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,9,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{1,1,0x7f,1,1},{0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,8,0x14,0x63},{7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43}};
 static const uint8_t d[10][5]={{0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1e}};
 static const uint8_t dot[5]={0,0x60,0x60,0,0}, colon[5]={0,0x36,0x36,0,0}, dash[5]={8,8,8,8,8}, slash[5]={0x20,0x10,8,4,2}, gt[5]={0,0x41,0x22,0x14,8};
 if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
 if (c >= 'A' && c <= 'Z') return l[c - 'A'];
 if (c >= '0' && c <= '9') return d[c - '0'];
 if (c == '.') return dot;
 if (c == ':') return colon;
 if (c == '-') return dash;
 if (c == '/') return slash;
 if (c == '>') return gt;
 return b;
}
static void text_line(unsigned int line,const char *text)
{ unsigned int col=0U; while(*text && col<21U) { const uint8_t *g=glyph(*text++); unsigned int i; for(i=0;i<5U;i++) frame[line*W+col*6U+i]=g[i]; ++col; } }
static int load_bmp(const char *path)
{
 uint8_t head[54], row[512]; int fd=open(path,O_RDONLY); uint32_t off,w,habs,stride; int32_t hs; uint16_t bpp; unsigned int y,x;
 if(fd<0||read_all(fd,head,sizeof(head))||head[0]!='B'||head[1]!='M'||le32(head+14)<40U){if(fd>=0)close(fd);return -1;} off=le32(head+10); w=le32(head+18); hs=(int32_t)le32(head+22); habs=(uint32_t)(hs<0?-hs:hs); bpp=le16(head+28); if(w!=W||habs!=H||le16(head+26)!=1U||(bpp!=1U&&bpp!=24U&&bpp!=32U)||le32(head+30)!=0U){close(fd);return -1;} stride=((w*(uint32_t)bpp+31U)/32U)*4U; if(stride>sizeof(row)||lseek(fd,(off_t)off,SEEK_SET)<0){close(fd);return -1;}
 memset(frame,0,sizeof(frame)); for(y=0;y<H;y++){unsigned int dy=hs>0?(H-1U-y):y; if(read_all(fd,row,stride)){close(fd);return -1;} for(x=0;x<W;x++){int on; if(bpp==1U)on=(row[x/8U]&(0x80U>>(x%8U)))!=0; else {unsigned int p=x*(bpp/8U); unsigned int lum=(uint32_t)row[p]*11U+(uint32_t)row[p+1U]*59U+(uint32_t)row[p+2U]*30U; on=lum<12800U;} if(on)frame[(dy/8U)*W+x]|=(uint8_t)(1U<<(dy%8U)); }} close(fd); return 0;
}
static int render(send_fn send)
{ static const uint8_t init[]={0,0xae,0xd5,0x80,0xa8,0x3f,0xd3,0,0x40,0x8d,0x14,0x20,0,0xa1,0xc8,0xda,0x12,0x81,0xcf,0xd9,0xf1,0xdb,0x40,0xa4,0xa6,0x2e,0xaf}; static const uint8_t window[]={0,0x21,0,0x7f,0x22,0,7}; uint8_t part[128],tail[2]; unsigned int p; if(sendbuf(send,init,sizeof(init))||sendbuf(send,window,sizeof(window)))return -1; for(p=0;p<PAGES;p++){part[0]=0x40;memcpy(part+1,frame+p*W,W-1U);tail[0]=0x40;tail[1]=frame[p*W+W-1U];if(sendbuf(send,part,sizeof(part))||sendbuf(send,tail,sizeof(tail)))return -1;}return 0; }
int main(int argc,char **argv)
{ void *u,*r,*i,*h; set_fn set; send_fn send; int ok=0;
 if(argc<3||strcmp(argv[1],"--force")){fprintf(stderr,"Uso: %s --force text <linha...> | bmp <arquivo.bmp>\n",argv[0]);return 2;} memset(frame,0,sizeof(frame)); if(!strcmp(argv[2],"text")&&argc>=4){int n;for(n=3;n<argc&&n<11;n++)text_line((unsigned int)(n-3),argv[n]);}else if(!strcmp(argv[2],"bmp")&&argc==4){if(load_bmp(argv[3])){fprintf(stderr,"BMP precisa ser BI_RGB 128x64, 1/24/32 bpp\n");return 1;}}else{return 2;}
 u=dlopen(UBASIC,RTLD_NOW|RTLD_GLOBAL);r=u?dlopen(IOREACTOR,RTLD_NOW|RTLD_GLOBAL):NULL;i=r?dlopen(IPC,RTLD_NOW|RTLD_GLOBAL):NULL;h=i?dlopen(HAL,RTLD_NOW|RTLD_GLOBAL):NULL;if(!h){fprintf(stderr,"HAL I2C indisponivel: %s\n",dlerror());return 1;}set=(set_fn)dlsym(h,"hi_hal_i2c_attr_set");send=(send_fn)dlsym(h,"hi_hal_i2c_data_send");if(set&&send&&baud(set,1U)==0&&render(send)==0)ok=1;else fprintf(stderr,"falha I2C/OLED\n");if(set) (void)baud(set,0U);(void)dlclose(h);(void)dlclose(i);(void)dlclose(r);(void)dlclose(u);return ok?0:1; }
