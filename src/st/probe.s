/**
 * File: src/st/probe.s
 * Author: Diego Parrilla Santamaría
 * Date: 2026-10-09
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Probe a hardware address for the machine information.
 */

/*
 * int st_probe_read(unsigned long address)
 *
 * Reads one byte at address with a temporary bus-error handler: returns 1 when
 * the read completes, 0 when it faults (the hardware is not there). Runs in
 * supervisor mode with interrupts masked, as the whole ROM does; the fault
 * handler drops the exception frame by restoring the stack pointer
 * (EPIC-03 STORY-09).
 */
    .text
    .globl _st_probe_read
_st_probe_read:
    move.l  4(%sp),%a0          /* the address to probe */
    move.l  0x8.w,%a1           /* save the bus-error vector */
    move.l  %sp,%d1             /* the stack to come back to */
    move.l  #probe_fault,0x8.w
    moveq   #1,%d0
    tst.b   (%a0)               /* may fault */
    bra.s   probe_done
probe_fault:
    move.l  %d1,%sp             /* drop the bus-error frame */
    moveq   #0,%d0
probe_done:
    move.l  %a1,0x8.w           /* restore the vector */
    rts
