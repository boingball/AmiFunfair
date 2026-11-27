#include "game.h"
#include <string.h>
int game_run(void){
 static Park park;struct surf s;Input in;uint32_t last,now,acc=0;
 park_init(&park,0);if(!platform_init())return 20;last=platform_ms();
 while(platform_begin(&s)){
  memset(&in,0,sizeof(in));platform_input(&in);if(in.quit||in.key==KEY_QUIT)break;
  game_input(&park,&in);now=platform_ms();acc+=now-last;last=now;
  /* Limit catch-up after a slow frame; simulation never depends on draw rate. */
  if(acc>500)acc=500;while(acc>=50){park_tick(&park);acc-=50;}
  scene_draw(&park,&s);ui_draw(&park,&s,&in);platform_end();
 }
 platform_shutdown();return 0;
}
