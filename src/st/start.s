/**
 * File: src/st/start.s
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST bootstrap and ROM entry assembly.
 */

.globl _start
.extern _rom_switcher_main

#include "mem.h"

.text
_start:
    /*
     * If already in supervisor mode (SR.S=1), skip GEMDOS Super().
     * Otherwise, call GEMDOS Super(0) to enter supervisor mode.
     */
    move.w  %sr, %d0
    andi.w  #0x2000, %d0
    bne.s   have_supervisor

    clr.l   -(%sp)
    move.w  #0x20, -(%sp)
    trap    #1
    addq.l  #6, %sp

    /* On error GEMDOS returns a negative value in D0. */
    tst.l   %d0
    bmi.s   halt

have_supervisor:
    move.l  #ST_STACK_TOP_ADDR_UL, %sp
    jsr     _rom_switcher_main

halt:
    bra.s   halt
