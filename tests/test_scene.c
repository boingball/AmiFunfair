#include "game.h"
#include "render.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Park p;
static uint8_t chunky[MAX_SCREEN_W*MAX_SCREEN_H],planar[MAX_SCREEN_W/8*MAX_SCREEN_H*5];
static uint8_t cache[MAX_SCREEN_W/8*MAX_SCREEN_H*5];
int platform_init(void){return 0;}int platform_begin(struct surf *s){(void)s;return 0;}
void platform_end(void){}void platform_shutdown(void){}void platform_input(Input *i){(void)i;}
uint32_t platform_ms(void){return 0;}
static void render_compare(int mode){
 const FairDetail *d=&fair_modes[mode];fair_detail=mode;
 struct surf c={0},s={0};Input in={0};int x,y,b;
 c.buf=chunky;c.w=d->w;c.h=d->h;
 s.w=d->w;s.h=d->h;s.planes=planar;s.bpr=d->bpr;s.psize=d->plane_bytes;
 memset(chunky,0,sizeof(chunky));memset(planar,0,sizeof(planar));
 r_setup(chunky,d->w,d->h);scene_draw(&p,&c);ui_draw(&p,&c,&in);
 scene_cache_bind(cache,sizeof(cache));
 r_setup_planar(planar,d->bpr,d->plane_bytes,d->w,d->h,0);scene_draw(&p,&s);ui_draw(&p,&s,&in);
 /* The second draw restores cached terrain. It must also erase the old UI. */
 r_setup_planar(planar,d->bpr,d->plane_bytes,d->w,d->h,0);scene_draw(&p,&s);ui_draw(&p,&s,&in);
 for(y=0;y<d->h;y++)for(x=0;x<d->w;x++){
  int v=0;for(b=0;b<5;b++)v|=((planar[b*s.psize+y*s.bpr+x/8]>>(7-(x&7)))&1)<<b;
  assert(chunky[y*d->w+x]==v);
 }
 scene_cache_bind(0,0);
}
static void cache_invalidation(void){
 static Park saved;static uint8_t expected[MAX_SCREEN_W*MAX_SCREEN_H];
 struct surf s={0};int kind;s.buf=chunky;s.w=320;s.h=200;
 park_demo(&p);saved=p;
 for(kind=0;kind<6;kind++){
  p=saved;
  if(kind==0)p.camera_x+=100;if(kind==1)p.camera_yaw+=400;if(kind==2)p.camera_distance-=500;
  if(kind==3)p.paths[GATE_Y-7][GATE_X-11]=0;if(kind==4)p.reachable[GATE_Y-7][GATE_X-11]=0;if(kind==5)p.camera_y-=130;
  scene_cache_bind(0,0);r_setup(chunky,320,200);scene_draw(&p,&s);memcpy(expected,chunky,320*200);
  {static Park changed;changed=p;p=saved;scene_cache_bind(cache,sizeof(cache));r_setup(chunky,320,200);scene_draw(&p,&s);p=changed;}
  r_setup(chunky,320,200);scene_draw(&p,&s);assert(!memcmp(expected,chunky,320*200));
 }
 scene_cache_bind(0,0);
}
static void view_and_pan_checks(void){
 Input in={0};int mode,yaw,x,y,sx,sy,tx,ty;struct pvert before,after;
 for(mode=0;mode<2;mode++)for(yaw=0;yaw<4096;yaw+=512){
  park_init(&p,0);p.started=1;p.view_2d=mode;p.camera_yaw=yaw;p.camera_x=p.camera_y=2048;
  x=p.camera_x;y=p.camera_y;r_setup(chunky,320,200);
  if(mode)topdown_project(&p,x,y,&sx,&sy);else{scene_camera(&p);r_pvert(x,y,0,&before);sx=before.sx>>4;sy=before.sy>>4;}
  in.key=KEY_RIGHT;game_input(&p,&in);
  if(mode)topdown_project(&p,x,y,&tx,&ty);else{scene_camera(&p);r_pvert(x,y,0,&after);tx=after.sx>>4;ty=after.sy>>4;}
  assert(tx<sx); /* camera right reveals right-hand content */
  p.camera_x=x;p.camera_y=y;in.key=KEY_UP;game_input(&p,&in);
  if(mode)topdown_project(&p,x,y,&tx,&ty);else{scene_camera(&p);r_pvert(x,y,0,&after);tx=after.sx>>4;ty=after.sy>>4;}
  assert(ty>sy);
  in.key=KEY_DOWN;game_input(&p,&in);assert(p.camera_x==x&&p.camera_y==y);
  in.key=KEY_LEFT;game_input(&p,&in);in.key=KEY_RIGHT;game_input(&p,&in);assert(p.camera_x==x&&p.camera_y==y);
 }
 park_demo(&p);p.view_2d=1;
 for(mode=0;mode<DETAIL_COUNT;mode++){
  const FairDetail *d=&fair_modes[mode];r_setup(chunky,d->w,d->h);
  for(x=0;x<MAP;x++)for(y=0;y<MAP;y++){
   topdown_project(&p,x*64+32,y*64+32,&sx,&sy);
   if(sx<0||sx>=320||sy<VIEW_TOP||sy>=VIEW_BOTTOM)continue;
   assert(scene_pick(&p,sx,sy,&tx,&ty)&&tx==x&&ty==y);
  }render_compare(mode);
 }
 {int cash=p.cash,selected=p.selected;in.key=KEY_VIEW;game_input(&p,&in);assert(!p.view_2d&&p.cash==cash&&p.selected==selected);game_input(&p,&in);assert(p.view_2d);}
}
static void topdown_build_check(void){
 Input in={0};int sx,sy,cash;
 park_init(&p,0);p.started=1;p.view_2d=1;r_setup(chunky,320,200);assert(park_choose(&p,BOUNCY));
 topdown_project(&p,(GATE_X-4)*64+32,(GATE_Y-6)*64+32,&sx,&sy);
 in.mx=sx;in.my=sy;in.left=1;game_input(&p,&in);assert(p.pending==0&&p.tool==ENTRANCE);cash=p.cash;
 in.left=0;in.key=KEY_VIEW;game_input(&p,&in);assert(!p.view_2d&&p.pending==0&&p.cash==cash);game_input(&p,&in);assert(p.view_2d);
 in.key=KEY_NONE;in.left=1;
 topdown_project(&p,(GATE_X-1)*64+32,(GATE_Y-6)*64+32,&in.mx,&in.my);game_input(&p,&in);assert(p.tool==EXIT);
 topdown_project(&p,(GATE_X-1)*64+32,(GATE_Y-5)*64+32,&in.mx,&in.my);game_input(&p,&in);assert(p.pending==-1&&p.items[0].complete);
 p.tool=PATH;game_input(&p,&in);
 topdown_project(&p,(GATE_X-1)*64+32,(GATE_Y-6)*64+32,&in.mx,&in.my);game_input(&p,&in);assert(p.items[0].connected);
}
static void culling_checks(void){
 static uint8_t reference[320*200];struct surf s={0};int i,x,y;
 static const int cameras[6][4]={{2080,3104,2048,2400},{500,700,0,900},{3500,500,1536,1000},{3800,3200,3000,2000},{2080,2080,2048,3500},{2000,1800,800,550}};
 park_demo(&p);p.selected=-1;
 for(y=3;y<MAP-1;y+=7)for(x=3;x<MAP-1;x+=7)p.trees[y][x]=1;
 for(y=1;y<MAP-1;y++)for(x=1;x<MAP-1;x++)if(x%13==0||y%11==0)p.paths[y][x]=p.reachable[y][x]=1;
 s.buf=chunky;s.w=320;s.h=200;scene_cache_bind(0,0);
 for(i=0;i<6;i++){
  p.camera_x=cameras[i][0];p.camera_y=cameras[i][1];p.camera_yaw=cameras[i][2];p.camera_distance=cameras[i][3];
  scene_culling=0;r_setup(chunky,320,200);scene_draw(&p,&s);memcpy(reference,chunky,sizeof(reference));
  scene_culling=1;r_setup(chunky,320,200);scene_draw(&p,&s);
  if(memcmp(reference,chunky,sizeof(reference))){fprintf(stderr,"culling mismatch camera %d\n",i);assert(0);}

 }
 park_init(&p,0);p.started=1;p.camera_x=p.camera_y=1500;p.camera_distance=550;
 p.items[0].used=1;p.items[0].type=HELTER;p.items[0].x=3;p.items[0].y=3;p.items[0].ex=p.items[0].xx=-1;
 for(i=0;i<2;i++){p.view_2d=i;r_setup(chunky,320,200);scene_draw(&p,&s);assert(scene_counts.items==0&&scene_counts.cells<MAP*MAP);}
}
int main(void){
 int mode,i,t,r,x,y,sx,sy,checks=0;Input in={0};struct pvert v;
 view_and_pan_checks();topdown_build_check();culling_checks();cache_invalidation();park_demo(&p);for(mode=0;mode<DETAIL_COUNT;mode++)render_compare(mode);
 /* Every ride model draws identically through the PC chunky and Amiga planar paths. */
 for(t=0;t<ITEM_COUNT;t++)for(r=0;r<4;r++){
  park_init(&p,0);p.started=1;park_choose(&p,t);p.rotation=r;assert(park_place(&p,10,10));
  p.tool=SELECT;p.pending=-1;p.items[0].complete=1;p.items[0].connected=1;p.ticks=173;
  p.camera_x=800;p.camera_y=800;p.camera_distance=1400;p.camera_yaw=2300+r*700;
  p.view_2d=0;for(mode=0;mode<DETAIL_COUNT;mode++)render_compare(mode);
  p.view_2d=1;for(mode=0;mode<DETAIL_COUNT;mode++)render_compare(mode);
 }
 /* Mouse picking round-trips projected tile centres, across camera directions/zoom. */
 park_init(&p,0);for(mode=0;mode<DETAIL_COUNT;mode++)for(i=0;i<8;i++){
  const FairDetail *d=&fair_modes[mode];
  p.camera_yaw=i*512;p.camera_distance=1200+i*160;r_setup(chunky,d->w,d->h);scene_camera(&p);
  for(y=6;y<27;y+=4)for(x=6;x<27;x+=4){
   r_pvert(x*64+32,y*64+32,0,&v);sx=(v.sx>>4)*SCREEN_W/d->w;sy=(v.sy>>4)*SCREEN_H/d->h;
   if(v.code||sx<0||sx>=SCREEN_W||sy<VIEW_TOP||sy>=VIEW_BOTTOM)continue;
   {int tx,ty;assert(scene_pick(&p,sx,sy,&tx,&ty));assert(tx==x&&ty==y);checks++;}
  }
 }assert(checks>50);
 /* Start menu, bottom tabs/cards, inspector changes, rotate and cancellation. */
 fair_detail=fair_requested_detail=DETAIL_NORMAL;park_init(&p,0);in.left=1;in.mx=140;in.my=105;game_input(&p,&in);assert(p.started&&p.cash==250000);
 in.mx=50;in.my=165;game_input(&p,&in);assert(p.tool==BUILD&&p.buy_type==BOUNCY);
 in.left=0;in.key=KEY_ROTATE;game_input(&p,&in);assert(p.rotation==1);
 in.key=KEY_ESC;game_input(&p,&in);assert(p.tool==SELECT&&p.pending<0);
 in.key=KEY_NONE;in.left=1;in.mx=190;in.my=190;game_input(&p,&in);assert(p.category==LANDSCAPE);
 in.mx=45;in.my=163;game_input(&p,&in);assert(p.tool==PATH);
 in.mx=245;in.my=190;game_input(&p,&in);assert(p.category==PARK_MENU);
 in.mx=99;in.my=160;game_input(&p,&in);assert(p.ticket==11);
 in.mx=290;in.my=190;game_input(&p,&in);assert(p.show_help);
 in.left=0;in.key=KEY_DETAIL;game_input(&p,&in);assert(fair_requested_detail==DETAIL_HIRES);game_input(&p,&in);assert(fair_requested_detail==DETAIL_LACE);
 p.show_help=0;in.key=KEY_NONE;in.left=1;in.mx=260;in.my=176;game_input(&p,&in);assert(fair_requested_detail==DETAIL_NORMAL);
 puts("scene/UI: 504 model/rotation/view/detail planar comparisons, scaled picking and UI flow passed");return 0;
}
