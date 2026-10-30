#include "game.h"
#include "render.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Park p;
static uint8_t chunky[SCREEN_W*SCREEN_H],planar[SCREEN_W/8*SCREEN_H*5];
int platform_init(void){return 0;}int platform_begin(struct surf *s){(void)s;return 0;}
void platform_end(void){}void platform_shutdown(void){}void platform_input(Input *i){(void)i;}
uint32_t platform_ms(void){return 0;}
static void render_compare(void){
 struct surf c={0},s={0};Input in={0};int x,y,b;
 c.buf=chunky;c.w=SCREEN_W;c.h=SCREEN_H;
 s.w=SCREEN_W;s.h=SCREEN_H;s.planes=planar;s.bpr=SCREEN_W/8;s.psize=SCREEN_W/8*SCREEN_H;
 memset(chunky,0,sizeof(chunky));memset(planar,0,sizeof(planar));
 r_setup(chunky,SCREEN_W,SCREEN_H);scene_draw(&p,&c);ui_draw(&p,&c,&in);
 r_setup_planar(planar,SCREEN_W/8,SCREEN_W/8*SCREEN_H,SCREEN_W,SCREEN_H,0);scene_draw(&p,&s);ui_draw(&p,&s,&in);
 for(y=0;y<SCREEN_H;y++)for(x=0;x<SCREEN_W;x++){
  int v=0;for(b=0;b<5;b++)v|=((planar[b*s.psize+y*s.bpr+x/8]>>(7-(x&7)))&1)<<b;
  assert(chunky[y*SCREEN_W+x]==v);
 }
}
int main(void){
 int i,t,r,x,y,sx,sy,checks=0;Input in={0};struct pvert v;
 park_demo(&p);render_compare();
 /* Every ride model draws identically through the PC chunky and Amiga planar paths. */
 for(t=0;t<ITEM_COUNT;t++)for(r=0;r<4;r++){
  park_init(&p,0);p.started=1;park_choose(&p,t);p.rotation=r;assert(park_place(&p,10,10));
  p.tool=SELECT;p.pending=-1;p.items[0].complete=1;p.items[0].connected=1;p.ticks=173;
  p.camera_x=800;p.camera_y=800;p.camera_distance=1400;p.camera_yaw=2300+r*700;render_compare();
 }
 /* Mouse picking round-trips projected tile centres, across camera directions/zoom. */
 park_init(&p,0);for(i=0;i<8;i++){
  p.camera_yaw=i*512;p.camera_distance=1200+i*160;r_setup(chunky,SCREEN_W,SCREEN_H);scene_camera(&p);
  for(y=6;y<27;y+=4)for(x=6;x<27;x+=4){
   r_pvert(x*64+32,y*64+32,0,&v);sx=v.sx>>4;sy=v.sy>>4;
   if(v.code||sx<0||sx>=SCREEN_W||sy<VIEW_TOP||sy>=VIEW_BOTTOM)continue;
   {int tx,ty;assert(scene_pick(&p,sx,sy,&tx,&ty));assert(tx==x&&ty==y);checks++;}
  }
 }assert(checks>50);
 /* Start menu, bottom tabs/cards, inspector changes, rotate and cancellation. */
 park_init(&p,0);in.left=1;in.mx=140;in.my=105;game_input(&p,&in);assert(p.started&&p.cash==250000);
 in.mx=50;in.my=165;game_input(&p,&in);assert(p.tool==BUILD&&p.buy_type==BOUNCY);
 in.left=0;in.key=KEY_ROTATE;game_input(&p,&in);assert(p.rotation==1);
 in.key=KEY_ESC;game_input(&p,&in);assert(p.tool==SELECT&&p.pending<0);
 in.key=KEY_NONE;in.left=1;in.mx=190;in.my=190;game_input(&p,&in);assert(p.category==LANDSCAPE);
 in.mx=45;in.my=163;game_input(&p,&in);assert(p.tool==PATH);
 in.mx=245;in.my=190;game_input(&p,&in);assert(p.category==PARK_MENU);
 in.mx=99;in.my=160;game_input(&p,&in);assert(p.ticket==11);
 in.mx=290;in.my=190;game_input(&p,&in);assert(p.show_help);
 puts("scene/UI: 84 model/rotation planar comparisons, camera picking and UI flow passed");return 0;
}
