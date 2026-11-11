---
id: STORY-01
epic: EPIC-05
title: A reset that does not depend on the old image
status: done
---

## Goal

`platform_hard_reset()` on the Amiga waits until the firmware has rebooted. It then resets with
code that does not read the ROM, so it no longer matters which image the bus serves at that
moment.

## Tasks

- [x] The design, agreed with Diego and recorded as a new `D-NN`, which replaces the ROM-stub rule
  in `CLAUDE.md`. Proposed:
  - wait at least 0.5 s from RAM, counted in frames as the screenshot build already does
    (`platform_frames()`);
  - then the aligned `reset` / `jmp` pair from RAM, with the `jmp` in the same longword so the
    68000 has prefetched it before the overlay comes back;
  - jump to `$F80002`, not to address 2, so the overlay's state doesn't matter.

  Check this idiom against the Amiga Hardware Reference Manual and exec's `ColdReboot()` before
  relying on it.
- [x] The wait covers C-07's worst case: up to about 410 ms of the old image, then 26 to 29 ms
  with nothing on the bus. The margin is chosen and written down.
- [x] `_platform_hard_reset` leaves ROM `.boot` (`src/amiga/startup.s:74-80`), or stays only as a
  fallback. `rom_abs.ld.S` and `mem.h` change with it.
- [x] The red screen (`COLOR00` `0x0F00`) stays during the wait, so the bench can see it.
- [x] Harness: FS-UAE's selection session still resets into the test image. The trace shows
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

**Done 2026-11-11**, on `epic-05-amiga-reset` (cut from `epic-04-carried-defects`, whose code
is still uncommitted in the same working tree), uncommitted. Diego: "go with epic 05".

- **The design.** Recorded as D-10. `CLAUDE.md`'s ROM-stub rule is replaced in its "Amiga
  reset" gotcha, the selection paragraph and the file map.
- **The idiom, checked against Commodore's own documentation.** The Hardware Reference Manual
  (Amiga Developer CD v2.1, `Hardware_Manual_guide/node02E3`) and Amiga Mail's 1988 reboot
  note (`jf90.lha/reboot.ascii`) both use `lea 2,a0` / `RESET` / `jmp (a0)`, longword-aligned,
  "Rely on prefetch to execute this instruction", with the comment "all RAM goes away now!" on
  `RESET`. `reset.s` follows that note. It jumps through address 2, as the note does, not
  through `$F80002` as first proposed here: after the reset the overlay maps the ROM at 0
  whatever its size, so a 256 KB Kickstart mirrored at `$FC0000` works too.
- **The wait.** One second of frames, 50 on PAL and 60 on NTSC (`kResetWaitSeconds`). That
  is more than twice C-07's worst case of about 440 ms. The ST waits about 2.9 s.
- **Out of ROM.** `src/amiga/reset.s` is listed in `SRCS_S_RAM` and links into `.ramtext` with
  the C objects. `_platform_hard_reset` left `startup.s`. `rom_abs.ld.S` and `mem.h` needed no
  change. The reset pair lands at `0x3E010`, and `scripts/finalize_rom.py` now refuses an image
  where `_amiga_cold_reset_pair` is not longword-aligned.
- **Red screen.** `COLOR00` turns red before the wait, as before. It is in the code but has not
  been seen in an emulator, because one second is hard to catch. The bench sees it in STORY-02.
- **A frame counter bug, found and fixed.** The first FS-UAE run at real speed (`--no-warp`)
  measured the wait at 0.6 s. `platform_frames()` read bit 8 of the beam line (`VPOSR`) and
  its low byte (`VHPOSR`) separately. A tight loop then reads line 255 to 256 as line 0, an
  extra frame each frame. It now reads bit 8 again and retries. The next run measured 1.0 s
  (`reset` at 47.0 s, `reset now` at 48.0 s, the image back at 48.2 s). The Amiga soak, which
  polls less often, was unaffected: 30 s and 179,484 reads, against 178,663 before.
- **The harness.** `@@ reset wait <n> frames` and `@@ reset now` are traced, and the FS-UAE
  sessions now require them between `@@ reset rom 1` and the image coming back.
  `all_harness.sh` passed 31 of 31 with 0 key retries.
- **Release payload.** Amiga 25,988 → 26,080 bytes (EPIC-04's 25,988 as the baseline).
