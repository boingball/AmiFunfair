#include "pointer.h"
static void event(volatile FairPointer *p,int type,int key){
 int next=(p->head+1)%POINTER_EVENTS;
 if(next==p->tail)return;
 p->events[p->head].x=p->x;p->events[p->head].y=p->y;
 p->events[p->head].type=type;p->events[p->head].key=key;p->head=next;
}
void pointer_init(volatile FairPointer *p){
 p->x=160;p->y=100;p->ready=0;p->buttons=0;p->head=p->tail=0;
}
void pointer_sample(volatile FairPointer *p,uint16_t counters,int buttons){
 int x=counters&255,y=counters>>8;
 if(p->ready){p->x+=(int8_t)(x-p->oldx);p->y+=(int8_t)(y-p->oldy);}
 p->oldx=x;p->oldy=y;p->ready=1;
 if(p->x<0)p->x=0;if(p->x>319)p->x=319;
 if(p->y<0)p->y=0;if(p->y>199)p->y=199;
 if((buttons&1)&&!(p->buttons&1))event(p,POINTER_LEFT,0);
 if((buttons&2)&&!(p->buttons&2))event(p,POINTER_RIGHT,0);
 p->buttons=buttons;
}
void pointer_key(volatile FairPointer *p,int rawkey){event(p,POINTER_KEY,rawkey);}
int pointer_pop(volatile FairPointer *p,PointerEvent *e){
 if(p->head==p->tail)return 0;
 e->x=p->events[p->tail].x;e->y=p->events[p->tail].y;
 e->type=p->events[p->tail].type;e->key=p->events[p->tail].key;
 p->tail=(p->tail+1)%POINTER_EVENTS;return 1;
}
void pointer_sprite_move(volatile uint16_t *s,int x,int y){
 unsigned h=(unsigned)(x+0x81),v=(unsigned)(y+44),stop=v+POINTER_HEIGHT;
 s[0]=(uint16_t)((v&255)<<8|(h>>1));
 s[1]=(uint16_t)((stop&255)<<8|((v>>6)&4)|((stop>>7)&2)|(h&1));
}
void pointer_sprite_init(uint16_t *s){
 int row,x;for(row=0;row<POINTER_WORDS;row++)s[row]=0;
 for(row=0;row<POINTER_HEIGHT;row++){
  unsigned a=0,b=0;int width=row<9?row/2+1:3;
  for(x=0;x<width;x++){
   /* Pens 1 and 3: dark outline, bright fill; lowres 16-pixel sprite. */
   unsigned pen=(x==0||x==width-1||row==8)?1:3;
   if(pen&1)a|=0x8000u>>x;if(pen&2)b|=0x8000u>>x;
  }s[2+row*2]=(uint16_t)a;s[3+row*2]=(uint16_t)b;
 }pointer_sprite_move(s,160,100);
}
