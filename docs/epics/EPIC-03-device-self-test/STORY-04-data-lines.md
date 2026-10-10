---
id: STORY-04
epic: EPIC-03
title: The data-line test
status: done
---

## Goal

The ROM reads walking-one and walking-zero words the build placed in the image, as words and as
bytes, and names any data line D0 to D15 that is stuck or shorted, and any byte lane (UDS, LDS)
that fails.

## Tasks

- [x] A 64-byte block in the random fill, clear of every offset of STORY-03's layout, holding the
      16 words `1 << k` and the 16 words `~(1 << k)`; written by `scripts/finalize_rom.py`
      before the checksum
- [x] The ROM reads each word, then each of its two bytes, and reports `D0-D15 ok` or the lines
      stuck at 0, stuck at 1 or shorted, and the failing lane; `@@ selftest data ...` lines
- [x] Harness: `ok` on all three images; a negative run with one pattern word changed names the line
- [x] Release payload sizes before and after

## Acceptance

`ok` on all three images in the emulators; the negative run names the right line.

## Notes

The 68000 has no A0: a word read drives both byte lanes, a byte read only one (UDS for the
even byte, LDS for the odd one), so a lane fault shows only in the byte reads.

**Done 2026-10-09.** The block sits at `size - 0x1000` (ST `0x2F000`, STE `0x3F000`, Amiga
`0x7F000`): the 16 words `1 << k`, then the 16 words `~(1 << k)` (`data_patterns()` in
`scripts/finalize_rom.py`, written with the address signatures by `write_selftest_patterns()`,
which checks every offset against the others, the payload and the reserved fields). The ROM
reads each word, then its two bytes: a walking-one bit read as 0 is a line stuck at 0, a
walking-zero bit read as 1 a line stuck at 1, any other bit set in a walking-one word a short,
and bytes that disagree with their word a failing lane. `@@ selftest data ok`, or the four
masks; the screen shows "Data lines ok D0-D15, both byte lanes" or the failing lines.

- `--stuck-data K` (both harnesses) clears bit K in every pattern word and recomputes the
  checksums: D9 on the ST and D0 on the Amiga each reported as stuck at 0 and nothing else;
  both runs are in `all_harness.sh`, which passes 14 of 14.
- Release payloads: `st` 18,276, `amiga` 18,576 bytes (from 17,596 and 17,896 after STORY-03).
