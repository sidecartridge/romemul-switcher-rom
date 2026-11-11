---
id: STORY-01
epic: EPIC-05
title: A reset that does not depend on the old image
status: todo
---

## Goal

`platform_hard_reset()` on the Amiga waits until the firmware has rebooted. It then resets with
code that does not read the ROM, so it no longer matters which image the bus serves at that
moment.

## Tasks

- [ ] The design, agreed with Diego and recorded as a new `D-NN`, which replaces the ROM-stub rule
  in `CLAUDE.md`. Proposed:
  - wait at least 0.5 s from RAM, counted in frames as the screenshot build already does
    (`platform_frames()`);
  - then the aligned `reset` / `jmp` pair from RAM, with the `jmp` in the same longword so the
    68000 has prefetched it before the overlay comes back;
  - jump to `$F80002`, not to address 2, so the overlay's state doesn't matter.

  Check this idiom against the Amiga Hardware Reference Manual and exec's `ColdReboot()` before
  relying on it.
- [ ] The wait covers C-07's worst case: up to about 410 ms of the old image, then 26 to 29 ms
  with nothing on the bus. The margin is chosen and written down.
- [ ] `_platform_hard_reset` leaves ROM `.boot` (`src/amiga/startup.s:74-80`), or stays only as a
  fallback. `rom_abs.ld.S` and `mem.h` change with it.
- [ ] The red screen (`COLOR00` `0x0F00`) stays during the wait, so the bench can see it.
- [ ] Harness: FS-UAE's selection session still resets into the test image. The trace shows
  the wait. Release payload sizes before and after.

## Acceptance

In FS-UAE a selection waits about the chosen time, then the A500 restarts from `$F80002`. On
hardware: STORY-02.

## Notes

- C-07, from the firmware session on 2026-10-09: after `SELECT_ROM` the old image stays for
  about 55 ms, up to about 410 ms. Then the bus is undriven for 26 to 29 ms, then the new image
  appears.
- The ST's reset (`src/st/platform.c`) waits about 2.9 s at 8 MHz and clears the memvalid
  magics. The Amiga needs no memvalid step: Kickstart cold-boots after `reset`.
