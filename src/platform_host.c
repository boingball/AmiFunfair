/* Deterministic renderer/simulation smoke test. No display server required. */
#include "game.h"
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t pixels[MAX_SCREEN_W*MAX_SCREEN_H];
int platform_init(void){return 1;}
int platform_begin(struct surf *s){(void)s;return 0;}
void platform_end(void){}
void platform_input(Input *in){(void)in;}
uint32_t platform_ms(void){return 0;}
void platform_shutdown(void){}
int main(int argc,char **argv){
 static Park park;struct surf s={0};Input in={0};int i;
 const FairDetail *mode;
 const char *filename=argc>1?argv[1]:"build/showcase.ppm";
 park_demo(&park);for(i=0;i<1400;i++)park_tick(&park);
 if(argc>2){int t=atoi(argv[2]);if(t>=0&&t<ITEM_COUNT){park_init(&park,0);park.started=1;
  park_choose(&park,t);park_place(&park,14,17);park.rotation=0;park.selected=0;
  park.items[0].complete=1;park.items[0].connected=1;park.tool=SELECT;park.pending=-1;
  park.camera_x=1050;park.camera_y=1220;park.camera_distance=950;park.camera_yaw=400;park.ticks=500;
 }}
 if(argc>3)fair_request_detail(atoi(argv[3]));fair_detail=fair_requested_detail;mode=&fair_modes[fair_detail];
 r_setup(pixels,mode->w,mode->h);s.buf=pixels;s.w=mode->w;s.h=mode->h;
 in.mx=160;in.my=90;scene_draw(&park,&s);ui_draw(&park,&s,&in);
 {FILE *f=fopen(filename,"wb");if(!f){perror(filename);return 1;}
  fprintf(f,"P6\n%d %d\n255\n",mode->w,mode->h);for(i=0;i<mode->w*mode->h;i++){
   uint32_t c=fair_palette[pixels[i]&31];fputc(c>>16,f);fputc(c>>8,f);fputc(c,f);
  }fclose(f);
 }
 printf("Rendered %s; cash=%ld, guests=%d, arrivals=%u\n",filename,(long)park.cash,park_active(&park),park.arrivals);return 0;
}
