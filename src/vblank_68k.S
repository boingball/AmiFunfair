/* Native field switching, adapted from AmiPool's level-3/VBR glue. */
#include "asm.h"
        .text
        .globl SYM(fair_get_vbr)
SYM(fair_get_vbr):
        movem.l %a5-%a6,-(%sp)
        move.l SYM(SysBase),%a6
        lea 1f(%pc),%a5
        jsr -30(%a6)                 /* Supervisor */
        move.l %d0,%a0
        movem.l (%sp)+,%a5-%a6
        rts
1:      .short 0x4e7a,0x0801          /* movec vbr,d0 (68020+ game) */
        rte
        .globl SYM(fair_lev3)
SYM(fair_lev3):
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        btst #5,0xdff01f
        beq.s 2f
        jsr SYM(fair_vblank)
2:      move.w #0x0070,0xdff09c
        move.w #0x0070,0xdff09c
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte
        NO_EXEC_STACK
