---
id: STORY-03
epic: EPIC-06
title: The A19 line in the self-test
status: blocked
---

## Goal

The self-test's address line adds A19 on the Amiga:
- `ok` when the two halves read their own markers;
- `stuck` when the slot is staged but both addresses read the same marker;
- `skipped` when the firmware says the slot does not hold the rescue's upper half.

## Tasks

- [ ] Read the firmware's "the slot holds the rescue image's half" from where the owner record
  lives: a parameters-page word, or a sector after the slot. The offset and values come from the
  firmware session with its commit hash. Mirror them in `src/common/selftest.c` and in
  `src/common/test.c`'s parameters page (D-03).
- [ ] After A1 to A18 (`selftestAddress()`), read the marker at `$E00000` and at `$F80000`:
  - the low marker, then the high marker: `ok`;
  - the record says staged and both reads match: `stuck` (low or high);
  - the record says not staged: `skipped`;
  - anything else: `wrong`.

  `rom_size` stays 512 KB, so A1 to A18, `rom_check` and the stress reads are unchanged.
- [ ] The screen says "A1-A19" when A19 passes, and "A1-A18, A19 skipped" otherwise, within 80
  columns (EPIC-03 STORY-12).
- [ ] `@@ selftest addr A19 ok|stuck|skipped|wrong` in debug builds, and the harness reads it.
- [ ] A stuck A19 in the harness (`--stuck-line 19` writes the low marker into the high half)
  reports `stuck`.

## Acceptance

In FS-UAE (STORY-04):
- the 1 MB image on a machine that maps both halves says A19 `ok`;
- with the high marker overwritten, it says `stuck`;
- the 512 KB image says `skipped`.

## Notes

Blocked until the firmware fixes where the owner record lives (firmware EPIC-03 STORY-01, D-13).
Its first proposal, bit 0x4 of `romflags` at byte 532, is on hold.

The v4.0.0 self-test tests only bits 0x1 and 0x2 of `romflags` (`src/common/selftest.c:864-893`),
so a new bit or word there does not disturb it.
