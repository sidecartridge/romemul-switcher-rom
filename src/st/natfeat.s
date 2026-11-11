/**
 * File: src/st/natfeat.s
 * Author: Diego Parrilla Santamaría
 * Date: 2026-03-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Atari ST Native Features assembly helpers.
 */

.equ ILLEGAL_VECTOR, 0x10    /* vector 4: illegal instruction */

    .text

/*
 * long nf_get_id(const char *feature_name)
 *
 * Hatari answers the 0x7300 opcode with the feature's ID in d0. On a real ST
 * it is an illegal instruction, so the probe runs with the illegal-instruction
 * vector pointing at nfNoFeatures, which returns 0 ("no native features"), and
 * the vector is restored afterwards: a debug image put on a real ST runs
 * without traces instead of crashing (EPIC-04 STORY-04). The stack is as the
 * caller left it, so Hatari finds the argument at 4(sp).
 */
    .globl _nf_get_id
_nf_get_id:
    move.l  ILLEGAL_VECTOR, %a1
    move.l  #nfNoFeatures, ILLEGAL_VECTOR
    moveq   #0, %d0
    .short 0x7300
nfResume:
    move.l  %a1, ILLEGAL_VECTOR
    rts

/* The exception frame of a 68000: SR, then the PC of the illegal opcode. */
nfNoFeatures:
    moveq   #0, %d0
    move.l  #nfResume, 2(%sp)
    rte

/* Called only with an ID nf_get_id() returned, so only under Hatari. */
    .globl _nf_call
_nf_call:
    .short 0x7301
    rts
