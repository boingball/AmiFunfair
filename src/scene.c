#include "game.h"
#include "shape.h"
#include "sort.h"
#include "fixmath.h"
#include <string.h>

const uint32_t fair_palette[32]={
 0x84c5dc,0x172538,0x274257,0x47617a,0xf4f0d5,0xfad466,0xdda142,0x99632b,
 0x4e342c,0x326641,0x4d8b49,0x69a457,0x91bb64,0x9d384a,0xdd5762,0xff9b83,
 0x305484,0x4485bc,0x73c3d6,0x533f75,0x8965a6,0xc38dcb,0x867f79,0xb3ada0,
 0xd7d0b6,0xe6ab74,0xb67e55,0x2c2937,0x386870,0x62a99f,0xb1d9b7,0xffffff
};
enum {R_GRASS,R_RED,R_BLUE,R_YELLOW,R_PURPLE,R_TEAL,R_WOOD,R_GREY,R_WHITE,R_DARK};
const struct ramp ramps[]={
 {4,{9,10,11,12}}, {3,{13,14,15}}, {3,{16,17,18}}, {3,{7,6,5}},
 {3,{19,20,21}}, {3,{28,29,30}}, {3,{8,7,26}}, {3,{22,23,24}},
 {3,{23,24,4}}, {3,{27,1,2}}
};
static const int colours[6]={R_RED,R_BLUE,R_YELLOW,R_PURPLE,R_TEAL,R_WHITE};
SceneCounts scene_counts;
int scene_culling=1;
static Park *world;
static const struct surf *surface;
static int ox,oy,rot,spanx,spany;
static int tick;
static void point(int x,int y,int z,int32_t *out){
 int tx=x,ty=y;
 if(rot==1){tx=spany-y;ty=x;}else if(rot==2){tx=spanx-x;ty=spany-y;}
 else if(rot==3){tx=y;ty=spanx-x;}
 out[0]=ox+tx;out[1]=oy+ty;out[2]=z;
}
static void box(int x,int y,int z,int w,int d,int h,int ramp){
 int32_t a[3],b[3];point(x,y,z,a);point(x+w,y+d,z+h,b);
 sh_box(imin(a[0],b[0]),imin(a[1],b[1]),z,imax(a[0],b[0]),imax(a[1],b[1]),z+h,ramp,ramp);
}
static void cyl(int x,int y,int r,int z,int h,int ramp){
 int32_t a[3];point(x,y,z,a);sh_cyl(a[0],a[1],r,z,z+h,8,ramp,shade_ramp(ramp,0,0,16384));
}
static void cone(int x,int y,int r,int z,int h,int ramp){
 int32_t a[3];point(x,y,z,a);sh_cone(a[0],a[1],r,0,z,z+h,8,ramp,-1);
}
static void beam(int x0,int y0,int z0,int x1,int y1,int z1,int width,int col){
 int32_t p[12],a[3],b[3];int k;
 point(x0,y0,z0,a);point(x1,y1,z1,b);
 /* Sub-pixel rails/supports do not need three world-space ribbon faces
  * (12 vertex transforms). Project their endpoints once and fill one ribbon.
  * Nearby thick beams and clipped beams retain the original geometry. */
 {struct pvert u,v;int depth,rx,ry;
  r_pvert(a[0],a[1],a[2],&u);r_pvert(b[0],b[1],b[2],&v);
  depth=imin(u.c.z,v.c.z);
  if(!u.code&&!v.code&&depth>NEAR_Z){
   rx=width*V.fx*16/depth;ry=width*V.fy*16/depth;
   if(rx<=16&&ry<=16){
    int xs[4],ys[4],dx=v.sx-u.sx,dy=v.sy-u.sy,wx=0,wy=0;
    if(iabs(dx)>=iabs(dy))wy=imax(4,ry);else wx=imax(4,rx);
    xs[0]=u.sx-wx;ys[0]=u.sy-wy;xs[1]=u.sx+wx;ys[1]=u.sy+wy;
    xs[2]=v.sx+wx;ys[2]=v.sy+wy;xs[3]=v.sx-wx;ys[3]=v.sy-wy;
    if((wx&&dy>0)||(!wx&&dx<0)){
     int t;for(k=0;k<2;k++){t=xs[k];xs[k]=xs[3-k];xs[3-k]=t;t=ys[k];ys[k]=ys[3-k];ys[3-k]=t;}
    }
    r_fill_poly2d(xs,ys,4,col);return;
   }
  }
 }
 for(k=0;k<3;k++){
  int axis=k==2?0:k,other=k==0?1:2,j;
  for(j=0;j<4;j++){int32_t *o=p+j*3;const int32_t *src=j<2?a:b;
   o[0]=src[0];o[1]=src[1];o[2]=src[2];
   o[axis]+=j==0||j==3?-width:width;
   o[other]+=k==2?width:0;
  }sh_flat(p,4,col);
 }
}
static void oval(int i,int n,int w,int h,int *x,int *y){
 *x=w/2+(isin(i*4096/n)*(w/2-25)>>14);
 *y=h/2+(icos(i*4096/n)*(h/2-25)>>14);
}
static void track(const Attraction *a){
 int pts[48][3],order[48],depth[48],i,j,n=32,w=spanx,h=spany,rail=a->type==RAILWAY;
 int base=rail?8:35,height=a->type==MOUSE?60:140;
 for(i=0;i<n;i++){
  int x,y,z;int32_t p[3];oval(i,n,w,h,&x,&y);
  z=base+(rail?0:((isin(i*8192/n)+16384)*height>>15));
  if(a->type==CORKSCREW&&i>=6&&i<=14){
   int an=(i-6)*4096/8;x=w/2+(i-10)*18;y=h/2+(icos(an)*45>>14);z=100+(isin(an)*80>>14);
  }
  if(a->type==MOUSE){x=(i<16?25:w-25)+(isin(i*12288/n)*18>>14);y=25+(i<16?i:31-i)*(h-50)/15;}
  pts[i][0]=x;pts[i][1]=y;pts[i][2]=z;point(x,y,z,p);depth[i]=sh_depth(p[0],p[1],p[2]);order[i]=i;
 }
 for(i=1;i<n;i++){int t=order[i];j=i;while(j&&depth[order[j-1]]<depth[t]){order[j]=order[j-1];j--;}order[j]=t;}
 for(j=0;j<n;j++){int k;int *v,*u;i=order[j];k=(i+1)%n;v=pts[i];u=pts[k];
  if(!rail&&!(i&1)){beam(v[0],v[1],0,v[0],v[1],v[2],4,a->type==WOODEN?7:23);
   beam(v[0]-12,v[1],0,u[0],u[1],u[2],2,a->type==WOODEN?8:22);}
  beam(v[0]-7,v[1],v[2],u[0]-7,u[1],u[2],2,rail?27:5);
  beam(v[0]+7,v[1],v[2],u[0]+7,u[1],u[2],2,rail?27:5);
  beam(v[0]-11,v[1],v[2]-2,v[0]+11,v[1],v[2]-2,2,7);
  if(i==((tick*a->speed/3)%n)){box(v[0]-12,v[1]-9,v[2]+3,24,18,14,colours[a->colour]);
   if(rail){box(v[0]-8,v[1]-7,v[2]+15,16,14,18,R_RED);cyl(v[0]+7,v[1],4,v[2]+15,20,R_DARK);}}
 }
 box(15,spany/2-20,0,60,40,18,R_WOOD);box(15,spany/2-20,50,60,40,8,R_RED);
}
static void roundride(const Attraction *a){
 int i,j,order[8],dep[8],n=a->type==TWISTER?6:8,r=imin(spanx,spany)/2-25;
 int cx=spanx/2,cy=spany/2,an=tick*a->speed*12;
 cyl(cx,cy,r+12,1,12,R_GREY);cyl(cx,cy,12,12,75,colours[a->colour]);
 if(a->type==CAROUSEL){cyl(cx,cy,5,12,115,R_YELLOW);}
 for(i=0;i<n;i++){int32_t p[3];int ang=i*4096/n+an;
  point(cx+(isin(ang)*r>>14),cy+(icos(ang)*r>>14),20,p);dep[i]=sh_depth(p[0],p[1],p[2]);order[i]=i;}
 for(i=1;i<n;i++){int t=order[i];j=i;while(j&&dep[order[j-1]]<dep[t]){order[j]=order[j-1];j--;}order[j]=t;}
 for(j=0;j<n;j++){int ang=order[j]*4096/n+an,x=cx+(isin(ang)*r>>14),y=cy+(icos(ang)*r>>14),z=22;
  int c=colours[(order[j]+a->colour)%6];
  if(a->type==JETS){z=45+(isin(ang+tick*13)*25>>14);beam(cx,cy,65,x,y,z,3,23);
   box(x-13,y-6,z,26,12,10,c);box(x-4,y-22,z+2,8,44,3,R_WHITE);box(x+7,y-7,z+7,6,14,10,c);
  }else if(a->type==CAROUSEL){
   z+=isin(ang+tick*15)*10>>14;beam(x,y,15,x,y,112,2,5);
   box(x-12,y-5,z+18,24,10,10,c);box(x+7,y-5,z+28,7,10,12,c);
   box(x-9,y-4,z+4,4,3,16,R_WOOD);box(x+7,y+2,z+4,4,3,16,R_WOOD);
  }else if(a->type==TEACUPS){cyl(x,y,18,16,18,c);beam(x+18,y,22,x+24,y,28,3,4);}
  else {beam(cx,cy,28,x,y,z,4,23);cyl(x,y,25,z,10,c);
   box(x-17,y-12,z+10,34,24,14,c);box(x-10,y-6,z+12,20,12,5,R_DARK);}
 }
 if(a->type==CAROUSEL){cone(cx,cy,r+18,112,45,colours[a->colour]);cyl(cx,cy,3,150,20,R_YELLOW);}
 if(a->type==WALTZER){cone(cx,cy,r+18,95,40,colours[a->colour]);}
}
static void draw_ride(const Attraction *a){
 int c=colours[a->colour],cx=spanx/2,cy=spany/2,i;
 box(3,3,0,spanx-6,spany-6,4,R_GREY);
 switch(a->type){
 case WOODEN:case CORKSCREW:case MOUSE:case RAILWAY:track(a);break;
 case TEACUPS:case JETS:case CAROUSEL:case TWISTER:case WALTZER:roundride(a);break;
 case BOUNCY:
  box(12,12,5,spanx-24,spany-24,22,R_YELLOW);
  box(12,12,27,spanx-24,12,40,c);box(12,12,27,12,spany-24,40,c);box(spanx-24,12,27,12,spany-24,40,c);
  for(i=0;i<4;i++){int x=i&1?spanx-20:20,y=i&2?spany-20:20;cyl(x,y,14,10,80,c);cone(x,y,18,90,28,R_BLUE);}
  for(i=0;i<3;i++){int z=40+(isin(tick*70+i*1200)*12>>14);box(50+i*28,cy,z,10,9,18,R_RED);}break;
 case GHOST:
  box(15,20,4,spanx-30,spany-40,90,R_DARK);box(10,15,94,spanx-20,spany-30,12,R_PURPLE);
  for(i=0;i<3;i++){box(30+i*70,spany-22,15,35,4,45,R_DARK);cyl(45+i*70,spany-14,11,60,18,R_WHITE);}
  box(20+(tick*a->speed*2)%(spanx-70),spany-16,6,35,22,18,R_RED);break;
 case PIRATE:{int swing=isin(tick*a->speed*22)*650>>14,z=105-(icos(swing)*80>>14),x=cx+(isin(swing)*80>>14);
  beam(25,cy-35,4,cx,cy,140,5,7);beam(spanx-25,cy-35,4,cx,cy,140,5,7);
  beam(25,cy+35,4,cx,cy,140,5,7);beam(spanx-25,cy+35,4,cx,cy,140,5,7);
  beam(cx,cy,140,x,cy,z+25,3,23);box(x-65,cy-20,z,130,40,24,c);
  box(x-75,cy-20,z+16,18,40,22,R_WOOD);box(x+57,cy-20,z+16,18,40,22,R_WOOD);break;}
 case DODGEMS:
  box(12,12,5,spanx-24,spany-24,6,R_DARK);
  for(i=0;i<4;i++){int x=i&1?spanx-12:12,y=i&2?spany-12:12;box(x-3,y-3,5,6,6,80,R_YELLOW);}
  for(i=0;i<6;i++){int x=cx+(isin(tick*9+i*700)*(spanx/2-40)>>14),y=cy+(icos(tick*13+i*700)*(spany/2-40)>>14);
   box(x-13,y-10,10,26,20,13,colours[i]);beam(x,y,23,x,y,83,1,23);}
  box(8,8,86,spanx-16,12,10,c);box(8,spany-20,86,spanx-16,12,10,c);break;
 case HELTER:
  cyl(cx,cy,40,4,165,R_YELLOW);cone(cx,cy,50,169,35,R_RED);
  for(i=31;i>=0;i--){int an=i*4096/12,an2=(i+1)*4096/12;
   beam(cx+(isin(an)*48>>14),cy+(icos(an)*48>>14),165-i*5,cx+(isin(an2)*48>>14),cy+(icos(an2)*48>>14),160-i*5,6,4);}
  break;
 case SKI:
  for(i=0;i<16;i++){int x,y,x2,y2;oval(i,16,spanx,spany,&x,&y);oval(i+1,16,spanx,spany,&x2,&y2);
   beam(x,y,30+(isin(i*4096/16)*22>>14),x2,y2,30+(isin((i+1)*4096/16)*22>>14),6,5);}
  for(i=0;i<6;i++){int x,y,an=i*4096/6+tick*a->speed*18;x=cx+(isin(an)*(cx-25)>>14);y=cy+(icos(an)*(cy-25)>>14);
   box(x-16,y-10,38+(isin(an)*22>>14),32,20,18,colours[i]);}break;
 case ARCADE:
  box(10,10,4,spanx-20,spany-20,65,R_PURPLE);
  for(i=0;i<5;i++){box(22+i*40,spany-8,4,25,18,42,i&1?R_BLUE:R_RED);
   box(25+i*40,spany+10,22,19,2,16,R_DARK);box(29+i*40,spany+12,25,11,2,10,R_YELLOW);}
  box(6,5,72,spanx-12,spany,10,c);break;
 default:break;
 }
}
static void draw_stand(const Attraction *a){
 int cx=spanx/2,cy=spany/2,i,c=colours[a->colour];
 box(8,8,2,spanx-16,spany-16,55,c);box(14,spany-10,18,spanx-28,4,22,R_DARK);
 box(10,spany-4,17,spanx-20,12,5,R_WHITE);
 for(i=0;i<6;i++)box(3+i*(spanx-6)/6,3,57,(spanx-6)/6,spany-6,8,i&1?R_WHITE:c);
 if(a->type==ICECREAM){cone(cx,cy,14,65,26,R_YELLOW);cyl(cx,cy,16,88,20,R_WHITE);}
 if(a->type==CANDY){cyl(cx,cy,3,65,25,R_WOOD);cyl(cx,cy,19,90,25,R_RED);}
 if(a->type==COFFEE){cyl(cx,cy,17,65,26,R_WHITE);cyl(cx,cy,14,91,2,R_WOOD);}
 if(a->type==CHIPS){box(cx-22,cy-12,65,44,24,12,R_WHITE);
  for(i=0;i<5;i++)box(cx-18+i*8,cy-10,77,5,20,10,R_YELLOW);}
 if(a->type==TOILETS){box(15,spany-8,3,25,3,42,R_BLUE);box(spanx-40,spany-8,3,25,3,42,R_RED);}
}
static void tile(int x,int y,int z,int col){
 int32_t p[12]={x*64,y*64,z,(x+1)*64,y*64,z,(x+1)*64,(y+1)*64,z,x*64,(y+1)*64,z};sh_flat(p,4,col);
}
static void gate(int x,int y,int colour){
 int xx=x*64+32,yy=y*64+32;sh_box(xx-22,yy-3,2,xx-18,yy+3,42,R_WHITE,R_WHITE);
 sh_box(xx+18,yy-3,2,xx+22,yy+3,42,R_WHITE,R_WHITE);
 {int32_t p[12]={xx-22,yy,35,xx+22,yy,35,xx+22,yy,48,xx-22,yy,48};sh_flat(p,4,colour);}
}
static void item_draw(void *arg){
 scene_counts.items++;
 int id=(int)(intptr_t)arg;Attraction *a=&world->items[id];ox=a->x*64;oy=a->y*64;rot=a->rotation;
 spanx=catalogue[a->type].w*64;spany=catalogue[a->type].h*64;
 tick=(a->complete&&a->open&&a->connected&&a->condition>10)?(int)(world->ticks&65535):0;
 if(catalogue[a->type].group==RIDES)draw_ride(a);else draw_stand(a);
}
static void guest_draw(void *arg){
 int id=(int)(intptr_t)arg;Guest *g=&world->guests[id];int progress=imin(g->progress,8),sx,sy,hx,hy;
 int x=(g->x*progress+g->nx*(8-progress))*8+32,y=(g->y*progress+g->ny*(8-progress))*8+32;
 struct cvert feet,head;r_xform(x,y,1,&feet);r_xform(x,y,27,&head);
 if(!r_project(&feet,&sx,&sy)||!r_project(&head,&hx,&hy))return;
 {int height=imax(2,(sy-hy+15)/16),width=imax(1,height*V.fx/V.fy/3),xx=sx>>4,yy=hy>>4;
  static const int shirts[6]={14,17,5,20,29,4};
  if(xx+width<V.wx0||xx-width>=V.wx1||yy+height<V.wy0||yy>=V.wy1)return;
  scene_counts.guests++;
  r_rect(xx-width/2,yy+height/3,width,imax(1,height*2/3),shirts[g->colour]);
  r_disc(hx,hy+(height/6)*16,imax(8,height*16*V.fx/V.fy/5),25);
 }
}
static void tree_draw(void *arg){scene_counts.trees++;int id=(int)(intptr_t)arg;int x=(id%MAP)*64+32,y=(id/MAP)*64+32;
 sh_box(x-3,y-3,0,x+3,y+3,30,R_WOOD,R_WOOD);sh_cone(x,y,23,0,20,85,6,R_GRASS,-1);}
static void gate_draw(void *arg){int id=(int)(intptr_t)arg;Attraction *a=&world->items[id/2];
 if(id&1)gate(a->xx,a->xy,14);else gate(a->ex,a->ey,18);}
static void entrance_draw(void *unused){int x=GATE_X*64+32,y=GATE_Y*64+32;(void)unused;
 sh_box(x-75,y-20,0,x-45,y+20,110,R_RED,R_YELLOW);sh_box(x+45,y-20,0,x+75,y+20,110,R_BLUE,R_YELLOW);
 sh_box(x-80,y-23,100,x+80,y+23,133,R_RED,R_YELLOW);
 {struct pvert p;int sx,sy;r_pvert(x,y,115,&p);
  if(!(p.code&1)){sx=(p.sx>>4)*SCREEN_W/V.w-24;sy=(p.sy>>4)*SCREEN_H/V.h;
   if(sx>=0&&sx+48<=SCREEN_W&&sy>=VIEW_TOP&&sy+7<=VIEW_BOTTOM)ui_text(surface,sx,sy,"FUN FAIR",5);
  }
 }
}
void scene_camera(const Park *p){
 int horizontal=p->camera_distance*3/4;
 r_window(0,VIEW_TOP*V.h/SCREEN_H,V.w,VIEW_BOTTOM*V.h/SCREEN_H);
 r_zoom(200);
 r_camera(p->camera_x-(isin(p->camera_yaw)*horizontal>>14),
          p->camera_y-(icos(p->camera_yaw)*horizontal>>14),p->camera_distance,p->camera_yaw,600);
}
static int ray_plane(int mx,int my,int z,int *wx,int *wy){
 int rx,ry,rz=16384,dx,dy,dz,t;
 mx=mx*V.w/SCREEN_W;my=my*V.h/SCREEN_H;
 rx=(mx-V.cx)*16384/V.fx;ry=(V.cy-my)*16384/V.fy;
 dx=rx;dy=(ry*V.spit+rz*V.cpit)>>14;dz=(ry*V.cpit-rz*V.spit)>>14;
 if(dz>=-100)return 0;t=((z-V.camz)*1024)/dz;if(t<=0)return 0;
 rx=(dx*V.cyaw+dy*V.syaw)>>14;ry=(-dx*V.syaw+dy*V.cyaw)>>14;
 *wx=V.camx+rx*t/1024;*wy=V.camy+ry*t/1024;return 1;
}
void scene_visible_map(const Park *p,MapWindow *b){
 int i,z,x,y,minx=MAP*64,miny=MAP*64,maxx=0,maxy=0;
 if(p->view_2d){topdown_visible(p,b);return;}
 scene_camera(p);
 if(!scene_culling){b->x0=b->y0=0;b->x1=b->y1=MAP;return;}
 for(z=0;z<=240;z+=240)for(i=0;i<4;i++){
  if(!ray_plane(i&1?SCREEN_W:0,i&2?VIEW_BOTTOM:VIEW_TOP,z,&x,&y)){
   b->x0=b->y0=0;b->x1=b->y1=MAP;return;
  }minx=imin(minx,x);miny=imin(miny,y);maxx=imax(maxx,x);maxy=imax(maxy,y);
 }
 b->x0=iclamp(minx/64-2,0,MAP);b->y0=iclamp(miny/64-2,0,MAP);
 b->x1=iclamp(maxx/64+3,0,MAP);b->y1=iclamp(maxy/64+3,0,MAP);
}
static int visible_box(int x,int y,int w,int h,int z0,int z1){
 int i,minx=32767,miny=32767,maxx=-32767,maxy=-32767;
 if(!scene_culling)return 1;
 if(r_box_hidden(x,y,w,h,z0,z1))return 0;
 for(i=0;i<8;i++){
  struct pvert p;r_pvert(x+(i&1?w:0),y+(i&2?h:0),i&4?z1:z0,&p);
  if(p.code&1)return 1;
  minx=imin(minx,p.sx);maxx=imax(maxx,p.sx);miny=imin(miny,p.sy);maxy=imax(maxy,p.sy);
 }
 return maxx>=V.wx0*16&&minx<V.wx1*16&&maxy>=V.wy0*16&&miny<V.wy1*16;
}
static int in_window(const MapWindow *b,int x,int y,int w,int h){
 return x<b->x1&&x+w>b->x0&&y<b->y1&&y+h>b->y0;
}
static uint8_t *ground_cache;
static uint32_t ground_capacity;
static uint8_t saved_paths[MAP][MAP],saved_reachable[MAP][MAP];
static int cache_valid,cache_x,cache_y,cache_yaw,cache_distance,cache_w,cache_h,cache_planar,cache_bpr;
void scene_cache_bind(void *memory,uint32_t bytes){ground_cache=memory;ground_capacity=bytes;cache_valid=0;}
static void ground_rect(int x0,int y0,int x1,int y1,int z,int col){
 struct pvert v[4];const struct pvert *vp[4];int i,near=0,minx=32767,miny=32767,maxx=-32767,maxy=-32767;
 for(i=0;i<4;i++){
  r_pvert((i==1||i==2?x1:x0)*64,(i>=2?y1:y0)*64,z,&v[i]);vp[i]=&v[i];near|=v[i].code&1;
  if(!(v[i].code&1)){minx=imin(minx,v[i].sx);maxx=imax(maxx,v[i].sx);miny=imin(miny,v[i].sy);maxy=imax(maxy,v[i].sy);}
 }
 if(scene_culling&&!near&&(maxx<V.wx0*16||minx>=V.wx1*16||maxy<V.wy0*16||miny>=V.wy1*16))return;
 r_ppoly(vp,4,col,0);
}
static void draw_ground(Park *p,const struct surf *s,const MapWindow *b){
 uint32_t size=s->planes?(uint32_t)s->psize*5:(uint32_t)s->w*s->h;
 uint8_t *dest=s->planes?s->planes:s->buf;int x,y;
 int can_cache=ground_cache&&ground_capacity>=size;
 if(can_cache&&cache_valid&&cache_w==s->w&&cache_h==s->h&&cache_bpr==s->bpr&&cache_planar==!!s->planes&&
    cache_x==p->camera_x&&cache_y==p->camera_y&&cache_yaw==p->camera_yaw&&cache_distance==p->camera_distance&&
    !memcmp(saved_paths,p->paths,sizeof(saved_paths))&&!memcmp(saved_reachable,p->reachable,sizeof(saved_reachable))){
  memcpy(dest,ground_cache,size);return;
 }
 r_clear(0);
 ground_rect(0,0,MAP,MAP,0,11);
 ground_rect(0,0,MAP,1,1,9);ground_rect(0,MAP-1,MAP,MAP,1,9);
 ground_rect(0,1,1,MAP-1,1,9);ground_rect(MAP-1,1,MAP,MAP-1,1,9);
 for(y=imax(1,b->y0);y<imin(MAP-1,b->y1);y++)for(x=imax(1,b->x0);x<imin(MAP-1,b->x1);){
  if(p->paths[y][x]){int start=x,col=p->reachable[y][x]?24:22;
   /* Keep original run endpoints: shortening a perspective edge can change
    * its sub-pixel rounding even though the cut lies beyond the viewport. */
   while(start>1&&p->paths[y][start-1]&&(p->reachable[y][start-1]?24:22)==col)start--;
   do{x++;}while(x<MAP-1&&p->paths[y][x]&&(p->reachable[y][x]?24:22)==col);
   ground_rect(start,y,x,y+1,1,col);
  }else x++;
 }
 if(can_cache){
  memcpy(ground_cache,dest,size);memcpy(saved_paths,p->paths,sizeof(saved_paths));memcpy(saved_reachable,p->reachable,sizeof(saved_reachable));
  cache_x=p->camera_x;cache_y=p->camera_y;cache_yaw=p->camera_yaw;cache_distance=p->camera_distance;
  cache_w=s->w;cache_h=s->h;cache_bpr=s->bpr;cache_planar=!!s->planes;cache_valid=1;
 }else cache_valid=0;
}
void scene_draw(Park *p,const struct surf *s){
 int x,y,i;MapWindow b;world=p;surface=s;memset(&scene_counts,0,sizeof(scene_counts));
 if(p->view_2d){topdown_draw(p,s);return;}
 scene_visible_map(p,&b);draw_ground(p,s,&b);
 scene_counts.cells=(b.x1-b.x0)*(b.y1-b.y0);
 /* Ground overlays are drawn before the structures. */
 if(p->selected>=0&&p->items[p->selected].used){Attraction *a=&p->items[p->selected];int w,h;
  park_dimensions(a->type,a->rotation,&w,&h);
  if(in_window(&b,a->x,a->y,w,h)&&visible_box(a->x*64,a->y*64,w*64,h*64,3,4))
   for(y=a->y;y<a->y+h;y++)for(x=a->x;x<a->x+w;x++)tile(x,y,3,5);
 }
 if(p->hover_x>=0&&p->hover_y>=0&&p->tool!=SELECT){int w=1,h=1,col=18;
  if(p->tool==BUILD){park_dimensions(p->buy_type,p->rotation,&w,&h);col=park_can_place(p,p->buy_type,p->hover_x,p->hover_y,p->rotation)?18:14;}
  else if(p->tool==ENTRANCE||p->tool==EXIT)col=park_gate_valid(p,p->pending,p->hover_x,p->hover_y)?18:14;
  for(y=p->hover_y;y<p->hover_y+h&&y<MAP;y++)for(x=p->hover_x;x<p->hover_x+w&&x<MAP;x++)tile(x,y,4,R_DITHER(col,4));
 }
 r_sort_begin(10000);
 for(i=0;i<MAX_ITEMS;i++)if(p->items[i].used){Attraction *a=&p->items[i];int w,h;
  park_dimensions(a->type,a->rotation,&w,&h);
  if(in_window(&b,a->x,a->y,w,h)&&visible_box(a->x*64-32,a->y*64-32,w*64+64,h*64+64,0,240))
   r_sort_add(sh_depth(a->x*64+w*32,a->y*64+h*32,0),item_draw,(void *)(intptr_t)i);
  if(a->ex>=0&&in_window(&b,a->ex,a->ey,1,1)&&visible_box(a->ex*64,a->ey*64,64,64,0,55))r_sort_add(sh_depth(a->ex*64+32,a->ey*64+32,20),gate_draw,(void *)(intptr_t)(i*2));
  if(a->xx>=0&&in_window(&b,a->xx,a->xy,1,1)&&visible_box(a->xx*64,a->xy*64,64,64,0,55))r_sort_add(sh_depth(a->xx*64+32,a->xy*64+32,20),gate_draw,(void *)(intptr_t)(i*2+1));
 }
 for(y=imax(1,b.y0);y<imin(MAP-1,b.y1);y++)for(x=imax(1,b.x0);x<imin(MAP-1,b.x1);x++)if(p->trees[y][x]&&visible_box(x*64,y*64,64,64,0,90))r_sort_add(sh_depth(x*64+32,y*64+32,30),tree_draw,(void *)(intptr_t)(y*MAP+x));
 for(i=0;i<MAX_GUESTS;i++)if(p->guests[i].used&&!p->guests[i].riding){Guest *g=&p->guests[i];if(in_window(&b,g->x,g->y,2,2))r_sort_add(sh_depth(g->x*64+32,g->y*64+32,12),guest_draw,(void *)(intptr_t)i);}
 if(visible_box(GATE_X*64-64,GATE_Y*64,192,64,0,140))r_sort_add(sh_depth(GATE_X*64+32,GATE_Y*64+32,60),entrance_draw,0);r_sort_draw();
 r_window_off();
}
int scene_pick(const Park *p,int mx,int my,int *x,int *y){
 int wx,wy;
 if(p->view_2d)return topdown_pick(p,mx,my,x,y);
 if(mx<0||mx>=SCREEN_W||my<VIEW_TOP||my>=VIEW_BOTTOM)return 0;
 scene_camera(p);if(!ray_plane(mx,my,0,&wx,&wy))return 0;
 *x=wx/64;*y=wy/64;return wx>=0&&wy>=0&&*x<MAP&&*y<MAP;
}
