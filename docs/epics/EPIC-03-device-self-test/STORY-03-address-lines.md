---
id: STORY-03
epic: EPIC-03
title: The address-line test
status: todo
---

## Goal

For every address line the image spans, the ROM reads two words whose offsets differ only in
that line and checks each against a signature the build placed there, so a line that is stuck
or shorted is named.

## Tasks

- [ ] The layout: for each line An, a pair of offsets `P` and `P ^ (1 << n)`, both inside the
      image and both in the random fill, never in the payload, the checksum fields, the Amiga's
      kickety split or its footer. Constraints to resolve and record here: the ST window is
      192 KB (`0x30000`), not a power of two, so A16 and A17 cannot share one reference
      offset; release payloads end near 16 KB but test payloads near 150 KB, which overlaps
      the low partners of A17 on the ST; the Amiga's kickety split at `0x40000` is fixed
      content the A18 test can use
- [ ] `scripts/finalize_rom.py` writes a distinct signature word at every offset of the layout,
      before the checksum, and refuses to build when an offset falls in the payload
- [ ] The ROM has the same layout and signatures as constants, compiled into the code it copies
      to RAM, and reports per line `ok`, `stuck` (the partner read returns the reference
      word) or `wrong` (any other value); `@@ selftest addr A<n> ok|stuck|wrong` in debug builds
- [ ] Harness: every line `ok` in Hatari (ST, STE) and FS-UAE (Amiga), which serve our image as
      it is; and a negative run on a copy with one signature changed, which must name that line
- [ ] Release payload sizes before and after

## Acceptance

All lines `ok` on all three images in the emulators; the negative run names the right line.
On hardware: STORY-04.

## Notes
