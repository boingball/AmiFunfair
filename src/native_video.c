#include "native_video.h"
#include "detail.h"
static uint16_t hi12(uint32_t c){return (uint16_t)(((c>>12)&0xf00)|((c>>8)&0xf0)|((c>>4)&15));}
static uint16_t lo12(uint32_t c){return (uint16_t)(((c>>8)&0xf00)|((c>>4)&0xf0)|(c&15));}
#define MOVE(r,v) do{*c++=(uint16_t)(r);*c++=(uint16_t)(v);}while(0)
static uint16_t *colour(uint16_t *c,int reg,uint32_t rgb,int aga){
 if(aga){MOVE(0x106,0x0c00);MOVE(reg,hi12(rgb));MOVE(0x106,0x0e00);MOVE(reg,lo12(rgb));}
 else{MOVE(reg,hi12(rgb));}return c;
}
int fair_native_copper(uint16_t *out,uint32_t planes,uint32_t ns,int mode,int field,int aga,const uint32_t *palette){
 uint16_t *c=out;const FairDetail *d=&fair_modes[mode];int i,lace=mode==DETAIL_LACE;
 MOVE(0x1fc,aga?3:0);MOVE(0x100,0x5200|(mode!=DETAIL_NORMAL?0x8000:0)|(lace?4:0));
 MOVE(0x102,0);MOVE(0x104,0x0024);MOVE(0x106,0x0c00);MOVE(0x10c,0x0011);
 /* 200 display lines, starting at 44, on PAL and NTSC. In lace each
  * field fetches alternate rows of the 400-line rendered picture. */
 MOVE(0x08e,0x2c81);MOVE(0x090,0xf4c1);
 MOVE(0x092,0x38);MOVE(0x094,mode!=DETAIL_NORMAL?0xc8:aga?0xb8:0xd0);
 MOVE(0x108,lace?d->bpr:0);MOVE(0x10a,lace?d->bpr:0);
 for(i=0;i<8;i++){MOVE(0x120+i*4,ns>>16);MOVE(0x122+i*4,ns&65535);}
 for(i=0;i<32;i++)c=colour(c,0x180+i*2,palette[i],aga);
 for(i=0;i<5;i++){
  uint32_t a=planes+i*d->plane_bytes+(lace?field*d->bpr:0);
  MOVE(0x0e0+i*4,a>>16);MOVE(0x0e2+i*4,a&65535);
 }
 *c++=0xf407;*c++=0xfffe;
 MOVE(0x100,0x0200|(lace?4:0));c=colour(c,0x180,0,aga);
 *c++=0xffff;*c++=0xfffe;return (int)(c-out);
}
