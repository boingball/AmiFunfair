#ifndef GAME_H
#define GAME_H
#include "park.h"
#include "text.h"
#include "detail.h"
#define SCREEN_W 320
#define SCREEN_H 200
#define VIEW_TOP 14
#define VIEW_BOTTOM 137
enum { KEY_NONE,KEY_ESC,KEY_ROTATE,KEY_BACKROTATE,KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,
 KEY_ORBIT_L,KEY_ORBIT_R,KEY_ZOOM_IN,KEY_ZOOM_OUT,KEY_PAUSE,KEY_HELP,KEY_QUIT,KEY_TAB,KEY_DETAIL,KEY_VIEW };
typedef struct {int mx,my,left,right,held,key,wheel,quit;} Input;
extern const uint32_t fair_palette[32];
typedef struct {int x0,y0,x1,y1;} MapWindow;
typedef struct {int cells,items,trees,guests;} SceneCounts;
extern SceneCounts scene_counts;
extern int scene_culling; /* Reference-render comparison switch; normally 1. */
void scene_visible_map(const Park *p,MapWindow *out);
void scene_camera(const Park *p);
void topdown_draw(Park *p,const struct surf *s);
int topdown_pick(const Park *p,int mx,int my,int *x,int *y);
void topdown_project(const Park *p,int wx,int wy,int *sx,int *sy);
void topdown_visible(const Park *p,MapWindow *out);
void scene_cache_bind(void *memory,uint32_t bytes);
void scene_draw(Park *p,const struct surf *s);
int scene_pick(const Park *p,int mx,int my,int *x,int *y);
void ui_draw(Park *p,const struct surf *s,const Input *in);
void game_input(Park *p,const Input *in);
void ui_text(const struct surf *s,int x,int y,const char *text,int colour);
int game_run(void);
int platform_init(void);
int platform_begin(struct surf *s);
void platform_end(void);
void platform_input(Input *in);
uint32_t platform_ms(void);
void platform_shutdown(void);
#endif
