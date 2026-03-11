/**
 * File: src/st/startup_st.s
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST ROM startup assembly.
 */

.globl _os_entry
.globl _os_beg
.globl _os_magic
.globl _os_date
.globl _os_conf
.globl _os_dosdate
.globl _main

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

#ifndef ROM_BASE_ADDR_UL
#error "ROM_BASE_ADDR_UL is not defined"
#endif

.equ ST_DIAG_CART_BASE, 0x00FA0000
.equ ST_DIAG_CART_MAGIC, 0xFA52235F

.text
_os_entry:
    bra.s   _main
    .word   0x0104          /* os_version */
    .long   _main           /* reseth */
_os_beg:
    .long   ROM_BASE_ADDR_UL      /* os_beg */
    .long   0x000068CC      /* os_end */
    .long   _main      /* os_res1 */
_os_magic:
    .long   0x00E32572      /* os_magic */
_os_date:
    .long   0x20260302      /* os_date */  
_os_conf:
    .word   0x0007          /* os_conf */

_os_dosdate:
    .word   0x176E          /* os_dosdate */
    .long   0x00003DF6      /* os_root */
    .long   0x000010C9      /* os_kbshift */
    .long   0x00005CD4      /* os_run */
    .long   0x00000000      /* os_dummy */

.text
_main:

    move.w  #0x2700,sr
    move.b  #0x4, 0xffff8001.w  /* We need to set the MMU to run the program in RAM */

    move.l  #ST_STACK_TOP_ADDR_UL, %sp

    nop
    reset

    /*
     * Check for diagnostic cartridge.
     * If present, jump to cartridge entry at base+4 and return to nodiag.
     */
    cmpi.l  #ST_DIAG_CART_MAGIC, ST_DIAG_CART_BASE
    bne.s   nodiag
    lea     nodiag(%pc), %a6
    jmp     ST_DIAG_CART_BASE+4
nodiag:

    /* Copy RAM-resident code+rodata from ROM image to RAM VMA. */
    lea     __ram_code_rom_start, %a0
    lea     __ram_code_start, %a1
    lea     __ram_code_end, %a2
0:
    cmpa.l  %a2, %a1
    bcc.s   1f
    move.b  %a0@+, %a1@+
    bra.s   0b
1:
    /* Copy initialized data from ROM image to RAM VMA. */
    lea     __data_rom_start, %a0
    lea     __data_start, %a1
    lea     __data_end, %a2
2:
    cmpa.l  %a2, %a1
    bcc.s   3f
    move.b  %a0@+, %a1@+
    bra.s   2b
3:
    /* Clear BSS in RAM. */
    lea     __bss_start, %a0
    lea     __bss_end, %a1
4:
    cmpa.l  %a1, %a0
    bcc.s   5f
    clr.b   %a0@+
    bra.s   4b
5:
    jmp     _rom_switcher_main
