#ifndef FAIR_DETAIL_H
#define FAIR_DETAIL_H
#include <stdint.h>
enum { DETAIL_NORMAL, DETAIL_HIRES, DETAIL_LACE, DETAIL_COUNT };
#define MAX_SCREEN_W 640
#define MAX_SCREEN_H 400
typedef struct {
    int w,h,bpr;
    int32_t plane_bytes,bytes;
    const char *name;
} FairDetail;
extern const FairDetail fair_modes[DETAIL_COUNT];
extern int fair_detail,fair_requested_detail;
extern int fair_hardware_pointer;
extern unsigned fair_detail_mask;
int fair_request_detail(int mode);
int fair_cycle_detail(void);

/* A published frame stays immutable through both interlace fields. The CPU
 * masks VBlank briefly when publishing; only the IRQ selects a display. */
typedef struct { int pending,display; } FairFrames;
static inline int fair_publish(volatile FairFrames *q,int ready)
{
    int next;
    q->pending=ready;
    for(next=0;next<3;next++)if(next!=ready&&next!=q->display)return next;
    return -1;
}
static inline int fair_select(volatile FairFrames *q,unsigned vposr,int display_laced)
{
    int field=(vposr&0x8000)?0:1;
    if(q->pending>=0&&(!display_laced||field==0||q->display<0)){
        q->display=q->pending;q->pending=-1;
    }
    return field;
}
/* Copper performs the jump after line 32, rather than a CPU COPJMP during
 * rendering. Callers update only the data words, early in VBlank. */
#define FAIR_DISPATCH_WORDS 14
static inline void fair_dispatch_target(volatile uint16_t *c,uint32_t target)
{ c[7]=(uint16_t)(target>>16);c[9]=(uint16_t)target; }
static inline void fair_dispatch_pointer(volatile uint16_t *c,uint32_t sprite)
{ c[1]=(uint16_t)(sprite>>16);c[3]=(uint16_t)sprite; }
static inline void fair_dispatch_init(volatile uint16_t *c,uint32_t target)
{
    /* Sprite DMA needs its pointer before line 25, ahead of the field jump. */
    c[0]=0x0138;c[1]=0;c[2]=0x013a;c[3]=0;
    c[4]=0x2001;c[5]=0xfffe;c[6]=0x0084;c[8]=0x0086;
    c[10]=0x008a;c[11]=0;c[12]=0xffff;c[13]=0xfffe;
    fair_dispatch_target(c,target);
}
#endif
