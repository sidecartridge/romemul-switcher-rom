/**
 * File: src/amiga/startup.s
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Amiga ROM bootstrap and reset entry.
 */

.globl _amiga_boot

.extern _rom_switcher_main
.extern __ram_code_rom_start
.extern __ram_code_start
.extern __ram_code_end
.extern __data_rom_start
.extern __data_start
.extern __data_end
.extern __bss_start
.extern __bss_end

#include "mem.h"

.equ CIAA_PRA,  0x00BFE001
.equ CIAA_DDRA, 0x00BFE201
.equ COLOR00,   0x00DFF180

.text
    reset
_amiga_boot:
    move.w  #0x2700, %sr
    move.l  #AMIGA_STACK_TOP_ADDR_UL, %sp

    /* Map chip RAM at address 0 before touching any linked RAM. */
    move.b  CIAA_DDRA, %d0
    ori.b   #0x03, %d0
    move.b  %d0, CIAA_DDRA
    move.b  CIAA_PRA, %d0
    andi.b  #0xFE, %d0
    ori.b   #0x02, %d0
    move.b  %d0, CIAA_PRA

    lea     __ram_code_rom_start, %a0
    lea     __ram_code_start, %a1
    lea     __ram_code_end, %a2
1:
    cmpa.l  %a2, %a1
    bcc.s   2f
    move.b  %a0@+, %a1@+
    bra.s   1b

2:
    lea     __data_rom_start, %a0
    lea     __data_start, %a1
    lea     __data_end, %a2
3:
    cmpa.l  %a2, %a1
    bcc.s   4f
    move.b  %a0@+, %a1@+
    bra.s   3b

4:
    lea     __bss_start, %a0
    lea     __bss_end, %a1
5:
    cmpa.l  %a1, %a0
    bcc.s   6f
    clr.b   %a0@+
    bra.s   5b

6:
    jmp     _rom_switcher_main

    .balign 4
    .globl _platform_hard_reset
_platform_hard_reset:
    move.w  #0x0F00, COLOR00
    move.w  #0x2700, %sr
    lea     2, %a0
    reset
    jmp     (%a0)
