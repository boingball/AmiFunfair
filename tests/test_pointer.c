#include "pointer.h"
#include <assert.h>
#include <stdio.h>
static FairPointer p;
int main(void){
 uint16_t sprite[POINTER_WORDS+1];PointerEvent e;int x=250,y=250,i,expected=160;
 pointer_init(&p);pointer_sample(&p,(uint16_t)(y<<8|x),0);
 /* Render stalls for 20 VBlanks; hardware counters wrap several times. */
 for(i=0;i<20;i++){
  x=(x+17)&255;y=(y+3)&255;pointer_sample(&p,(uint16_t)(y<<8|x),i==4||i==5?1:0);
  expected+=17;if(expected>319)expected=319;assert(p.x==expected);
 }
 assert(p.x==319&&p.y==160);assert(pointer_pop(&p,&e)&&e.type==POINTER_LEFT&&e.x==245&&e.y==115);
 assert(!pointer_pop(&p,&e));
 for(i=0;i<10;i++){x=(x-9)&255;pointer_sample(&p,(uint16_t)(y<<8|x),0);}assert(p.x==229);
 pointer_sample(&p,(uint16_t)(y<<8|x),2);pointer_sample(&p,(uint16_t)(y<<8|x),0);pointer_key(&p,0x22);
 assert(pointer_pop(&p,&e)&&e.type==POINTER_RIGHT&&e.x==229);
 assert(pointer_pop(&p,&e)&&e.type==POINTER_KEY&&e.key==0x22);assert(!pointer_pop(&p,&e));
 for(i=0;i<100;i++)pointer_key(&p,i);for(i=0;i<POINTER_EVENTS-1;i++)assert(pointer_pop(&p,&e)&&e.key==i);assert(!pointer_pop(&p,&e));
 sprite[POINTER_WORDS]=0xdead;pointer_sprite_init(sprite);assert(sprite[POINTER_WORDS]==0xdead);
 assert(!sprite[POINTER_WORDS-1]&&!sprite[POINTER_WORDS-2]);
 for(x=0;x<=319;x+=11)for(y=0;y<=199;y+=7){
  unsigned h,v,stop;pointer_sprite_move(sprite,x,y);
  h=((sprite[0]&255)<<1)|(sprite[1]&1);v=(sprite[0]>>8)|((sprite[1]&4)<<6);
  stop=(sprite[1]>>8)|((sprite[1]&2)<<7);
  assert(h==(unsigned)x+0x81&&v==(unsigned)y+44&&stop-v==POINTER_HEIGHT);
 }
 puts("pointer: VBlank wraparound, latched click positions, queue and sprite coordinates passed");return 0;
}
