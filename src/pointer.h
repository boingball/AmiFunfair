#ifndef FAIR_POINTER_H
#define FAIR_POINTER_H
#include <stdint.h>
#define POINTER_EVENTS 32
#define POINTER_HEIGHT 12
#define POINTER_WORDS (2+POINTER_HEIGHT*2+2)
enum { POINTER_LEFT=1,POINTER_RIGHT=2,POINTER_KEY=3 };
typedef struct {int x,y,type,key;} PointerEvent;
typedef struct {
 int x,y,ready,oldx,oldy,buttons,head,tail;
 PointerEvent events[POINTER_EVENTS];
} FairPointer;
void pointer_init(volatile FairPointer *p);
void pointer_sample(volatile FairPointer *p,uint16_t counters,int buttons);
void pointer_key(volatile FairPointer *p,int rawkey);
int pointer_pop(volatile FairPointer *p,PointerEvent *e);
void pointer_sprite_init(uint16_t *sprite);
void pointer_sprite_move(volatile uint16_t *sprite,int x,int y);
#endif
