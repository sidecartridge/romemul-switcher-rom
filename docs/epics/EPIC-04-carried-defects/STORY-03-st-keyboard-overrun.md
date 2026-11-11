---
id: STORY-03
epic: EPIC-04
title: The ST keyboard keeps its keys
status: todo
---

## Goal

A key pressed on the ST or STE is never lost, whether through an ACIA overrun or a long gap
between polls.

## Tasks

- [ ] `src/st/kbd.c`, `kbd_poll_scancode()`: on an overrun (bit 5 of the ACIA status), the
  byte in the receive register is valid, so it is kept. Only a framing or parity error drops
  the byte.
- [ ] The empty-poll delay of 2,500 loops (`kPollDelayLoops`) goes, or shrinks to what the
  ACIA needs. That gap is about 28 ms in a debug build, measured in Hatari (EPIC-00 STORY-02).
- [ ] Measure the poll gap in Hatari before and after, in a debug build and a release build.
- [ ] Harness: all sessions pass with no key retries, and the soak's ESC still stops it.

## Acceptance

`all_harness.sh` passes with 0 key retries. On hardware (STORY-05): quick key presses in the
list (arrows held down, RETURN then ESC at once) are never lost on an ST.

## Notes

- From iteration 1's candidates (2026-10-09, EPIC-00 STORY-02).
- The IKBD sends at 7,812.5 baud, so a byte arrives about every 1.28 ms. The ACIA holds one
  byte, so a 28 ms gap can lose a key release and the next key's make code.
