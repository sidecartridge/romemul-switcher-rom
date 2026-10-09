---
id: STORY-04
epic: EPIC-03
title: The data-line test
status: todo
---

## Goal

The ROM reads walking-one and walking-zero words the build placed in the image, as words and as
bytes, and names any data line D0 to D15 that is stuck or shorted, and any byte lane (UDS, LDS)
that fails.

## Tasks

- [ ] A 64-byte block in the random fill, clear of every offset of STORY-03's layout, holding the
      16 words `1 << k` and the 16 words `~(1 << k)`; written by `scripts/finalize_rom.py`
      before the checksum
- [ ] The ROM reads each word, then each of its two bytes, and reports `D0-D15 ok` or the lines
      stuck at 0, stuck at 1 or shorted, and the failing lane; `@@ selftest data ...` lines
- [ ] Harness: `ok` on all three images; a negative run with one pattern word changed names the line
- [ ] Release payload sizes before and after

## Acceptance

`ok` on all three images in the emulators; the negative run names the right line.

## Notes

The 68000 has no A0: a word read drives both byte lanes, a byte read only one (UDS for the
even byte, LDS for the odd one), so a lane fault shows only in the byte reads.
