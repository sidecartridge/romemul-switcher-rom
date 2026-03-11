/**
 * File: src/st/natfeat.s
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST Native Features assembly helpers.
 */

    .text

    .globl _nf_get_id
_nf_get_id:
    .short 0x7300
    rts

    .globl _nf_call
_nf_call:
    .short 0x7301
    rts
