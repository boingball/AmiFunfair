#include "park.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Park p;
#define TX(x) ((x)+GATE_X-16)
#define TY(y) ((y)+GATE_Y-30)
static void build_ride(void){
 park_init(&p,0);p.started=1;assert(park_choose(&p,BOUNCY));assert(park_place(&p,TX(12),TY(24)));
 assert(p.tool==ENTRANCE);assert(!park_place(&p,TX(12),TY(24)));assert(!park_place(&p,TX(10),TY(23)));
 assert(park_place(&p,TX(15),TY(24)));assert(p.tool==EXIT);assert(!park_place(&p,TX(15),TY(24)));
 assert(park_place(&p,TX(15),TY(25)));assert(p.pending==-1&&p.items[0].complete);
}
static void connect_ride(void){assert(park_path(&p,TX(15),TY(24)));assert(park_path(&p,TX(15),TY(25)));assert(park_path(&p,TX(16),TY(24)));assert(p.items[0].connected);}
int main(void){
 int d,t,r,x,y,w,h,i;int32_t cash;
 for(d=0;d<3;d++){park_init(&p,d);assert(p.cash==(d==0?500000:d==1?250000:100000));}
 assert(p.camera_yaw==CAMERA_STRAIGHT);
 for(t=0;t<ITEM_COUNT;t++)for(r=0;r<4;r++){
  park_init(&p,0);assert(park_choose(&p,t));p.rotation=r;park_dimensions(t,r,&w,&h);
  assert(w==(r&1?catalogue[t].h:catalogue[t].w));
  assert(!park_can_place(&p,t,0,0,r));assert(!park_can_place(&p,t,MAP-2,MAP-2,r));
  assert(park_place(&p,3,3));assert(p.cash==500000-catalogue[t].cost);
  for(y=3;y<3+h;y++)for(x=3;x<3+w;x++)assert(park_at(&p,x,y)==0);
  assert(!park_can_place(&p,t,3,3,r));
  if(catalogue[t].group==RIDES){
   assert(park_place(&p,3,2));assert(park_place(&p,4,2));assert(p.items[0].complete);
  }else assert(p.tool==SELECT&&p.items[0].complete);
 }
 /* Cancelling either gate stage is a complete transactional refund. */
 for(d=0;d<2;d++){
  park_init(&p,0);park_choose(&p,PIRATE);park_place(&p,3,3);if(d)park_place(&p,3,2);
  park_cancel(&p);assert(p.cash==500000&&p.spent==0);assert(!p.items[0].used);assert(park_at(&p,3,2)==-1);
 }
 build_ride();assert(!p.items[0].connected);assert(park_path(&p,TX(15),TY(24)));assert(!p.items[0].connected);
 connect_ride();cash=p.cash;assert(park_path(&p,TX(15),TY(24)));assert(p.cash==cash);
 assert(!park_path(&p,TX(12),TY(24)));assert(!park_tree(&p,TX(15),TY(24)));assert(!park_remove(&p,GATE_X,GATE_Y));
 assert(park_remove(&p,TX(16),TY(25)));assert(!p.items[0].connected);
 assert(park_path(&p,TX(16),TY(25)));assert(p.items[0].connected);
 for(i=0;i<1400;i++)park_tick(&p);assert(p.arrivals>0&&p.items[0].visitors>0&&p.items[0].income>0);
 p.paused=1;cash=p.cash;{uint32_t ticks=p.ticks;for(i=0;i<50;i++)park_tick(&p);assert(p.ticks==ticks&&p.cash==cash);}
 build_ride();connect_ride();p.ticket=100;for(i=0;i<200;i++)park_tick(&p);assert(p.arrivals==0);
 p.ticket=0;for(i=0;i<200;i++)park_tick(&p);assert(p.arrivals>0);
 p.items[0].price=100;{uint16_t before=p.items[0].visitors;for(i=0;i<200;i++)park_tick(&p);assert(p.items[0].visitors==before);}
 build_ride();connect_ride();p.park_open=0;for(i=0;i<200;i++)park_tick(&p);assert(p.arrivals==0);
 p.items[0].condition=11;p.ticks=1199;park_tick(&p);assert(p.items[0].condition<=10);
 assert(park_hire(&p,1));p.ticks=2399;park_tick(&p);assert(p.items[0].condition>10);
 cash=p.cash;assert(park_hire(&p,0));assert(p.cash==cash-500&&p.security==1);
 p.cash=0;assert(!park_hire(&p,0));assert(!park_choose(&p,CORKSCREW));assert(p.tool==SELECT);
 p.tool=PATH;assert(!park_path(&p,20,20));
 park_init(&p,0);assert(park_choose(&p,ICECREAM));assert(park_place(&p,TX(17),TY(26)));assert(p.items[0].connected);
 cash=p.cash;assert(park_remove(&p,TX(17),TY(26)));assert(p.cash==cash+catalogue[ICECREAM].cost/2);
 /* A disconnected island and a diagonal contact never count as a route. */
 build_ride();assert(park_remove(&p,TX(16),TY(25)));assert(park_path(&p,TX(15),TY(24)));assert(park_path(&p,TX(15),TY(25)));
 assert(!p.items[0].connected);assert(p.routes[0][GATE_Y][GATE_X]==65535);
 /* Paths at the same x or y as an unplaced gate must remain selectable. */
 assert(park_at(&p,-1,3)==-1);
 park_demo(&p);assert(p.started);for(i=0;i<MAX_ITEMS;i++)if(p.items[i].used)assert(p.items[i].complete&&p.items[i].connected);
 for(i=0;i<6000;i++)park_tick(&p);assert(p.day==6&&p.arrivals>0&&p.earned>0);
 park_init(&p,0);p.started=1;assert(park_choose(&p,BOUNCY));assert(park_place(&p,50,40));assert(park_place(&p,50,39));assert(park_place(&p,51,39));
 for(y=39;y<=GATE_Y;y++)assert(park_path(&p,GATE_X,y));for(x=GATE_X;x<=51;x++)assert(park_path(&p,x,39));
 assert(p.items[0].connected&&p.routes[0][GATE_Y][GATE_X]==GATE_Y-39+50-GATE_X);
 for(i=0;i<1600;i++)park_tick(&p);assert(p.items[0].visitors>0);
 puts("park: all 21 items x 4 rotations, gates, refunds, routes, guests, pricing, staff and economy passed");return 0;
}
