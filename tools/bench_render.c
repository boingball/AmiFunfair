#include "game.h"
#include "render.h"
#include <stdio.h>
#include <time.h>
static Park p;static unsigned char planes[160000],cache[160000];
int platform_init(void){return 0;}int platform_begin(struct surf *s){(void)s;return 0;}void platform_end(void){}void platform_shutdown(void){}void platform_input(Input *i){(void)i;}uint32_t platform_ms(void){return 0;}
int main(void){struct surf s={0};Input in={0};clock_t start,end;int i;
park_demo(&p);for(i=0;i<1400;i++)park_tick(&p);
s.w=320;s.h=200;s.planes=planes;s.bpr=40;s.psize=8000;
#ifdef BENCH_CACHE
scene_cache_bind(cache,sizeof(cache));
#endif
start=clock();for(i=0;i<100;i++){p.ticks++;r_setup_planar(planes,40,8000,320,200,0);scene_draw(&p,&s);ui_draw(&p,&s,&in);}end=clock();
printf("100 planar scene+UI frames: %.3f s\n",(double)(end-start)/CLOCKS_PER_SEC);return 0;}
