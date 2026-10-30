#include "game.h"
#include "fixmath.h"
#include "render.h"
extern const uint8_t font8[64][8];
/* UI/input use a stable 320x200 logical layout in every native mode. */
static void ui_rect(const struct surf *s,int x,int y,int w,int h,int col){
 int sx=s->w/SCREEN_W,sy=s->h/SCREEN_H;surf_rect(s,x*sx,y*sy,w*sx,h*sy,col);
}
static void ui_frame(const struct surf *s,int x,int y,int w,int h,int col){
 ui_rect(s,x,y,w,1,col);ui_rect(s,x,y+h-1,w,1,col);
 ui_rect(s,x,y,1,h,col);ui_rect(s,x+w-1,y,1,h,col);
}
static void ui_pixel(const struct surf *s,int x,int y,int col){ui_rect(s,x,y,1,1,col);}
static void cycle_detail(Park *p){
 int mode=fair_cycle_detail();
 if(fair_detail_mask==1)park_message(p,"Normal only: hires needs AGA and enough chip RAM.");
 else park_message(p,mode==DETAIL_NORMAL?"Detail: NORMAL 320x200.":mode==DETAIL_HIRES?"Detail: HIRES 640x200.":"Detail: HIRES LACED 640x400.");
}

static int hit(int x,int y,int bx,int by,int w,int h){return x>=bx&&x<bx+w&&y>=by&&y<by+h;}
void ui_text(const struct surf *s,int x,int y,const char *str,int colour){
 int c,row,b,sx=s->w/SCREEN_W,sy=s->h/SCREEN_H;
 while(*str){c=(unsigned char)*str++;if(c>='a'&&c<='z')c-=32;
  if(c>=32&&c<=95){
   int px=x*sx,py=y*sy;
   if(s->planes&&px>=0&&py>=0&&px+6*sx<=s->w&&py+7*sy<=s->h){
    uint16_t masks[28];int nw=((px&15)+6*sx+15)/16,k=0,yy;
    for(row=0;row<7;row++){
     uint32_t bits=0;
     for(b=0;b<6;b++)if(font8[c-32][row]&(128>>b))bits|=((1u<<sx)-1)<<(32-(b+1)*sx);
     bits>>=px&15;
     for(yy=0;yy<sy;yy++){masks[k++]=(uint16_t)(bits>>16);if(nw==2)masks[k++]=(uint16_t)bits;}
    }
    planar_words(s->planes+py*s->bpr+(px>>4)*2,s->bpr,s->psize,masks,7*sy,nw,s->pens?s->pens[colour]:colour);
   }else{
    for(row=0;row<7;row++)for(b=0;b<6;b++)if(font8[c-32][row]&(128>>b))ui_pixel(s,x+b,y+row,colour);
   }
  }x+=6;
 }
}
static void labelnum(const struct surf *s,int x,int y,const char *name,int v,int col){
 char b[60];char *e=str_cat(b,name);str_num(e,v);ui_text(s,x,y,b,col);
}
static void panel(const struct surf *s,int x,int y,int w,int h){
 ui_rect(s,x,y,w,h,1);ui_frame(s,x,y,w,h,3);ui_rect(s,x+1,y+1,w-2,1,18);
}
static void button(const struct surf *s,int x,int y,int w,int h,const char *name,int on,int disabled){
 ui_rect(s,x,y,w,h,on?28:2);ui_frame(s,x,y,w,h,on?18:3);
 ui_text(s,x+(w-str_len(name)*6)/2,y+(h-6)/2,name,disabled?22:on?5:4);
}
static void brief(const struct surf *s,int x,int y,const char *name,int max,int col){
 char b[40];int i=0;while(name[i]&&i<max){b[i]=name[i];i++;}b[i]=0;ui_text(s,x,y,b,col);
}
static int inspector(const Park *p){return p->tool==BUILD||p->selected>=0;}
static void inspect_draw(Park *p,const struct surf *s){
 const Attraction *a=p->selected>=0?&p->items[p->selected]:0;
 int t=p->tool==BUILD?p->buy_type:a?a->type:0;
 panel(s,208,17,109,112);brief(s,213,23,catalogue[t].name,16,5);
 if(p->tool==BUILD){
  labelnum(s,214,36,"COST ",catalogue[t].cost,4);
  labelnum(s,214,47,"ROTATION ",p->rotation*90,18);
  button(s,213,58,47,12,"< Z",0,0);button(s,265,58,47,12,"R >",0,0);
  labelnum(s,214,75,"COLOUR ",p->colour+1,4);button(s,295,73,17,12,"+",0,0);
  labelnum(s,214,90,"SPEED ",p->speed,4);button(s,295,88,17,12,"+",0,0);
  ui_text(s,214,107,"CLICK TO PLACE",18);ui_text(s,214,118,"RMB CANCELS",23);
 }else if(a){
  labelnum(s,214,35,"PRICE ",a->price,4);button(s,274,32,17,12,"-",0,0);button(s,295,32,17,12,"+",0,0);
  button(s,213,48,59,12,a->open?"OPEN":"CLOSED",a->open,0);button(s,277,48,35,12,"SELL",0,0);
  labelnum(s,214,66,"COLOUR ",a->colour+1,4);button(s,295,63,17,12,"+",0,0);
  labelnum(s,214,81,"SPEED ",a->speed,4);button(s,295,78,17,12,"+",0,0);
  labelnum(s,214,94,"REPAIR ",a->condition,4);
  labelnum(s,214,105,"VISITS ",a->visitors,4);
  ui_text(s,214,118,!a->complete?"PLACE GATES":!a->connected?"NO PATH LINK":a->condition<=10?"BROKEN DOWN":"PATHS CONNECTED",a->connected?18:14);
 }
}
void ui_draw(Park *p,const struct surf *s,const Input *in){
 int i;char b[100];
 ui_rect(s,0,0,320,14,1);labelnum(s,4,4,"CASH ",p->cash,p->cash<0?14:5);
 labelnum(s,125,4,"GUESTS ",park_active(p),4);labelnum(s,220,4,"DAY ",p->day,4);
 if(p->paused)ui_text(s,274,4,"PAUSED",14);
 if(inspector(p)&&p->started)inspect_draw(p,s);
 ui_rect(s,0,137,320,15,1);
 brief(s,4,138,p->message,52,4);if(str_len(p->message)>52)brief(s,4,145,p->message+52,52,23);
 panel(s,0,152,320,34);
 if(p->category==RIDES||p->category==STANDS){
  int list[ITEM_COUNT],n=0;
  for(i=0;i<ITEM_COUNT;i++)if(catalogue[i].group==p->category)list[n++]=i;
  if(p->page*3>=n)p->page=0;
  button(s,1,155,12,27,"<",0,0);button(s,307,155,12,27,">",0,0);
  for(i=0;i<3&&p->page*3+i<n;i++){int t=list[p->page*3+i],x=16+i*97,disabled=p->cash<catalogue[t].cost;
   ui_rect(s,x,155,94,27,p->buy_type==t?28:2);ui_frame(s,x,155,94,27,disabled?22:3);
   brief(s,x+3,158,catalogue[t].name,15,disabled?22:4);
   labelnum(s,x+3,170,"",catalogue[t].cost,disabled?22:5);
   b[0]=(char)('0'+catalogue[t].w);b[1]='X';b[2]=(char)('0'+catalogue[t].h);b[3]=0;ui_text(s,x+68,170,b,18);
  }
 }else if(p->category==STAFF){
  button(s,4,155,152,27,"SECURITY +",0,p->cash<500);button(s,163,155,152,27,"MECHANIC +",0,p->cash<500);
  labelnum(s,9,175,"HIRED ",p->security,5);labelnum(s,168,175,"HIRED ",p->mechanics,5);
 }else if(p->category==LANDSCAPE){
  button(s,4,155,101,27,"PATH 20",p->tool==PATH,0);button(s,109,155,101,27,"TREE 50",p->tool==TREE,0);
  button(s,214,155,101,27,"DEMOLISH",p->tool==ERASE,0);
 }else{
  labelnum(s,7,157,"ENTRY ",p->ticket,5);button(s,73,155,18,13,"-",0,0);button(s,94,155,18,13,"+",0,0);
  button(s,118,155,69,13,p->park_open?"PARK OPEN":"CLOSED",p->park_open,0);
  button(s,193,155,57,13,p->paused?"RESUME":"PAUSE",p->paused,0);button(s,256,155,59,13,"NEW PARK",0,0);
  labelnum(s,7,175,"TOTAL INCOME ",p->earned,4);str_cat(str_cat(b,"DETAIL "),fair_modes[fair_detail].name);button(s,181,171,134,12,b,0,0);
 }
 button(s,0,186,56,14,"RIDES",p->category==RIDES,0);button(s,56,186,56,14,"STANDS",p->category==STANDS,0);
 button(s,112,186,56,14,"STAFF",p->category==STAFF,0);button(s,168,186,56,14,"PATHS",p->category==LANDSCAPE,0);
 button(s,224,186,56,14,"PARK",p->category==PARK_MENU,0);button(s,280,186,40,14,"HELP",0,0);
 if(!p->started){
  panel(s,16,25,288,148);ui_text(s,70,37,"A M I F U N F A I R",5);
  ui_text(s,43,53,"BUILD A FAIR. BRING IT TO LIFE.",18);
  ui_text(s,35,76,"CHOOSE YOUR STARTING DIFFICULTY",4);
  button(s,26,94,84,30,"EASY",0,0);button(s,118,94,84,30,"MEDIUM",0,0);button(s,210,94,84,30,"HARD",0,0);
  ui_text(s,44,114,"500,000",5);ui_text(s,136,114,"250,000",5);ui_text(s,228,114,"100,000",5);
  button(s,62,135,196,22,"PLAY SHOWCASE PARK",0,0);ui_text(s,74,162,"AN AMI3D PROTOTYPE",23);
 }
 if(p->show_help){
  panel(s,10,18,300,162);ui_text(s,82,27,"FAIRGROUND HANDBOOK",5);
  ui_text(s,18,43,"1. CHOOSE A RIDE OR STAND BELOW.",4);
  ui_text(s,18,55,"2. R / Z ROTATE; CLICK TO BUILD.",4);
  ui_text(s,18,67,"3. PLACE RIDE ENTRANCE, THEN EXIT.",4);
  ui_text(s,18,79,"4. PATH BOTH GATES TO THE FRONT GATE.",4);
  ui_text(s,18,91,"CLICK RIDES TO SET PRICE/COLOUR/SPEED.",18);
  ui_text(s,18,103,"ARROWS PAN. Q/E ORBIT. +/- OR WHEEL ZOOM.",4);
  ui_text(s,18,115,"DRAG TO PAINT PATHS. RMB/ESC CANCEL.",4);
  ui_text(s,18,127,"SPACE PAUSE. D DETAIL. F10 QUITS.",4);
  ui_text(s,18,139,"STAFF: 500 HIRE; 250/350 DAILY WAGES.",23);
  ui_text(s,18,151,"RIDES WEAR OUT. MECHANICS REPAIR THEM.",23);
  ui_text(s,52,166,"CLICK OR H TO RETURN TO PARK",5);
 }
 /* Software pointer is shared by SDL and the Amiga hardware platform. */
 if(!fair_hardware_pointer){
 for(i=0;i<9;i++){ui_pixel(s,in->mx,in->my+i,1);ui_pixel(s,in->mx+i/2,in->my+i,1);}
 for(i=0;i<7;i++)ui_pixel(s,in->mx+1,in->my+i,31);
 }
}
static void inspect_click(Park *p,int x,int y){
 Attraction *a=p->selected>=0?&p->items[p->selected]:0;
 if(p->tool==BUILD){
  if(hit(x,y,213,58,47,12))p->rotation=(p->rotation+3)&3;
  if(hit(x,y,265,58,47,12))p->rotation=(p->rotation+1)&3;
  if(hit(x,y,295,73,17,12))p->colour=(p->colour+1)%6;
  if(hit(x,y,295,88,17,12))p->speed=p->speed%3+1;
 }else if(a&&p->pending<0){
  if(hit(x,y,274,32,17,12)&&a->price)a->price--;
  if(hit(x,y,295,32,17,12)&&a->price<100)a->price++;
  if(hit(x,y,213,48,59,12))a->open=!a->open;
  if(hit(x,y,277,48,35,12))park_remove(p,a->x,a->y);
  if(hit(x,y,295,63,17,12))a->colour=(a->colour+1)%6;
  if(hit(x,y,295,78,17,12))a->speed=a->speed%3+1;
 }
}
void game_input(Park *p,const Input *in){
 int x,y;if(in->key==KEY_DETAIL){cycle_detail(p);return;}
 if(in->key==KEY_HELP){p->show_help=!p->show_help;return;}
 if(p->show_help){if(in->left||in->key==KEY_ESC)p->show_help=0;return;}
 if(!p->started){
  if(in->left&&in->my>=94&&in->my<125&&in->mx>=26&&in->mx<294){
   int d=in->mx<114?0:in->mx<206?1:2;park_init(p,d);p->started=1;
  }else if(in->left&&hit(in->mx,in->my,62,135,196,22))park_demo(p);
  return;
 }
 if(in->right||in->key==KEY_ESC)park_cancel(p);
 if(in->key==KEY_ROTATE)p->rotation=(p->rotation+1)&3;
 if(in->key==KEY_BACKROTATE)p->rotation=(p->rotation+3)&3;
 if(in->key==KEY_LEFT)p->camera_x-=64;if(in->key==KEY_RIGHT)p->camera_x+=64;
 if(in->key==KEY_UP)p->camera_y+=64;if(in->key==KEY_DOWN)p->camera_y-=64;
 if(in->key==KEY_ORBIT_L)p->camera_yaw=(p->camera_yaw-128)&4095;
 if(in->key==KEY_ORBIT_R)p->camera_yaw=(p->camera_yaw+128)&4095;
 if(in->key==KEY_ZOOM_IN||in->wheel>0)p->camera_distance-=100;
 if(in->key==KEY_ZOOM_OUT||in->wheel<0)p->camera_distance+=100;
 p->camera_x=iclamp(p->camera_x,0,2048);p->camera_y=iclamp(p->camera_y,0,2048);
 p->camera_distance=iclamp(p->camera_distance,550,3500);
 if(in->key==KEY_PAUSE)p->paused=!p->paused;
 if(in->key==KEY_TAB){p->category=(p->category+1)%5;p->page=0;}
 p->hover_x=p->hover_y=-1;
 if(!(inspector(p)&&hit(in->mx,in->my,208,17,109,112))&&scene_pick(p,in->mx,in->my,&x,&y)){
  p->hover_x=x;p->hover_y=y;
  if(in->left||(in->held&&(p->tool==PATH||p->tool==TREE))){
   if(p->tool==SELECT&&y>=29&&x>=14&&x<=18){p->category=PARK_MENU;p->selected=-1;}
   else park_place(p,x,y);
  }
 }
 if(!in->left)return;x=in->mx;y=in->my;
 if(inspector(p)&&hit(x,y,208,17,109,112)){inspect_click(p,x,y);return;}
 if(y>=186){int c=x/56;if(c>=5){p->show_help=1;return;}
  p->category=c;p->page=0;return;
 }
 if(y<152)return;
 if(p->category==RIDES||p->category==STANDS){int list[ITEM_COUNT],n=0,i;
  for(i=0;i<ITEM_COUNT;i++)if(catalogue[i].group==p->category)list[n++]=i;
  if(x<14){p->page=(p->page+(n+2)/3-1)%((n+2)/3);return;}
  if(x>=307){p->page=(p->page+1)%((n+2)/3);return;}
  i=p->page*3+(x-16)/97;if(x>=16&&i<n)park_choose(p,list[i]);
 }else if(p->category==STAFF){park_hire(p,x>=160);}
 else if(p->category==LANDSCAPE){park_cancel(p);p->selected=-1;p->tool=x<107?PATH:x<212?TREE:ERASE;
  park_message(p,p->tool==PATH?"Drag paths to connect rides and stands to the gate.":p->tool==TREE?"Plant trees for 50 each.":"Click to sell a ride or remove a path/tree.");
 }else{
  if(hit(x,y,181,171,134,12))cycle_detail(p);
  if(hit(x,y,73,155,18,13)&&p->ticket)p->ticket--;
  if(hit(x,y,94,155,18,13)&&p->ticket<100)p->ticket++;
  if(hit(x,y,118,155,69,13))p->park_open=!p->park_open;
  if(hit(x,y,193,155,57,13))p->paused=!p->paused;
  if(hit(x,y,256,155,59,13)){park_cancel(p);p->started=0;}
 }
}
