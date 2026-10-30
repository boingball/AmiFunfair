#include "game.h"
#include "render.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static uint8_t pixels[MAX_SCREEN_W*MAX_SCREEN_H];
static uint32_t argb[MAX_SCREEN_W*MAX_SCREEN_H];
static int mx=160,my=100;
int platform_init(void){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)){fprintf(stderr,"SDL: %s\n",SDL_GetError());return 0;}
 window=SDL_CreateWindow("AmiFunFair - Ami3D prototype",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,600,SDL_WINDOW_RESIZABLE);
 if(!window)goto fail;
 renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
 if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
 if(!renderer)goto fail;
 SDL_RenderSetLogicalSize(renderer,SCREEN_W,SCREEN_H);
 SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
 fair_detail_mask=(1u<<DETAIL_COUNT)-1;fair_detail=fair_requested_detail;
 texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,fair_modes[fair_detail].w,fair_modes[fair_detail].h);
 if(!texture)goto fail;SDL_ShowCursor(SDL_DISABLE);return 1;
 fail:fprintf(stderr,"SDL: %s\n",SDL_GetError());platform_shutdown();return 0;
}
int platform_begin(struct surf *s){
 const FairDetail *d;
 if(fair_requested_detail!=fair_detail){
  SDL_Texture *next=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,fair_modes[fair_requested_detail].w,fair_modes[fair_requested_detail].h);
  if(next){SDL_DestroyTexture(texture);texture=next;fair_detail=fair_requested_detail;}
  else fair_requested_detail=fair_detail;
 }
 d=&fair_modes[fair_detail];memset(s,0,sizeof(*s));s->buf=pixels;s->w=d->w;s->h=d->h;r_setup(pixels,d->w,d->h);return 1;
}
void platform_end(void){
 const FairDetail *d=&fair_modes[fair_detail];int i;for(i=0;i<d->w*d->h;i++)argb[i]=0xff000000|fair_palette[pixels[i]&31];
 SDL_UpdateTexture(texture,0,argb,d->w*4);SDL_RenderClear(renderer);SDL_RenderCopy(renderer,texture,0,0);SDL_RenderPresent(renderer);
 SDL_Delay(1);
}
static int key(SDL_Keycode k){switch(k){
 case SDLK_ESCAPE:return KEY_ESC;case SDLK_r:return KEY_ROTATE;case SDLK_z:return KEY_BACKROTATE;
 case SDLK_LEFT:return KEY_LEFT;case SDLK_RIGHT:return KEY_RIGHT;case SDLK_UP:return KEY_UP;case SDLK_DOWN:return KEY_DOWN;
 case SDLK_q:return KEY_ORBIT_L;case SDLK_e:return KEY_ORBIT_R;case SDLK_PLUS:case SDLK_EQUALS:case SDLK_KP_PLUS:return KEY_ZOOM_IN;
 case SDLK_MINUS:case SDLK_KP_MINUS:return KEY_ZOOM_OUT;case SDLK_SPACE:return KEY_PAUSE;case SDLK_h:case SDLK_F1:return KEY_HELP;
 case SDLK_d:return KEY_DETAIL;case SDLK_F10:return KEY_QUIT;case SDLK_TAB:return KEY_TAB;default:return KEY_NONE;
}}
void platform_input(Input *in){SDL_Event e;
 while(SDL_PollEvent(&e)){switch(e.type){
  case SDL_QUIT:in->quit=1;break;
  case SDL_MOUSEMOTION:mx=e.motion.x;my=e.motion.y;break;
  case SDL_MOUSEBUTTONDOWN:mx=e.button.x;my=e.button.y;if(e.button.button==SDL_BUTTON_LEFT)in->left=1;if(e.button.button==SDL_BUTTON_RIGHT)in->right=1;break;
  case SDL_MOUSEWHEEL:in->wheel=e.wheel.y;break;
  case SDL_KEYDOWN:if(!e.key.repeat||key(e.key.keysym.sym)==KEY_LEFT||key(e.key.keysym.sym)==KEY_RIGHT||key(e.key.keysym.sym)==KEY_UP||key(e.key.keysym.sym)==KEY_DOWN||key(e.key.keysym.sym)==KEY_ZOOM_IN||key(e.key.keysym.sym)==KEY_ZOOM_OUT)in->key=key(e.key.keysym.sym);break;
  default:break;
 }}in->mx=mx;in->my=my;in->held=!!(SDL_GetMouseState(0,0)&SDL_BUTTON_LMASK);
}
uint32_t platform_ms(void){return SDL_GetTicks();}
void platform_shutdown(void){SDL_ShowCursor(SDL_ENABLE);if(texture)SDL_DestroyTexture(texture);if(renderer)SDL_DestroyRenderer(renderer);if(window)SDL_DestroyWindow(window);SDL_Quit();}
#ifndef FAIR_NO_MAIN
int main(void){return game_run();}
#endif
