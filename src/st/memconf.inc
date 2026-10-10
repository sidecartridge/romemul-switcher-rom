/**
 * File: src/st/memconf.inc
 * Author: Diego Parrilla Santamaría
 * Date: 2026-10-10
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: The RAM bank probe of the ROM startup (EPIC-03 STORY-13),
 * included by startup_st.s and startup_ste.s.
 *
 * The bank sizing of TOS 1.6 and EmuTOS (bios/memory.S, memconf): the MMU is
 * set to two 2 MB banks and a 252-word pattern is written at offset 8 of each
 * bank. Where the pattern shows up again tells the size of the bank: a 128 KB
 * bank repeats it at 0x40008 (STE) or 0x208 (ST), a 512 KB bank at 0x80008
 * (STE) or 0x408 (ST); a bank with no repeat but the pattern at 0x008 holds
 * 2 MB, and one without the pattern is empty. Bank 1 is sized first.
 *
 * No RAM may be used while the MMU is changed, not even a stack: every
 * address moves. The caller restores its own MMU value afterwards.
 *
 * Out: d5 the RAM in bytes; d6 the MMU value for it (bank 0 in bits 2 and 3,
 * bank 1 in bits 0 and 1). Uses d0, d1, d4, a0, a1, a5. Overwrites RAM at
 * 0x000008-0x0001FF and 0x200008-0x2001FF (as the MMU maps them).
 */

    move.b  #0x0a, 0xffff8001.w     /* two banks of 2 MB */
    movea.w #0x0008, %a0
    lea     0x200008, %a1
    moveq   #0, %d0
memconf_fill:
    move.w  %d0, %a0@+
    move.w  %d0, %a1@+
    add.w   #0xfa54, %d0
    cmpa.l  #0x200, %a0
    bne.s   memconf_fill

    move.l  #0x200000, %d1          /* bank 1 first, then bank 0 */
    moveq   #0, %d5
    moveq   #0, %d6
memconf_bank:
    lsr.w   #2, %d6
    move.l  #0x20000, %d4
    movea.l #0x40008, %a0           /* 128 KB, STE */
    lea     memconf_128k_st(%pc), %a5
    bra.s   memconf_check
memconf_128k_st:
    beq.s   memconf_bank_end
    movea.w #0x208, %a0             /* 128 KB, ST */
    lea     memconf_512k_ste(%pc), %a5
    bra.s   memconf_check
memconf_512k_ste:
    beq.s   memconf_bank_end
    move.l  #0x80000, %d4
    movea.l #0x80008, %a0           /* 512 KB, STE */
    lea     memconf_512k_st(%pc), %a5
    bra.s   memconf_check
memconf_512k_st:
    beq.s   memconf_bank_512k
    movea.w #0x408, %a0             /* 512 KB, ST */
    lea     memconf_2m(%pc), %a5
    bra.s   memconf_check
memconf_2m:
    beq.s   memconf_bank_512k
    movea.w #0x008, %a0             /* 2 MB, or nothing */
    lea     memconf_last(%pc), %a5
    moveq   #0, %d4
    bra.s   memconf_check
memconf_last:
    bne.s   memconf_bank_end        /* no pattern: an empty bank */
    move.l  #0x200000, %d4
    addq.w  #4, %d6
memconf_bank_512k:
    addq.w  #4, %d6
memconf_bank_end:
    add.l   %d4, %d5
    sub.l   #0x200000, %d1
    beq.s   memconf_bank            /* bank 1 done: now bank 0 */
    bra.s   memconf_done

    /* Z set when the 252 words at d1 + a0 are the pattern. Returns to a5. */
memconf_check:
    adda.l  %d1, %a0
    moveq   #0, %d0
    lea     0x1f8(%a0), %a1
memconf_check_loop:
    cmp.w   %a0@+, %d0
    bne.s   memconf_check_end
    add.w   #0xfa54, %d0
    cmpa.l  %a0, %a1
    bne.s   memconf_check_loop
memconf_check_end:
    jmp     %a5@

memconf_done:
