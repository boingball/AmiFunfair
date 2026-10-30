#ifndef FAIR_NATIVE_VIDEO_H
#define FAIR_NATIVE_VIDEO_H
#include <stdint.h>
#define FAIR_COPPER_WORDS 512
/* Returns words written. One list per frame buffer and interlace field. */
int fair_native_copper(uint16_t *out,uint32_t planes,uint32_t null_sprite,
                      int detail,int field,int aga,const uint32_t *palette);
#endif
