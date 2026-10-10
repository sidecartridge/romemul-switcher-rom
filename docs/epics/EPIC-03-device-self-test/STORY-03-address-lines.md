---
id: STORY-03
epic: EPIC-03
title: The address-line test
status: done
---

## Goal

For every address line the image spans, the ROM reads two words whose offsets differ only in
that line and checks each against a signature the build placed there, so a line that is stuck
or shorted is named.

## Tasks

- [x] The layout: for each line An, a pair of offsets `P` and `P ^ (1 << n)`, both inside the
      image and both in the random fill, never in the payload, the checksum fields, the Amiga's
      kickety split or its footer. Constraints to resolve and record here: the ST window is
      192 KB (`0x30000`), not a power of two, so A16 and A17 cannot share one reference
      offset; release payloads end near 16 KB but test payloads near 150 KB, which overlaps
      the low partners of A17 on the ST; the Amiga's kickety split at `0x40000` is fixed
      content the A18 test can use
- [x] `scripts/finalize_rom.py` writes a distinct signature word at every offset of the layout,
      before the checksum, and refuses to build when an offset falls in the payload
- [x] The ROM has the same layout and signatures as constants, compiled into the code it copies
      to RAM, and reports per line `ok`, `stuck` (the partner read returns the reference
      word) or `wrong` (any other value); `@@ selftest addr A<n> ok|stuck|wrong` in debug builds
- [x] Harness: every line `ok` in Hatari (ST, STE) and FS-UAE (Amiga), which serve our image as
      it is; and a negative run on a copy with one signature changed, which must name that line
- [x] Release payload sizes before and after

## Acceptance

All lines `ok` on all three images in the emulators; the negative run names the right line.
On hardware: STORY-04.

## Notes

**Done 2026-10-09.** The layout (`scripts/finalize_rom.py`, `address_layout()`, mirrored in
`selftestAddressLayout()`): for line An, reference `size - 0x100` and partner `reference ^ (1 <<
n)`; signatures `0x5AA5` (reference) and `0xA500 | n` (partner). One exception: on the ST's
192 KB window the A16 partner would leave the image, so A16 uses a second reference at
`(size - 0x200) & 0xFFFF` = `0xFE00` (signature `0x5A5A`), partner `0x1FE00`. Offsets: ST
`0xFE00` to `0x2FF80`, STE `0x1FF00` to `0x3FF80`, Amiga `0x3FF00` to `0x7FF80`, clear of the
Amiga's kickety split.

- The test payloads had to shrink first: the fake flash dropped its 64 KB storage array, which
  nothing read, and its catalog went from 61,440 bytes to the 33 records it holds plus the zero
  record that ends the chooser's walk (8,704 bytes). Debug-test payloads went from about 155 KB
  to 38 KB; the harness still reads 33 entries, default 17, rescue 0, protocol 0x40.
- The finalizer writes the signatures before the checksum and refuses an offset in the payload,
  a checksum field, the kickety split, or claimed twice (seen: a payload ending at `0x10000` on
  the ST is refused, "address signature of A16 at 0xfe00 is inside the payload").
- The ROM reads each pair: `ok`, `stuck` (one read returns the other's signature) or `wrong`;
  `@@ selftest addr A<n> ...`; the screen shows "Address lines ok A1-A17", or "FAIL" and the
  failing lines.
- `--stuck-line N` (both harnesses) writes the reference's signature into line N's partner and
  recomputes the checksums: ST A16, STE A3 and Amiga A18 each reported `stuck` with every other
  line `ok`; the three runs are in `all_harness.sh`. All 9 earlier runs: every line `ok`.
- Release payloads: `st`/`ste` 17,596, `amiga` 17,896 bytes, after STORY-02 and STORY-03 (from
  15,932 and 16,228 after STORY-01).
