#ifndef PARK_H
#define PARK_H
#include <stdint.h>
#define MAP 32
#define MAX_ITEMS 96
#define MAX_GUESTS 120
#define ITEM_COUNT 21
#define GATE_X 16
#define GATE_Y 30
enum { RIDES, STANDS, STAFF, LANDSCAPE, PARK_MENU };
enum { SELECT, BUILD, ENTRANCE, EXIT, PATH, ERASE, TREE };
enum { BOUNCY, GHOST, WOODEN, CORKSCREW, MOUSE, ICECREAM, CANDY, TOILETS,
       COFFEE, CHIPS, TEACUPS, PIRATE, JETS, DODGEMS, CAROUSEL, HELTER,
       RAILWAY, SKI, TWISTER, WALTZER, ARCADE };
typedef struct {
    const char *name;
    uint8_t group,w,h,appeal,capacity;
    int32_t cost;
    uint16_t upkeep;
    uint8_t price;
} ItemDef;
typedef struct {
    uint8_t used,type,x,y,rotation,colour,speed,open,complete;
    int8_t ex,ey,xx,xy;
    uint8_t condition,connected;
    uint16_t visitors;
    int32_t income;
    uint8_t price;
} Attraction;
typedef struct {
    uint8_t used,x,y,nx,ny,progress,colour,leaving;
    uint16_t age;
    int8_t target;
    uint16_t riding;
} Guest;
typedef struct {
    int32_t cash,earned,spent;
    uint32_t ticks,rng;
    uint16_t day,arrivals;
    uint8_t difficulty,ticket,security,mechanics,paused,park_open;
    uint8_t paths[MAP][MAP],trees[MAP][MAP],reachable[MAP][MAP];
    /* Cached shortest routes, rebuilt only when the layout changes. */
    uint16_t routes[MAX_ITEMS+1][MAP][MAP];
    Attraction items[MAX_ITEMS];
    Guest guests[MAX_GUESTS];
    int selected,pending,tool,buy_type,rotation,colour,speed;
    int hover_x,hover_y,category,page,started,show_help;
    int camera_x,camera_y,camera_yaw,camera_distance;
    char message[100];
} Park;
extern const ItemDef catalogue[ITEM_COUNT];
void park_init(Park *p,int difficulty);
int park_choose(Park *p,int type);
int park_place(Park *p,int x,int y);
void park_cancel(Park *p);
void park_dimensions(int type,int rot,int *w,int *h);
int park_can_place(const Park *p,int type,int x,int y,int rot);
int park_at(const Park *p,int x,int y);
int park_gate_valid(const Park *p,int id,int x,int y);
int park_path(Park *p,int x,int y);
int park_tree(Park *p,int x,int y);
int park_remove(Park *p,int x,int y);
int park_hire(Park *p,int mechanic);
void park_connect(Park *p);
void park_tick(Park *p);
int park_active(const Park *p);
void park_message(Park *p,const char *s);
void park_demo(Park *p);
#endif
