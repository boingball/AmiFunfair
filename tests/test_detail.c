#include "detail.h"
#include "native_video.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Read the actual generated Copper MOVEs before the first WAIT. */
static unsigned reg_value(const uint16_t *c,unsigned reg){
 unsigned i,v=0;for(i=0;!(c[i]&1);i+=2)if(c[i]==reg)v=c[i+1];return v;
}
int main(void){
 uint16_t c[FAIR_COPPER_WORDS+2],dispatch[FAIR_DISPATCH_WORDS];uint32_t palette[32]={0};
 int mode,field,plane,row,i,draw=0,types[3]={0,0,0};FairFrames q={-1,1};
 fair_detail_mask=7;fair_requested_detail=0;
 assert(fair_cycle_detail()==1);assert(fair_cycle_detail()==2);assert(fair_cycle_detail()==0);
 fair_detail_mask=1;assert(!fair_request_detail(1));assert(!fair_request_detail(-1));assert(!fair_request_detail(3));assert(fair_cycle_detail()==0);
 fair_detail_mask=3;assert(fair_cycle_detail()==1);assert(fair_cycle_detail()==0);fair_detail_mask=7;
 for(mode=0;mode<DETAIL_COUNT;mode++){
  const FairDetail *d=&fair_modes[mode];uint8_t seen[5][400];memset(seen,0,sizeof(seen));
  for(field=0;field<(mode==DETAIL_LACE?2:1);field++){
   int n;memset(c,0xa5,sizeof(c));n=fair_native_copper(c,0x100000,0x200000,mode,field,1,palette);
   assert(n<FAIR_COPPER_WORDS&&c[FAIR_COPPER_WORDS]==0xa5a5);
   assert(reg_value(c,0x08e)==0x2c81&&reg_value(c,0x090)==0xf4c1);
   assert((reg_value(c,0x100)&0x8000)==(mode?0x8000:0));
   assert(!!(reg_value(c,0x100)&4)==(mode==DETAIL_LACE));
   assert(reg_value(c,0x094)==(mode?0xc8:0xb8));
   for(plane=0;plane<5;plane++){
    uint32_t address=(reg_value(c,0x0e0+plane*4)<<16)|reg_value(c,0x0e2+plane*4);
    unsigned modulo=reg_value(c,plane&1?0x10a:0x108);
    for(row=0;row<200;row++){
     uint32_t offset=address-0x100000-plane*d->plane_bytes;
     int picture_row=(int)(offset/d->bpr);
     assert(offset%d->bpr==0&&picture_row>=0&&picture_row<d->h);
     assert(!seen[plane][picture_row]);seen[plane][picture_row]=1;
     assert(address+d->bpr<=0x100000u+(unsigned)(plane+1)*(uint32_t)d->plane_bytes);
     address+=d->bpr+modulo;
    }
   }
  }
  for(plane=0;plane<5;plane++)for(row=0;row<d->h;row++)assert(seen[plane][row]);
 }
 fair_native_copper(c,0,0,0,0,0,palette);assert(reg_value(c,0x094)==0xd0&&reg_value(c,0x1fc)==0);
 fair_dispatch_init(dispatch,0x12345678);assert(dispatch[0]==0x2001&&dispatch[3]==0x1234&&dispatch[5]==0x5678&&dispatch[6]==0x008a);
 fair_dispatch_target(dispatch,0xabcdef00);assert(dispatch[3]==0xabcd&&dispatch[5]==0xef00);
 /* A switch into lace is allowed from a static non-laced LOF; subsequent
  * pictures are adopted only at the first field of a full pair. */
 q.display=0;q.pending=1;assert(fair_select(&q,0,0)==1&&q.display==1);
 q.pending=2;assert(fair_select(&q,0,1)==1&&q.display==1&&q.pending==2);
 assert(fair_select(&q,0x8000,1)==0&&q.display==2&&q.pending==-1);
 /* Simulated slow rendering and repeated detail changes must never reuse
  * a buffer still owned by the display or mailbox. */
 q.display=1;q.pending=-1;
 for(i=0;i<12000;i++){
  if(i%5==0){int ready=draw;assert(draw!=q.display);types[ready]=(i/5)%3;draw=fair_publish(&q,ready);}
  else fair_select(&q,i&1?0:0x8000,types[q.display]==DETAIL_LACE);
  assert(draw>=0&&draw<3&&draw!=q.display&&draw!=q.pending);
 }
 puts("detail: Copper fetches cover every row/plane once; field pairs, mode switches and fallback passed");return 0;
}
