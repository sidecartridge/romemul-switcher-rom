/**
 * File: src/amiga/reset.s
 * Author: Diego Parrilla Santamaría
 * Date: 2026-11-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: The Amiga cold reset, run from RAM (EPIC-05 STORY-01).
 */

/*
 * void amiga_cold_reset(void)
 *
 * Commodore's cold reboot (exec ColdReboot(), and the "MagicResetCode" it
 * documented for V1.3): RESET asserts the reset line, the CIA reset turns the
 * overlay back on, and from then on the RAM this code runs from reads as ROM.
 * The 68000 has already prefetched the jmp, which goes through offset 2 of
 * whatever ROM the board serves now: the new image, once platform_hard_reset()
 * has waited for the firmware. Nothing here reads the old image. reset and jmp
 * sit in one longword, as Commodore asked for, though a 68000's one-word
 * prefetch would not need it; scripts/finalize_rom.py checks it in the map.
 */

    .text
    .balign 4
    .globl _amiga_cold_reset
_amiga_cold_reset:
    move.w  #0x2700, %sr
    lea     2, %a0
    .balignw 4, 0x4E71          /* nop words up to the longword */
    .globl _amiga_cold_reset_pair
_amiga_cold_reset_pair:
    reset
    jmp     (%a0)
