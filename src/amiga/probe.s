/**
 * File: src/amiga/probe.s
 * Author: Diego Parrilla Santamaría
 * Date: 2026-11-11
 * Copyright: 2024-26 - GOODDATA LABS SL
 * Description: Reads and writes that may fault, for the Gayle probe.
 */

/*
 * long amiga_probe_word(unsigned long address)
 *
 * Reads the word at address with a temporary bus-error handler: returns the
 * word (0 to 0xFFFF) when the read completes, -1 when it faults. The Gayle ID
 * register ($DE1000) exists only on the A600 and A1200; nothing documents what
 * other models do there (EPIC-06 STORY-01). The same pattern as the ST's
 * st_probe_read (probe.s):
 * supervisor mode, interrupts masked, the fault handler drops the exception
 * frame by restoring the stack pointer. Runs from RAM.
 */
    .text
    .globl _amiga_probe_word
_amiga_probe_word:
    move.l  4(%sp),%a0          /* the address to probe */
    move.l  0x8.w,%a1           /* save the bus-error vector */
    move.l  %sp,%d1             /* the stack to come back to */
    move.l  #probe_fault,0x8.w
    moveq   #0,%d0
    move.w  (%a0),%d0           /* may fault */
    bra.s   probe_done
probe_fault:
    move.l  %d1,%sp             /* drop the bus-error frame */
    moveq   #-1,%d0
probe_done:
    move.l  %a1,0x8.w           /* restore the vector */
    rts

/*
 * long amiga_probe_write_byte(unsigned long address, unsigned long value)
 *
 * Writes the low byte of value at address with the same temporary bus-error
 * handler: returns 0 when the write completes, -1 when it faults.
 */
    .globl _amiga_probe_write_byte
_amiga_probe_write_byte:
    move.l  4(%sp),%a0          /* the address to write */
    move.l  8(%sp),%d0          /* the byte, in the low 8 bits */
    move.l  0x8.w,%a1
    move.l  %sp,%d1
    move.l  #write_fault,0x8.w
    move.b  %d0,(%a0)           /* may fault */
    moveq   #0,%d0
    bra.s   write_done
write_fault:
    move.l  %d1,%sp
    moveq   #-1,%d0
write_done:
    move.l  %a1,0x8.w
    rts
