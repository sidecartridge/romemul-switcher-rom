---
id: STORY-03
epic: EPIC-04
title: The ST keyboard keeps its keys
status: done
---

## Goal

A key pressed on the ST or STE is never lost, whether through an ACIA overrun or a long gap
between polls.

## Tasks

- [x] `src/st/kbd.c`, `kbd_poll_scancode()`: on an overrun (bit 5 of the ACIA status), the
  byte in the receive register is valid, so it is kept. Only a framing or parity error drops
  the byte.
- [x] The empty-poll delay of 2,500 loops (`kPollDelayLoops`) goes, or shrinks to what the
  ACIA needs. That gap is about 28 ms in a debug build, measured in Hatari (EPIC-00 STORY-02).
- [x] Measure the poll gap in Hatari before and after, in a debug build and a release build.
  Closed by Diego, 2026-11-11, on the lost-key test below instead of a timing.
- [x] Harness: all sessions pass with no key retries, and the soak's ESC still stops it.

## Acceptance

`all_harness.sh` passes with 0 key retries. On hardware (STORY-05): quick key presses in the
list (arrows held down, RETURN then ESC at once) are never lost on an ST.

## Notes

- From iteration 1's candidates (2026-10-09, EPIC-00 STORY-02).
- The IKBD sends at 7,812.5 baud, so a byte arrives about every 1.28 ms. The ACIA holds one
  byte, so a 28 ms gap can lose a key release and the next key's make code.

**Done 2026-11-11**, on `epic-04-carried-defects` (code in `eab5634`).

- **The error mask.** `kIkbdStatusErrMask` (0x70) became `kIkbdStatusBadByteMask` (0x50). A
  framing or parity error still drops the byte, but an overrun no longer does.
- **The delay.** `kbdDelay()` and `kPollDelayLoops` are gone. `kbd_poll_scancode_wait()` now
  polls without a gap.
- **Measured as lost keys, not as a time.** Diego, 2026-11-11: "close timing task". The gap
  itself was the 2,500-loop delay, which is gone, so the polls now follow one another directly. The poll gap itself was not timed. What was
  measured instead is whether keys are lost. The Hatari harness waits 0.2 s after each key's
  release (`RELEASE_PAUSE`), which hid the gap, so the ST session was run with no pause:
  - the old code, 2 runs: both FAIL, a lost key each time ("key right" and "key down" after two
    presses);
  - the new code, 3 runs: all PASS, 63 steps, 0 key retries.

  The pause stays in the harness as margin, and its comment now says why.
- **The full run.** `all_harness.sh` passed 31 of 31 with 0 key retries.
- **ESC during the soak.** Checked in a separate scripted Hatari session (ST, no key pause):
  T, S, then ESC half a second later gave `@@ soak end seconds=6 reads=31095 errors=0 ...
  stopped=1`. No harness session presses ESC mid-soak; the `--soak` sessions run the full 30 s.
- The release build is not measured separately. It had no trace to time, and it shares the
  code path.
