---
id: STORY-14
epic: EPIC-03
title: The resolution set in the vertical blank
status: done
---

## Goal

On an ST or STE, the screen starts in place after a cold boot, in mono and in colour. Diego,
2026-10-10, with two photos of the 256 KB image on a Mega ST in mono: "Look at the left column,
we have again the problem found in the high res initialization." Every line's last 4 bytes
showed at its start: "tore" of "restore", "US 0" of "ACCELERATED_BUS 0", the list's A and R
flags. The text itself was drawn right; the display was wrapped by 32 pixels.

## Tasks

- [x] Find the cause: `screen_init()` wrote the resolution register right after the startup's
      `reset`, at whatever point of the frame it reached. EmuTOS (`bios/screen.c`,
      `screen_init_mode()`) documents both effects: changed while a line displays, "the plane
      shift bug may appear", and after a Glue reset, without a second wait for VSYNC, "a
      monochrome screen display may wrap"
- [x] Set the base, wait for two vertical blanks, then set the resolution. With interrupts
      masked the wait polls the video counter: it leaves the base when a line displays and holds
      at the base from the vertical sync to the first line. Four equal reads in a row, since a
      torn read can fake the base once; each wait is bounded, so a counter that never moves
      cannot hang the ROM
- [x] Read the monitor line (GPIP bit 7) after the waits, not right after the reset
- [x] On an STE shifter, found as EmuTOS finds it (the base's low byte holds a value written to
      it), clear the low byte, the line offset and the fine scroll, which a program may have
      left set before a warm reset
- [x] `@@ screen vbl ok ste=0|1`; every Hatari session requires it, with `ste=1` on the STE and
      Mega STE only

## Acceptance

Every Hatari session finds the vertical blank, colour and mono, on all four machines. Hatari
does not show the wrap itself, so only the bench can confirm the fix: STORY-15.

## Notes

**Done 2026-10-10.**

- The first version asked for 64 equal reads and timed out in every Hatari session: a poll of a
  debug build takes about 80 us there, and the blank lasts about 2 to 4 ms.
- The machine line of the photo said "Atari Mega ST" for the 256 KB image, which runs at
  `0xE00000`: a Mega ST with a TOS 2.06 adapter, presumably. If it is a Mega STE, the machine
  probe is wrong.
- This also covers the candidate "ST/STE cold boot on a monochrome monitor" of
  `ITERATIONS.md`, Diego's report of 2026-10-09, which he now calls the same problem.
