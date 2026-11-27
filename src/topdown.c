#include "game.h"
#include "fixmath.h"
#include "render.h"

/* Flat plan-view artwork. No polygon transforms, depth sort or 3D models. */
static const uint8_t icons[ITEM_COUNT][8]={
 {0xa5,0xff,0x81,0x99,0xbd,0xa5,0x81,0xff}, /* castle */
 {0x3c,0x7e,0xdb,0xff,0xe7,0xbd,0x99,0x81}, /* ghost */
 {0x7e,0xc3,0x81,0x99,0x81,0x81,0xc3,0x7e}, /* wooden */
 {0x7e,0xc3,0x99,0xa5,0xa5,0x99,0xc3,0x7e}, /* corkscrew */
 {0xff,0x81,0x9f,0x90,0xf9,0x09,0x81,0xff}, /* mouse */
 {0x18,0x3c,0x7e,0x7e,0x3c,0x3c,0x18,0x18}, /* ice cream */
 {0x3c,0x7e,0xff,0xff,0x7e,0x3c,0x18,0x18}, /* floss */
 {0x66,0x66,0x00,0xff,0x66,0x66,0x66,0x66}, /* toilets */
 {0x00,0x7e,0x43,0x43,0x7f,0x3c,0x00,0x7e}, /* coffee */
 {0x54,0x54,0x54,0x7e,0x7e,0x3c,0x3c,0x18}, /* chips */
 {0x3c,0x42,0x99,0xa5,0xa5,0x99,0x42,0x3c}, /* cups */
 {0x18,0x18,0x7e,0xff,0xff,0x7e,0x3c,0x00}, /* ship */
 {0x18,0x99,0x5a,0x3c,0xff,0x3c,0x5a,0x99}, /* jets */
 {0xff,0x81,0xb5,0xb5,0x81,0xad,0xad,0xff}, /* dodgems */
 {0x3c,0x7e,0xdb,0xff,0xff,0xdb,0x7e,0x3c}, /* carousel */
 {0x18,0x3c,0x7e,0x52,0x4a,0x52,0x4a,0x7e}, /* helter */
 {0x7e,0xc3,0x81,0x81,0x81,0x81,0xc3,0x7e}, /* railway */
 {0x3c,0x42,0x81,0xa5,0xa5,0x81,0x42,0x3c}, /* ski */
 {0x81,0x42,0x3c,0x7e,0x7e,0x3c,0x42,0x81}, /* twister */
 {0x3c,0x66,0xdb,0x99,0x99,0xdb,0x66,0x3c}, /* waltzer */
 {0xff,0x81,0xbd,0xa5,0xbd,0x81,0xa5,0xff}  /* amusements */
};
static int cell_size(const Park *p){return iclamp(12000/p->camera_distance,3,24);}
static int origin_x(const Park *p){return SCREEN_W/2+p->camera_x*cell_size(p)/64;}
static int origin_y(const Park *p){return VIEW_TOP+(VIEW_BOTTOM-VIEW_TOP)*2/5-p->camera_y*cell_size(p)/64;}
static int floor_div(int a,int b){return a<0?-((-a+b-1)/b):a/b;}
void topdown_project(const Park *p,int wx,int wy,int *sx,int *sy){
 int cell=cell_size(p);*sx=origin_x(p)-wx*cell/64;*sy=origin_y(p)+wy*cell/64;
}
int topdown_pick(const Park *p,int mx,int my,int *x,int *y){
 int cell=cell_size(p);
 if(mx<0||mx>=SCREEN_W||my<VIEW_TOP||my>=VIEW_BOTTOM)return 0;
 *x=floor_div(origin_x(p)-1-mx,cell);*y=floor_div(my-origin_y(p),cell);
 return *x>=0&&*x<MAP&&*y>=0&&*y<MAP;
}
void topdown_visible(const Park *p,MapWindow *b){
 int cell=cell_size(p);
 b->x0=iclamp(floor_div(origin_x(p)-SCREEN_W,cell)-1,0,MAP);
 b->x1=iclamp(floor_div(origin_x(p),cell)+2,0,MAP);
 b->y0=iclamp(floor_div(VIEW_TOP-origin_y(p),cell)-1,0,MAP);
 b->y1=iclamp(floor_div(VIEW_BOTTOM-origin_y(p),cell)+2,0,MAP);
}
static int visible(int x,int y,int w,int h){return x<SCREEN_W&&x+w>0&&y<VIEW_BOTTOM&&y+h>VIEW_TOP;}
static void rect(const struct surf *s,int x,int y,int w,int h,int colour){
 int x1=imin(x+w,SCREEN_W),y1=imin(y+h,VIEW_BOTTOM),sx=s->w/SCREEN_W,sy=s->h/SCREEN_H;
 x=imax(x,0);y=imax(y,VIEW_TOP);if(x1<=x||y1<=y)return;
 if(!s->pens&&s->buf==V.buf&&s->planes==V.planes)r_rect(x*sx,y*sy,(x1-x)*sx,(y1-y)*sy,colour);
 else surf_rect(s,x*sx,y*sy,(x1-x)*sx,(y1-y)*sy,colour);
}
static void frame(const struct surf *s,int x,int y,int w,int h,int colour){
 rect(s,x,y,w,1,colour);rect(s,x,y+h-1,w,1,colour);rect(s,x,y,1,h,colour);rect(s,x+w-1,y,1,h,colour);
}
static void tile_rect(const Park *p,const struct surf *s,int x,int y,int w,int h,int colour,int outline){
 int sx,sy,cell=cell_size(p);topdown_project(p,(x+w)*64,y*64,&sx,&sy);
 if(outline)frame(s,sx,sy,w*cell,h*cell,colour);else rect(s,sx,sy,w*cell,h*cell,colour);
}
static void icon(const struct surf *s,int x,int y,int type,int size){
 int row,b;
 for(row=0;row<8;row++)for(b=0;b<8;){
  if(icons[type][row]&(128>>b)){int start=b;do{b++;}while(b<8&&(icons[type][row]&(128>>b)));
   rect(s,x+start*size,y+row*size,(b-start)*size,size,4);
  }else b++;
 }
}
void topdown_draw(Park *p,const struct surf *s){
 MapWindow b;int x,y,i,cell=cell_size(p),sx,sy;
 static const int ride_colours[6]={13,16,7,19,28,22};
 r_window_off();r_clear(0);topdown_visible(p,&b);
 tile_rect(p,s,0,0,MAP,MAP,11,0);tile_rect(p,s,0,0,MAP,MAP,9,1);
 for(y=b.y0;y<b.y1;y++)for(x=b.x0;x<b.x1;){
  scene_counts.cells++;
  if(p->paths[y][x]){int start=x,col=p->reachable[y][x]?24:22;
   do{x++;}while(x<b.x1&&p->paths[y][x]&&(p->reachable[y][x]?24:22)==col);
   tile_rect(p,s,start,y,x-start,1,col,0);
  }else x++;
 }
 for(y=b.y0;y<b.y1;y++)for(x=b.x0;x<b.x1;x++)if(p->trees[y][x]){
  topdown_project(p,x*64+32,y*64+32,&sx,&sy);
  if(visible(sx-2,sy-2,5,5)){rect(s,sx-1,sy-2,3,5,9);rect(s,sx-2,sy-1,5,3,10);scene_counts.trees++;}
 }
 for(i=0;i<MAX_ITEMS;i++)if(p->items[i].used){
  Attraction *a=&p->items[i];int w,h,iw,ih,size;
  park_dimensions(a->type,a->rotation,&w,&h);topdown_project(p,(a->x+w)*64,a->y*64,&sx,&sy);iw=w*cell;ih=h*cell;
  if(visible(sx,sy,iw,ih)){
   int edge=i==p->selected?5:a->connected?18:14;
   rect(s,sx,sy,iw,ih,ride_colours[a->colour]);frame(s,sx,sy,iw,ih,edge);
   size=iclamp((imin(iw,ih)-4)/8,1,3);
   if(imin(iw,ih)>=10)icon(s,sx+(iw-8*size)/2,sy+(ih-8*size)/2,a->type,size);else rect(s,sx+iw/2-1,sy+ih/2-1,2,2,4);
   if(a->open&&a->connected&&a->condition>10&&catalogue[a->type].group==RIDES){
    static const int dx[8]={-1,0,1,1,1,0,-1,-1},dy[8]={-1,-1,-1,0,1,1,1,0};
    int phase=(int)(p->ticks*a->speed/4)&7;
    rect(s,sx+iw/2+dx[phase]*(iw/2-2)-1,sy+ih/2+dy[phase]*(ih/2-2)-1,2,2,5);
   }scene_counts.items++;
  }
  if(a->ex>=0)tile_rect(p,s,a->ex,a->ey,1,1,18,1);
  if(a->xx>=0)tile_rect(p,s,a->xx,a->xy,1,1,14,1);
 }
 if(p->hover_x>=0&&p->hover_y>=0&&p->tool!=SELECT){
  int w=1,h=1,col=18;
  if(p->tool==BUILD){park_dimensions(p->buy_type,p->rotation,&w,&h);col=park_can_place(p,p->buy_type,p->hover_x,p->hover_y,p->rotation)?18:14;}
  if(p->tool==ENTRANCE||p->tool==EXIT)col=park_gate_valid(p,p->pending,p->hover_x,p->hover_y)?18:14;
  tile_rect(p,s,p->hover_x,p->hover_y,w,h,col,1);
 }
 for(i=0;i<MAX_GUESTS;i++)if(p->guests[i].used&&!p->guests[i].riding){
  Guest *g=&p->guests[i];int t=imin(g->progress,8);
  topdown_project(p,(g->x*t+g->nx*(8-t))*8+32,(g->y*t+g->ny*(8-t))*8+32,&sx,&sy);
  if(visible(sx,sy,2,2)){rect(s,sx,sy,1,2,14+g->colour);scene_counts.guests++;}
 }
 topdown_project(p,GATE_X*64+32,GATE_Y*64+32,&sx,&sy);
 rect(s,sx-cell,sy-cell/2,cell*2,cell,13);frame(s,sx-cell,sy-cell/2,cell*2,cell,5);
 if(sx>=25&&sx<SCREEN_W-25&&sy>=VIEW_TOP+12&&sy<VIEW_BOTTOM)ui_text(s,sx-24,sy-11,"ENTRANCE",5);
}
