/* AmiFunFair native display. AGA hires and interlace use the same
 * dispatcher/mailbox design as AmiPool: immutable, triple-buffered frames,
 * with each laced frame held for a matched pair of fields. */
#include "game.h"
#include "render.h"
#include "hw.h"
#include "native_video.h"
#define NBUF 3
#define COP_WORDS FAIR_COPPER_WORDS
#define COP_MEM (NBUF*2*COP_WORDS*2+96)
#define TOP 0x2c
static void *gfxbase,*old_view;
static u32 old_cop,old_lev3;
static u16 old_dmacon,old_intena,old_adkcon;
static int taken,aga,draw_buf,capacity;
static u32 screen_bytes;
static void *screen_mem[NBUF],*chip_mem;
static u8 *screen[NBUF],*fast;
static u16 *cop[NBUF][2];
static u32 *null_sprite;
static volatile u16 *dispatch;
static volatile u32 *vbr;
static volatile FairFrames frames={-1,-1};
static volatile int buffer_detail[NBUF];
volatile u32 fair_vbl_ticks;
void *fair_get_vbr(void);
void fair_lev3(void);
void clear_fast_68k(void *p,uint32_t bytes);

void *memset(void *d,int c,unsigned long n){u8 *p=d;while(n--)*p++=(u8)c;return d;}
void *memcpy(void *d,const void *s,unsigned long n){u8 *p=d;const u8 *q=s;while(n--)*p++=*q++;return d;}
static void print(const char *s){long n=0;while(s[n])n++;os_print(s,n);}
static u32 tod_b(void){u32 hi=CIAB_TODHI,mid=CIAB_TODMID,lo=CIAB_TODLO;return (hi<<16)|(mid<<8)|lo;}
uint32_t plat_lines(void){static u32 last,high;u32 now=tod_b();if(now<last)high+=1UL<<24;last=now;return high+now;}
static int vpos(void){return (int)((REGL(VPOSR)>>8)&0x1ff);}
static void wait_blit(void){(void)REG(DMACONR);while(REG(DMACONR)&0x4000);}
/* IRQ runs even while the CPU is drawing a slow hires frame. It never waits
 * for a field or a blit and never changes a frame that is being rendered. */
void fair_vblank(void){
 int field,shown;
 fair_vbl_ticks++;
 if(vpos()>=24)return;
 shown=frames.display;
 field=fair_select(&frames,REG(VPOSR),shown>=0&&buffer_detail[shown]==DETAIL_LACE);
 shown=frames.display;
 if(shown>=0)fair_dispatch_target(dispatch,(u32)cop[shown][buffer_detail[shown]==DETAIL_LACE?field:0]);
}
void platform_shutdown(void){
 int i;
 if(taken){
  wait_blit();REG(INTENA)=0x7fff;REG(DMACON)=0x7fff;REG(INTREQ)=0x7fff;
  vbr[27]=old_lev3;os_cacheclear();
  REGL(COP1LC)=old_cop;REG(COPJMP1)=0;
  REG(ADKCON)=0x8000|old_adkcon;REG(DMACON)=0x8000|old_dmacon;REG(INTENA)=0x8000|old_intena;
  gfx_loadview(gfxbase,old_view);gfx_waittof(gfxbase);gfx_waittof(gfxbase);
  os_permit();gfx_disownblitter(gfxbase);taken=0;
 }
 if(fast)os_free(fast,screen_bytes);fast=0;
 for(i=0;i<NBUF;i++){if(screen_mem[i])os_free(screen_mem[i],screen_bytes+8);screen_mem[i]=0;}
 if(chip_mem)os_free(chip_mem,COP_MEM);chip_mem=0;
 if(gfxbase)os_closelib(gfxbase);gfxbase=0;
}
int platform_init(void){
 int i,k;u8 *p;
 gfxbase=os_openlib("graphics.library",33);if(!gfxbase){print("AmiFunFair: needs graphics.library v33\n");return 0;}
 aga=(REG(0x07c)&255)==0xf8;vbr=fair_get_vbr();
 chip_mem=os_alloc(COP_MEM,MEMF_CHIP|MEMF_CLEAR);
 if(!chip_mem){print("AmiFunFair: no copper memory\n");platform_shutdown();return 0;}
 /* Try the largest supported allocation before takeover; all detail changes
  * then reuse the buffers, so a failed allocation cannot blank the game. */
 for(capacity=aga?DETAIL_LACE:DETAIL_NORMAL;capacity>=DETAIL_NORMAL;capacity--){
  screen_bytes=(u32)fair_modes[capacity].bytes;
  for(i=0;i<NBUF;i++){
   screen_mem[i]=os_alloc(screen_bytes+8,MEMF_CHIP|MEMF_CLEAR);
   if(!screen_mem[i])break;
   screen[i]=(u8 *)(((u32)screen_mem[i]+7)&~7UL);
  }
  if(i==NBUF)break;
  for(k=0;k<i;k++){os_free(screen_mem[k],screen_bytes+8);screen_mem[k]=0;}
 }
 if(capacity<DETAIL_NORMAL){print("AmiFunFair: needs about 125K free chip RAM\n");platform_shutdown();return 0;}
 fair_detail_mask=(1u<<(capacity+1))-1;
 if(!(fair_detail_mask&(1u<<fair_requested_detail)))fair_requested_detail=DETAIL_NORMAL;
 fair_detail=fair_requested_detail;
 fast=os_alloc(screen_bytes,MEMF_FAST|MEMF_CLEAR);
 p=(u8 *)(((u32)chip_mem+7)&~7UL);
 for(i=0;i<NBUF;i++)for(k=0;k<2;k++){cop[i][k]=(u16 *)(p+(i*2+k)*COP_WORDS*2);}
 dispatch=(volatile u16 *)(p+NBUF*2*COP_WORDS*2);null_sprite=(u32 *)(p+NBUF*2*COP_WORDS*2+32);
 for(i=0;i<NBUF;i++){
  buffer_detail[i]=fair_detail;fair_native_copper(cop[i][0],(u32)screen[i],(u32)null_sprite,fair_detail,0,aga,fair_palette);fair_native_copper(cop[i][1],(u32)screen[i],(u32)null_sprite,fair_detail,1,aga,fair_palette);
 }
 draw_buf=0;frames.display=1;frames.pending=-1;fair_dispatch_init(dispatch,(u32)cop[1][0]);
 old_view=*(void **)((u8 *)gfxbase+34);old_cop=*(u32 *)((u8 *)gfxbase+38);
 gfx_loadview(gfxbase,0);gfx_waittof(gfxbase);gfx_waittof(gfxbase);gfx_ownblitter(gfxbase);os_forbid();
 old_dmacon=REG(DMACONR);old_intena=REG(INTENAR);old_adkcon=REG(ADKCONR);
 REG(INTENA)=0x7fff;REG(INTREQ)=0x7fff;REG(DMACON)=0x7fff;REG(POTGO)=0xff00;
 old_lev3=vbr[27];vbr[27]=(u32)fair_lev3;os_cacheclear();taken=1;
 REGL(COP1LC)=(u32)dispatch;REG(COPJMP1)=0;
 REG(DMACON)=DMAF_SETCLR|DMAF_DMAEN|DMAF_BPLEN|DMAF_COPEN|DMAF_BLTEN|DMAF_SPREN;
 REG(INTENA)=0xc020;return 1;
}
static void clear_display(void){
 const FairDetail *d=&fair_modes[fair_detail];
 if(fast)clear_fast_68k(fast,(u32)d->bytes);
 else{
  int rows=d->h*5;u8 *dest=screen[draw_buf];
  wait_blit();REG(BLTCON0)=0x0100;REG(BLTCON1)=0;REG(BLTDMOD)=0;
  /* BLTSIZE is at most 1023 rows: a 400-line five-plane screen needs chunks. */
  while(rows){int n=rows>1000?1000:rows;REGL(BLTDPT)=(u32)dest;REG(BLTSIZE)=(u16)((n<<6)|(d->bpr/2));wait_blit();dest+=n*d->bpr;rows-=n;}
 }
}
int platform_begin(struct surf *s){
 const FairDetail *d;
 if(fair_detail_mask&(1u<<fair_requested_detail))fair_detail=fair_requested_detail;
 d=&fair_modes[fair_detail];
 r_setup_planar(fast?fast:screen[draw_buf],d->bpr,d->plane_bytes,d->w,d->h,clear_display);
 s->buf=0;s->w=d->w;s->h=d->h;s->planes=fast?fast:screen[draw_buf];s->bpr=d->bpr;s->psize=d->plane_bytes;s->pens=0;
 return 1;
}
void platform_end(void){
 const FairDetail *d=&fair_modes[fair_detail];
 if(fast){const u32 *src=(const u32 *)fast;u32 *dst=(u32 *)screen[draw_buf],n=(u32)d->bytes/16;
  while(n--){dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];dst[3]=src[3];src+=4;dst+=4;}
 }
 fair_native_copper(cop[draw_buf][0],(u32)screen[draw_buf],(u32)null_sprite,fair_detail,0,aga,fair_palette);
 fair_native_copper(cop[draw_buf][1],(u32)screen[draw_buf],(u32)null_sprite,fair_detail,1,aga,fair_palette);
 /* Metadata is committed before publishing the buffer to the IRQ. */
 REG(INTENA)=0x0020;buffer_detail[draw_buf]=fair_detail;
 draw_buf=fair_publish(&frames,draw_buf);REG(INTENA)=0x8020;
}

/* Mouse counters are signed deltas; no Intuition is active during takeover. */
void platform_input(Input *in)
{
    static int mx=160,my=100,was_left,was_right,oldx,oldy,ready;
    static uint8_t held[128];
    static uint32_t repeat_at;
    uint16_t j=REG(JOY0DAT);
    int hx=j&255,hy=j>>8,left=!(CIAA_PRA&0x40),right=!(REG(POTGOR)&0x400);
    if(!ready){oldx=hx;oldy=hy;ready=1;}
    mx+=(int8_t)(hx-oldx);my+=(int8_t)(hy-oldy);oldx=hx;oldy=hy;
    if(mx<0)mx=0;if(mx>=SCREEN_W)mx=SCREEN_W-1;
    if(my<0)my=0;if(my>=SCREEN_H)my=SCREEN_H-1;
    in->mx=mx;in->my=my;in->left=left&&!was_left;in->right=right&&!was_right;
    in->held=left;was_left=left;was_right=right;
    if(CIAA_ICR&8){
        uint8_t v=(uint8_t)~CIAA_SDR;
        int code=((v>>1)|(v<<7))&255;
        uint8_t cra=CIAA_CRA;
        uint32_t at=plat_lines();
        CIAA_CRA=cra|0x40;
        while(plat_lines()-at<2){} /* keyboard handshake, at least 85 us */
        CIAA_CRA=cra&~0x40;
        held[code&127]=!(code&128);
        if(!(code&128)){
            switch(code){
            case 0x45:in->key=KEY_ESC;break;
            case 0x13:in->key=KEY_ROTATE;break;
            case 0x31:in->key=KEY_BACKROTATE;break;
            case 0x40:in->key=KEY_PAUSE;break;
            case 0x25:case 0x50:in->key=KEY_HELP;break;
            case 0x59:in->key=KEY_QUIT;break;
            case 0x42:in->key=KEY_TAB;break;
            case 0x22:in->key=KEY_DETAIL;break;
            default:break;
            }
        }
    }
    if(platform_ms()>=repeat_at){
        if(held[0x4f])in->key=KEY_LEFT;else if(held[0x4e])in->key=KEY_RIGHT;
        else if(held[0x4c])in->key=KEY_UP;else if(held[0x4d])in->key=KEY_DOWN;
        else if(held[0x10])in->key=KEY_ORBIT_L;else if(held[0x12])in->key=KEY_ORBIT_R;
        else if(held[0x0c]||held[0x5e])in->key=KEY_ZOOM_IN;
        else if(held[0x0b]||held[0x4a])in->key=KEY_ZOOM_OUT;
        repeat_at=platform_ms()+90;
    }
}
uint32_t platform_ms(void){uint32_t n=plat_lines();return (n/125)*8+(n%125)*64/1000;}
int main(void){return game_run();}
