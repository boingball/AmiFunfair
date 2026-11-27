#include "game.h"
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
static Park p;
int main(void){
 struct surf s;Input in={0};SDL_Event e={0};int i;
 assert(platform_init());park_demo(&p);
 for(i=0;i<12;i++){
  const FairDetail *d=&fair_modes[i%3];assert(fair_request_detail(i%3));
  assert(platform_begin(&s));assert(s.w==d->w&&s.h==d->h&&fair_detail==i%3);
  p.view_2d=i&1;scene_draw(&p,&s);ui_draw(&p,&s,&in);platform_end();
 }
 e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_d;assert(SDL_PushEvent(&e)==1);
 platform_input(&in);assert(in.key==KEY_DETAIL);
 e.key.keysym.sym=SDLK_v;assert(SDL_PushEvent(&e)==1);in.key=0;platform_input(&in);assert(in.key==KEY_VIEW);
 platform_shutdown();puts("SDL: live detail changes, texture recreation and D input passed");return 0;
}
