#include "park.h"
#include <string.h>
const ItemDef catalogue[ITEM_COUNT]={
 {"Bouncy Castle",RIDES,3,3,16,8,3500,25,2},
 {"Ghost Train",RIDES,5,4,25,16,12000,65,4},
 {"Wooden Mine Coaster",RIDES,8,5,48,24,48000,180,6},
 {"Corkscrew",RIDES,7,5,55,24,65000,240,7},
 {"Mini Mouse",RIDES,5,4,32,12,22000,90,4},
 {"Ice Cream",STANDS,2,2,8,8,1500,18,2},
 {"Candy Floss",STANDS,2,2,8,8,1200,15,2},
 {"Toilets",STANDS,2,3,14,12,2500,30,0},
 {"Coffee Shop",STANDS,3,2,9,8,2800,22,3},
 {"Fish and Chips",STANDS,3,2,12,10,3200,28,4},
 {"Tea Cups",RIDES,3,3,19,12,6500,35,3},
 {"Pirate Ship",RIDES,4,3,28,16,14000,65,4},
 {"Jets and Planes",RIDES,4,4,22,12,9000,45,3},
 {"Dodgem Cars",RIDES,4,4,28,16,11500,55,4},
 {"Horse Carousel",RIDES,4,4,24,16,9500,40,3},
 {"Helter Skelter",RIDES,3,3,20,8,5500,25,2},
 {"Railway",RIDES,7,5,26,24,18000,75,3},
 {"Ski Jump",RIDES,4,4,30,16,15500,70,4},
 {"Twister",RIDES,4,4,32,18,16500,80,4},
 {"Waltzer",RIDES,4,4,29,16,13000,60,4},
 {"Amusements",RIDES,4,3,24,16,10500,50,3}
};
static int inside(int x,int y){return x>=1&&y>=1&&x<MAP-1&&y<MAP-1;}
static int abs_i(int x){return x<0?-x:x;}
static uint32_t random_p(Park *p){p->rng=p->rng*1664525u+1013904223u;return p->rng;}
void park_message(Park *p,const char *s){int i=0;while(s[i]&&i<99){p->message[i]=s[i];i++;}p->message[i]=0;}
void park_init(Park *p,int d){
 int y;memset(p,0,sizeof(*p));p->difficulty=(uint8_t)(d<0?0:d>2?2:d);
 p->cash=p->difficulty==0?500000:p->difficulty==1?250000:100000;
 p->selected=p->pending=-1;p->buy_type=-1;p->hover_x=p->hover_y=-1;
 p->ticket=10;p->park_open=1;p->rng=0xaff2026;p->day=1;p->speed=1;
 p->camera_x=1024;p->camera_y=1050;p->camera_yaw=2300;p->camera_distance=2400;
 for(y=25;y<=30;y++)p->paths[y][GATE_X]=1;
 park_connect(p);park_message(p,"Welcome! Choose Rides or Stands below.");
}
void park_dimensions(int t,int r,int *w,int *h){
 *w=catalogue[t].w;*h=catalogue[t].h;if(r&1){int z=*w;*w=*h;*h=z;}
}
static int body_at(const Park *p,int x,int y){
 int i,w,h;for(i=0;i<MAX_ITEMS;i++)if(p->items[i].used){
  const Attraction *a=&p->items[i];park_dimensions(a->type,a->rotation,&w,&h);
  if(x>=a->x&&x<a->x+w&&y>=a->y&&y<a->y+h)return i;
 }return -1;
}
int park_at(const Park *p,int x,int y){
 int i=body_at(p,x,y);if(i>=0)return i;
 for(i=0;i<MAX_ITEMS;i++)if(p->items[i].used&&
  ((p->items[i].ex==x&&p->items[i].ey==y)||(p->items[i].xx==x&&p->items[i].xy==y)))return i;
 return -1;
}
int park_can_place(const Park *p,int t,int x,int y,int rot){
 int xx,yy,w,h;if(t<0||t>=ITEM_COUNT)return 0;park_dimensions(t,rot,&w,&h);
 if(!inside(x,y)||!inside(x+w-1,y+h-1))return 0;
 for(yy=y;yy<y+h;yy++)for(xx=x;xx<x+w;xx++)
  if(p->paths[yy][xx]||p->trees[yy][xx]||park_at(p,xx,yy)>=0||(yy>=29&&abs_i(xx-GATE_X)<=2))return 0;
 return 1;
}
void park_cancel(Park *p){
 if(p->pending>=0){Attraction *a=&p->items[p->pending];
  p->cash+=catalogue[a->type].cost;p->spent-=catalogue[a->type].cost;
  memset(a,0,sizeof(*a));if(p->selected==p->pending)p->selected=-1;
 }
 p->pending=-1;p->tool=SELECT;p->buy_type=-1;park_connect(p);
 park_message(p,"Select an attraction, or choose a build tool.");
}
int park_choose(Park *p,int t){
 if(t<0||t>=ITEM_COUNT)return 0;park_cancel(p);
 if(p->cash<catalogue[t].cost){park_message(p,"Not enough cash for this item.");return 0;}
 p->buy_type=t;p->tool=BUILD;p->rotation=0;
 park_message(p,"Place footprint. R / Z rotate. Right click cancels.");return 1;
}
int park_gate_valid(const Park *p,int id,int x,int y){
 int w,h;const Attraction *a;if(!inside(x,y)||id<0||id>=MAX_ITEMS)return 0;
 a=&p->items[id];park_dimensions(a->type,a->rotation,&w,&h);
 if(!((x==a->x-1||x==a->x+w)&&(y>=a->y&&y<a->y+h))&&
    !((y==a->y-1||y==a->y+h)&&(x>=a->x&&x<a->x+w)))return 0;
 if(park_at(p,x,y)>=0||p->trees[y][x]||(y==GATE_Y&&x==GATE_X))return 0;
 return 1;
}
int park_place(Park *p,int x,int y){
 int i;if(p->tool==BUILD){
  if(!park_can_place(p,p->buy_type,x,y,p->rotation)){park_message(p,"Footprint blocked or outside the park.");return 0;}
  if(p->cash<catalogue[p->buy_type].cost){park_message(p,"Not enough cash.");return 0;}
  for(i=0;i<MAX_ITEMS&&p->items[i].used;i++){}if(i==MAX_ITEMS){park_message(p,"Park attraction limit reached.");return 0;}
  {Attraction *a=&p->items[i];memset(a,0,sizeof(*a));
   a->used=1;a->type=(uint8_t)p->buy_type;a->x=(uint8_t)x;a->y=(uint8_t)y;
   a->rotation=(uint8_t)p->rotation;a->colour=(uint8_t)p->colour;a->speed=(uint8_t)p->speed;
   a->ex=a->ey=a->xx=a->xy=-1;a->condition=100;a->price=catalogue[a->type].price;a->open=1;
   p->cash-=catalogue[a->type].cost;p->spent+=catalogue[a->type].cost;p->selected=i;
   if(catalogue[a->type].group==RIDES){p->pending=i;p->tool=ENTRANCE;park_message(p,"Place ENTRANCE beside the ride footprint.");}
   else {a->complete=1;p->tool=SELECT;p->buy_type=-1;park_message(p,"Stand built. Connect an adjacent path.");}
  }park_connect(p);return 1;
 }
 if(p->tool==ENTRANCE||p->tool==EXIT){
  Attraction *a=&p->items[p->pending];
  if(!park_gate_valid(p,p->pending,x,y)){park_message(p,"Choose a free tile beside the ride; gates must differ.");return 0;}
  if(p->tool==ENTRANCE){a->ex=(int8_t)x;a->ey=(int8_t)y;p->tool=EXIT;park_message(p,"Place EXIT beside the ride footprint.");}
  else {a->xx=(int8_t)x;a->xy=(int8_t)y;a->complete=1;p->pending=-1;p->tool=SELECT;p->buy_type=-1;
   park_message(p,"Ride built! Add paths to BOTH gates to open it.");}
  park_connect(p);return 1;
 }
 if(p->tool==PATH)return park_path(p,x,y);
 if(p->tool==TREE)return park_tree(p,x,y);
 if(p->tool==ERASE)return park_remove(p,x,y);
 p->selected=park_at(p,x,y);return p->selected>=0;
}
int park_path(Park *p,int x,int y){
 if(!inside(x,y)||body_at(p,x,y)>=0||p->trees[y][x]){park_message(p,"Path tile blocked.");return 0;}
 if(p->paths[y][x])return 1;
 if(p->cash<20){park_message(p,"A path costs 20.");return 0;}
 p->paths[y][x]=1;p->cash-=20;p->spent+=20;park_connect(p);return 1;
}
int park_tree(Park *p,int x,int y){
 if(!inside(x,y)||park_at(p,x,y)>=0||p->paths[y][x]||p->trees[y][x]||p->cash<50)return 0;
 p->trees[y][x]=1;p->cash-=50;p->spent+=50;return 1;
}
int park_remove(Park *p,int x,int y){
 int id;if(!inside(x,y)||(x==GATE_X&&y==GATE_Y))return 0;
 id=park_at(p,x,y);if(id>=0){
  if(p->pending>=0)return 0;
  p->cash+=catalogue[p->items[id].type].cost/2;p->items[id].used=0;p->selected=-1;
  park_message(p,"Attraction sold for half its purchase price.");
 }else if(p->trees[y][x])p->trees[y][x]=0;else p->paths[y][x]=0;
 park_connect(p);return 1;
}
void park_connect(Park *p){
 uint16_t queue[MAP*MAP];int head=0,tail=0,i,x,y,d,w,h;
 static const int dx[4]={1,0,-1,0},dy[4]={0,1,0,-1};
 memset(p->reachable,0,sizeof(p->reachable));queue[tail++]=GATE_Y*MAP+GATE_X;p->reachable[GATE_Y][GATE_X]=1;
 while(head<tail){int cell=queue[head++];x=cell%MAP;y=cell/MAP;
  for(d=0;d<4;d++){int nx=x+dx[d],ny=y+dy[d];
   if(inside(nx,ny)&&p->paths[ny][nx]&&!p->reachable[ny][nx]){p->reachable[ny][nx]=1;queue[tail++]=(uint16_t)(ny*MAP+nx);}
  }
 }
 for(i=0;i<MAX_ITEMS;i++)if(p->items[i].used){Attraction *a=&p->items[i];a->connected=0;if(!a->complete)continue;
  if(catalogue[a->type].group==RIDES){
   a->connected=(uint8_t)(a->ex>=0&&a->xx>=0&&p->reachable[(int)a->ey][(int)a->ex]&&p->reachable[(int)a->xy][(int)a->xx]);
  }else{park_dimensions(a->type,a->rotation,&w,&h);
   for(y=a->y-1;y<=a->y+h;y++)for(x=a->x-1;x<=a->x+w;x++)
    if(inside(x,y)&&((x==a->x-1||x==a->x+w)^(y==a->y-1||y==a->y+h))&&p->reachable[y][x])a->connected=1;
  }
 }
 /* Multi-source BFS for stands, a single entrance source for each ride.
  * Guests follow these fields without a path search every frame. */
 for(i=0;i<=MAX_ITEMS;i++){
  Attraction *a=i<MAX_ITEMS?&p->items[i]:0;
  uint16_t (*dist)[MAP]=p->routes[i];if(a&&!a->used)continue;
  memset(dist,255,MAP*MAP*sizeof(uint16_t));
  if(a&&(!a->used||!a->connected))continue;
  head=tail=0;
  if(!a){dist[GATE_Y][GATE_X]=0;queue[tail++]=GATE_Y*MAP+GATE_X;}
  else if(catalogue[a->type].group==RIDES){dist[(int)a->ey][(int)a->ex]=0;queue[tail++]=(uint16_t)(a->ey*MAP+a->ex);}
  else {park_dimensions(a->type,a->rotation,&w,&h);
   for(y=a->y-1;y<=a->y+h;y++)for(x=a->x-1;x<=a->x+w;x++)
    if(inside(x,y)&&p->reachable[y][x]&&((x==a->x-1||x==a->x+w)^(y==a->y-1||y==a->y+h))){dist[y][x]=0;queue[tail++]=(uint16_t)(y*MAP+x);}
  }
  while(head<tail){int cell=queue[head++];x=cell%MAP;y=cell/MAP;
   for(d=0;d<4;d++){int nx=x+dx[d],ny=y+dy[d];if(inside(nx,ny)&&p->reachable[ny][nx]&&dist[ny][nx]==65535){
    dist[ny][nx]=(uint16_t)(dist[y][x]+1);queue[tail++]=(uint16_t)(ny*MAP+nx);
   }}
  }
 }
}
int park_hire(Park *p,int mechanic){
 uint8_t *n=mechanic?&p->mechanics:&p->security;
 if(*n>=20||p->cash<500){park_message(p,"Hiring costs 500; maximum 20 per role.");return 0;}
 (*n)++;p->cash-=500;p->spent+=500;park_message(p,mechanic?"Mechanic hired. Repairs rides and reduces wear.":"Security hired. Reduces daily theft losses.");return 1;
}
int park_active(const Park *p){int n=0,i;for(i=0;i<MAX_GUESTS;i++)n+=p->guests[i].used!=0;return n;}
void park_tick(Park *p){
 static const int dx[4]={1,0,-1,0},dy[4]={0,1,0,-1};
 int i,j,d,appeal=0;if(p->paused||!p->started)return;p->ticks++;
 /* A fixed 20 Hz simulation; one park day = 60 real seconds. */
 if(p->ticks%1200==0){int cost=250*p->security+350*p->mechanics;
  p->day++;for(i=0;i<MAX_ITEMS;i++)if(p->items[i].used&&p->items[i].complete){
   Attraction *a=&p->items[i];cost+=catalogue[a->type].upkeep;
   a->condition=(uint8_t)(a->condition>4+p->difficulty?a->condition-4-p->difficulty:0);
   a->condition=(uint8_t)(a->condition+p->mechanics*8>100?100:a->condition+p->mechanics*8);
  }
  cost+=(park_active(p)*(3+p->difficulty*2))/(1+p->security*3);
  p->cash-=cost;p->spent+=cost;park_message(p,"New day: wages, upkeep and theft paid.");
 }
 for(i=0;i<MAX_ITEMS;i++)if(p->items[i].used&&p->items[i].connected&&p->items[i].open&&p->items[i].condition>10)
  appeal+=catalogue[p->items[i].type].appeal;
 if(p->park_open&&appeal&&p->ticks%40==0){
  int willingness=12+appeal/8-p->difficulty*2;
  if(p->ticket<=willingness)for(i=0;i<MAX_GUESTS;i++)if(!p->guests[i].used){
   Guest *g=&p->guests[i];memset(g,0,sizeof(*g));g->used=1;g->x=g->nx=GATE_X;g->y=g->ny=GATE_Y;
   g->target=-1;g->colour=(uint8_t)(random_p(p)%6);p->cash+=p->ticket;p->earned+=p->ticket;p->arrivals++;break;
  }
 }
 for(i=0;i<MAX_GUESTS;i++)if(p->guests[i].used){Guest *g=&p->guests[i];
  g->age++;if(g->age>1800||!p->park_open)g->leaving=1;
  if(g->riding){g->riding--;if(!g->riding)g->target=-1;continue;}
  if(g->progress){g->progress--;if(!g->progress){g->x=g->nx;g->y=g->ny;}continue;}
  if(!p->reachable[g->y][g->x]){g->used=0;continue;}
  if(g->leaving&&g->x==GATE_X&&g->y==GATE_Y){g->used=0;continue;}
  if(g->target>=0){Attraction *a=&p->items[(int)g->target];
   if(!a->used||!a->open||!a->connected||a->condition<=10||a->price>catalogue[a->type].price+3)g->target=-1;
  }
  if(g->target<0&&!g->leaving){int start=(int)((random_p(p)>>16)%MAX_ITEMS);
   for(j=0;j<MAX_ITEMS;j++){int k=(start+j)%MAX_ITEMS;Attraction *a=&p->items[k];
    if(a->used&&a->open&&a->connected&&a->condition>10&&a->price<=catalogue[a->type].price+3){g->target=(int8_t)k;break;}
   }if(g->target<0)g->leaving=1;
  }
  {int route=g->leaving?MAX_ITEMS:g->target;
   uint16_t distance=p->routes[route][g->y][g->x];
   if(!distance&&!g->leaving){Attraction *a=&p->items[route];int riders=0;
    for(j=0;j<MAX_GUESTS;j++)if(p->guests[j].used&&p->guests[j].riding&&p->guests[j].target==route)riders++;
    if(riders>=catalogue[a->type].capacity)continue;
    a->visitors++;a->income+=a->price;p->cash+=a->price;p->earned+=a->price;
    g->riding=(uint16_t)(catalogue[a->type].group==RIDES?180/a->speed:40);
    if(a->ex>=0){g->x=g->nx=(uint8_t)a->xx;g->y=g->ny=(uint8_t)a->xy;}
    continue;
   }
   d=(int)(random_p(p)>>16)&3;
   for(j=0;j<4;j++){int dir=(d+j)&3,nx=g->x+dx[dir],ny=g->y+dy[dir];
    if(inside(nx,ny)&&p->reachable[ny][nx]&&p->routes[route][ny][nx]<distance){g->nx=(uint8_t)nx;g->ny=(uint8_t)ny;g->progress=8;break;}
   }
  }
 }
}
void park_demo(Park *p){
 /* Optional showcase, built through the same purchase and gate validation. */
 int x,y;park_init(p,0);p->started=1;
 for(x=4;x<=27;x++)park_path(p,x,23);
 for(y=4;y<=28;y++)park_path(p,16,y);
 for(x=3;x<=28;x++)park_path(p,x,12);
 park_choose(p,WOODEN);park_place(p,5,6);park_place(p,6,11);park_place(p,7,11);
 park_path(p,6,11);park_path(p,7,11);
 park_choose(p,CAROUSEL);park_place(p,20,18);park_place(p,20,22);park_place(p,21,22);
 park_path(p,20,22);park_path(p,21,22);
 park_choose(p,PIRATE);park_place(p,8,19);park_place(p,8,22);park_place(p,9,22);
 park_path(p,8,22);park_path(p,9,22);
 park_choose(p,TEACUPS);park_place(p,21,8);park_place(p,21,11);park_place(p,22,11);
 park_path(p,21,11);park_path(p,22,11);
 park_choose(p,ICECREAM);park_place(p,17,14);
 park_choose(p,CHIPS);park_place(p,13,19);
 for(x=3;x<28;x+=3){park_tree(p,x,25);park_tree(p,x,3);}
 park_hire(p,0);park_hire(p,1);p->selected=-1;park_message(p,"AmiFunFair showcase. Build your own fair from the bar.");
}
