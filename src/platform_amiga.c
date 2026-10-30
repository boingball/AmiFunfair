/* AmiFunFair Amiga platform, adapted from Ami3D demo/amiga.c.
 * Five bitplanes, double buffered, 320x200 within PAL and NTSC display bounds.
 * Screen takeover/startup/restore glue remains shared with the pinned engine.
 * Fast RAM is used for drawing when available, then copied to the chip screen.
 */
#include "game.h"
#include "render.h"
#define DEMO_W SCREEN_W
#define DEMO_H SCREEN_H
#define DEMO_NAME "AmiFunFair"
#define demo_palette fair_palette
#define demo_sky(y) fair_palette[0]
#include "hw.h"

#define ROWB (DEMO_W / 8)
#define PLANE ((u32)ROWB * DEMO_H)
#define SCREEN (PLANE * 5)
#define COP_WORDS 6000
#define COP_LISTS 2
#define COP_MEM (COP_WORDS * 2 * COP_LISTS + 64)   /* (the lists, the null sprite) */
#define TOP 0x2c     /* 44..243 fits PAL and NTSC */

static void *gfxbase, *old_view;
static u32 old_cop;
static u16 old_dmacon, old_intena, old_adkcon;
static int taken, aga, shown;
static void *screen_mem[2], *chip_mem;
static u8 *fast;
static u8 *screen[2];
static u16 *cop[2][2];
static u32 *null_sprite;

/* gcc may call these */
void *memset(void *d, int c, unsigned long n)
{
    u8 *p = d;
    while (n--) *p++ = (u8)c;
    return d;
}

void *memcpy(void *d, const void *s, unsigned long n)
{
    u8 *p = d;
    const u8 *q = s;
    while (n--) *p++ = *q++;
    return d;
}

static void print(const char *s)
{
    long n = 0;
    while (s[n]) n++;
    os_print(s, n);
}

/* ---- time ---- */

static u32 tod_b(void)
{
    u32 hi = CIAB_TODHI;   /* reading hi latches the counter */
    u32 mid = CIAB_TODMID;
    u32 lo = CIAB_TODLO;   /* reading lo releases the latch */
    return (hi << 16) | (mid << 8) | lo;
}

/* CIA-B's time of day counts horizontal syncs: raster lines, 24 bits,
 * carried on past the wrap. */
uint32_t plat_lines(void)
{
    static u32 last, high;
    u32 now = tod_b();
    if (now < last) high += 1UL << 24;
    last = now;
    return high + now;
}

static int vpos(void) { return (int)((REGL(VPOSR) >> 8) & 0x1ff); }

static void wait_line(void)
{
    int v = vpos();
    while (vpos() == v) ;
}

static void wait_blit(void)
{
    (void)REG(DMACONR);                  /* early Agnus: the first read lies */
    while (REG(DMACONR) & 0x4000) ;      /* BBUSY */
}

/* ---- the display ---- */

static inline u16 hi12(u32 c)
{
    return (u16)(((c >> 12) & 0xf00) | ((c >> 8) & 0x0f0) | ((c >> 4) & 0x00f));
}

static inline u16 lo12(u32 c)
{
    return (u16)(((c >> 8) & 0xf00) | ((c >> 4) & 0x0f0) | (c & 0x00f));
}

#define MOVE(r, v) do { *c++ = (u16)(r); *c++ = (u16)(v); } while (0)
#define WAIT(y) do { *c++ = (u16)((((y) & 255) << 8) | 0x07); *c++ = 0xfffe; } while (0)

/* A colour register: AGA's 24 bits as two halves (LOCT), OCS's 12. */
static u16 *colour(u16 *c, int reg, u32 rgb)
{
    if (aga) {
        MOVE(BPLCON3, 0x0c00);
        MOVE(reg, hi12(rgb));
        MOVE(BPLCON3, 0x0e00);
        MOVE(reg, lo12(rgb));
    } else {
        MOVE(reg, hi12(rgb));
    }
    return c;
}

/* half: each line shown twice (the modulos -ROWB on the first of each
 * pair, 0 on the second) */
static void build_copper(u16 *c, const u8 *planes, int half)
{
    int i, wrapped = 0;
    u32 ns = (u32)null_sprite;
    MOVE(FMODE, aga ? 3 : 0);           /* AGA: 64-bit fetch, more bus for the CPU */
    MOVE(BPLCON0, 0x5200);
    MOVE(BPLCON1, 0);
    MOVE(BPLCON2, 0x0024);
    MOVE(BPLCON3, 0x0c00);
    MOVE(BPLCON4, 0x0011);
    MOVE(DIWSTRT, (TOP << 8) | 0x81);
    MOVE(DIWSTOP, ((TOP + DEMO_H) & 255) << 8 | 0xc1);
    MOVE(DDFSTRT, 0x0038);
    MOVE(DDFSTOP, aga ? 0x00b8 : 0x00d0);
    MOVE(BPL1MOD, half ? -ROWB : 0);
    MOVE(BPL2MOD, half ? -ROWB : 0);
    for (i = 0; i < 8; i++) {
        MOVE(SPR0PT + i * 4, ns >> 16);
        MOVE(SPR0PT + i * 4 + 2, ns & 0xffff);
    }
    for (i = 1; i < 32; i++) c = colour(c, COLOR00 + i * 2, demo_palette[i]);
    for (i = 0; i < 5; i++) {
        u32 a = (u32)planes + i * PLANE;
        MOVE(BPL1PT + i * 4, a >> 16);
        MOVE(BPL1PT + i * 4 + 2, a & 0xffff);
    }
    c = colour(c, COLOR00, demo_sky(0));
    for (i = 1; i <= DEMO_H; i++) {
        int y = TOP + i;
        if (y >= 256 && !wrapped) {
            *c++ = 0xffdf;
            *c++ = 0xfffe;
            wrapped = 1;
        }
        WAIT(y);
        if (half) {
            MOVE(BPL1MOD, i & 1 ? 0 : -ROWB);
            MOVE(BPL2MOD, i & 1 ? 0 : -ROWB);
        }
        c = colour(c, COLOR00, i < DEMO_H ? demo_sky(i) : 0);
    }
    *c++ = 0xffff;
    *c++ = 0xfffe;
}

void platform_shutdown(void)
{
    if (taken) {
        wait_blit();
        REG(DMACON) = 0x7fff;
        REG(INTENA) = 0x7fff;
        REG(INTREQ) = 0x7fff;
        REGL(COP1LC) = old_cop;
        REG(COPJMP1) = 0;
        REG(ADKCON) = 0x8000 | old_adkcon;
        REG(DMACON) = 0x8000 | old_dmacon;
        REG(INTENA) = 0x8000 | old_intena;
        gfx_loadview(gfxbase, old_view);
        gfx_waittof(gfxbase);
        gfx_waittof(gfxbase);
        os_permit();
        gfx_disownblitter(gfxbase);
        taken = 0;
    }
    if (fast) os_free(fast, SCREEN);
    fast = 0;
    if (screen_mem[0]) os_free(screen_mem[0], SCREEN + 8);
    if (screen_mem[1]) os_free(screen_mem[1], SCREEN + 8);
    if (chip_mem) os_free(chip_mem, COP_MEM);
    screen_mem[0] = screen_mem[1] = chip_mem = 0;
    if (gfxbase) os_closelib(gfxbase);
    gfxbase = 0;
}

int platform_init(void)
{
    int i;
    u8 *p;
    gfxbase = os_openlib("graphics.library", 33);
    if (!gfxbase) {
        print(DEMO_NAME ": needs graphics.library v33\n");
        return 0;
    }
    chip_mem = os_alloc(COP_MEM, MEMF_CHIP | MEMF_CLEAR);
    fast = os_alloc(SCREEN, MEMF_FAST | MEMF_CLEAR);
    for (i = 0; i < 2; i++) {
        screen_mem[i] = os_alloc(SCREEN + 8, MEMF_CHIP | MEMF_CLEAR);
        if (screen_mem[i]) screen[i] = (u8 *)(((u32)screen_mem[i] + 7) & ~7UL);
    }
    if (!chip_mem || !screen_mem[0] || !screen_mem[1]) {
        print(DEMO_NAME ": not enough chip RAM (needs about 100K)\n");
        platform_shutdown();
        return 0;
    }
    p = (u8 *)(((u32)chip_mem + 7) & ~7UL);
    null_sprite = (u32 *)(p + COP_WORDS * 2 * COP_LISTS);
    aga = (REG(0x07c) & 0xff) == 0xf8;    /* Lisa answers 0xf8 */
    for (i = 0; i < COP_LISTS; i++) {
        int k = COP_LISTS == 4 ? i >> 1 : i, half = COP_LISTS == 4 && (i & 1);
        cop[k][half] = (u16 *)(p + COP_WORDS * 2 * i);
        build_copper(cop[k][half], screen[k], half);
    }

    old_view = *(void **)((u8 *)gfxbase + 34);
    old_cop = *(u32 *)((u8 *)gfxbase + 38);
    gfx_loadview(gfxbase, 0);
    gfx_waittof(gfxbase);
    gfx_waittof(gfxbase);
    gfx_ownblitter(gfxbase);
    os_forbid();
    old_dmacon = REG(DMACONR);
    old_intena = REG(INTENAR);
    old_adkcon = REG(ADKCONR);
    wait_line();
    REG(INTENA) = 0x7fff;
    REG(INTREQ) = 0x7fff;
    REG(DMACON) = 0x7fff;
    REG(POTGO) = 0xff00;
    taken = 1;
    shown = 0;
    REGL(COP1LC) = (u32)cop[0][0];
    REG(COPJMP1) = 0;
    REG(DMACON) = DMAF_SETCLR | DMAF_DMAEN | DMAF_BPLEN | DMAF_COPEN | DMAF_BLTEN | DMAF_SPREN;
    return 1;
}

/* ---- frames ---- */

void clear_fast_68k(void *p, uint32_t bytes);
static void clear_display(void)
{
    u8 *b = fast ? fast : screen[1 - shown];
    if (fast) clear_fast_68k(fast, SCREEN);
    else {
    /* the blitter clears all five planes in one go: 1000 lines of 20 words */
    wait_blit();
    REG(BLTCON0) = 0x0100;
    REG(BLTCON1) = 0;
    REG(BLTDMOD) = 0;
    REGL(BLTDPT) = (u32)b;
    REG(BLTSIZE) = (u16)((5 * DEMO_H) << 6 | (ROWB / 2));
    wait_blit();
    }
}
int platform_begin(struct surf *s)
{
    u8 *b = fast ? fast : screen[1 - shown];
    /* scene_draw's r_clear invokes this once; no duplicate frame clear. */
    r_setup_planar(b, ROWB, (int32_t)PLANE, DEMO_W, DEMO_H, clear_display);
    s->buf = 0;
    s->w = DEMO_W;
    s->h = DEMO_H;
    s->planes = b;
    s->bpr = ROWB;
    s->psize = (int32_t)PLANE;
    s->pens = 0;
    return 1;
}

void platform_end(void)
{
    /* the copper starts from COP1LC at the vertical blank: the new screen
     * from then on, and the old one free to draw into */
    int half = 0;
    if (fast) {
        const u32 *src = (const u32 *)fast;
        u32 *dst = (u32 *)screen[1-shown], n = SCREEN/16;
        while (n--) { dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3]; src+=4; dst+=4; }
    }
    shown = 1 - shown;
    REGL(COP1LC) = (u32)cop[shown][half];
    REG(INTREQ) = 0x0020;
    while (!(REG(INTREQR) & 0x0020)) ;
}


/* Mouse counters are signed deltas; no Intuition is active during takeover. */
void platform_input(Input *in)
{
    static int mx=160,my=100,was_left,was_right,oldx,oldy,ready;
    static uint8_t held[128];
    static uint32_t repeat_at;
    uint16_t j=REG(JOY0DAT);
    int hx=j&255,hy=j>>8,left=!(CIAA_PRA&0x40),right=!(REG(POTGOR)&0x400);
    if(!ready){oldx=hx;oldy=hy;ready=1;}
    mx+=(int8_t)(hx-oldx);my+=(int8_t)(hy-oldy);oldx=hx;oldy=hy;
    if(mx<0)mx=0;if(mx>=SCREEN_W)mx=SCREEN_W-1;
    if(my<0)my=0;if(my>=SCREEN_H)my=SCREEN_H-1;
    in->mx=mx;in->my=my;in->left=left&&!was_left;in->right=right&&!was_right;
    in->held=left;was_left=left;was_right=right;
    if(CIAA_ICR&8){
        uint8_t v=(uint8_t)~CIAA_SDR;
        int code=((v>>1)|(v<<7))&255;
        uint8_t cra=CIAA_CRA;
        uint32_t at=plat_lines();
        CIAA_CRA=cra|0x40;
        while(plat_lines()-at<2){} /* keyboard handshake, at least 85 us */
        CIAA_CRA=cra&~0x40;
        held[code&127]=!(code&128);
        if(!(code&128)){
            switch(code){
            case 0x45:in->key=KEY_ESC;break;
            case 0x13:in->key=KEY_ROTATE;break;
            case 0x31:in->key=KEY_BACKROTATE;break;
            case 0x40:in->key=KEY_PAUSE;break;
            case 0x25:case 0x50:in->key=KEY_HELP;break;
            case 0x59:in->key=KEY_QUIT;break;
            case 0x42:in->key=KEY_TAB;break;
            default:break;
            }
        }
    }
    if(platform_ms()>=repeat_at){
        if(held[0x4f])in->key=KEY_LEFT;else if(held[0x4e])in->key=KEY_RIGHT;
        else if(held[0x4c])in->key=KEY_UP;else if(held[0x4d])in->key=KEY_DOWN;
        else if(held[0x10])in->key=KEY_ORBIT_L;else if(held[0x12])in->key=KEY_ORBIT_R;
        else if(held[0x0c]||held[0x5e])in->key=KEY_ZOOM_IN;
        else if(held[0x0b]||held[0x4a])in->key=KEY_ZOOM_OUT;
        repeat_at=platform_ms()+90;
    }
}
uint32_t platform_ms(void){uint32_t n=plat_lines();return (n/125)*8+(n%125)*64/1000;}
int main(void){return game_run();}
