---
id: STORY-01
epic: EPIC-06
title: The 1 MB image
status: todo
---

## Goal

The Amiga build also produces a 1 MB image made of two complete rescue images. The `$E00000`
half comes first, and the two halves differ only in a marker word.

## Tasks

- [ ] `scripts/finalize_rom.py` finalizes the same payload twice. Each half gets:
  - its marker word, at an offset in the free space clear of the self-test's words;
  - the fill pattern;
  - the address signatures and the data block;
  - our checksum at `0x7FFE4`;
  - its own `romtool` footer.

  The 1 MB file is the low-marker half, then the high-marker half. `romtool info` passes on each
  half.
- [ ] Each half boots alone. That is what the RP2350A serves, and what the firmware serves when
  it cannot stage the upper half. `rom_check.c` still sums 512 KB at `$F80000`, whichever half
  is there.
- [ ] `tools/dev/build.sh amiga <type>` and `./build.sh amiga` write the 1 MB image next to, or
  instead of, the 512 KB one, as decided in the epic's notes.
- [ ] The marker's offset and values are recorded here and in `CLAUDE.md`'s finalisation section.
- [ ] Reproducibility: a clean build of a commit gives the same 1 MB bytes locally and in CI.

## Acceptance

The 1 MB image's halves compare equal except for the marker word, both checksum words and both
footers. `romtool info` says `ok` on each half, and FS-UAE boots each half as a 512 KB
Kickstart.

## Notes

- The firmware accepted the name `RESCUE_SWITCHER_v<ver>_1024KB.img` (2026-11-11). It feeds
  `make_images.py` and `DEFAULT_ROM/MANIFEST.txt` (D-03).
