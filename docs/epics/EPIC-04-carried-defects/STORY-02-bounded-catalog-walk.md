---
id: STORY-02
epic: EPIC-04
title: A bounded catalog walk
status: todo
---

## Goal

`parse_rom_description()` stops at the end of its buffer, and at erased flash, as the host
SWITCHER already does.

## Tasks

- [ ] The walk (`src/common/chooser.c`, `parse_rom_description()`) stops at
  `MEMORY_EXCHANGE_SIZE / 256` = 240 records, and at a record whose first byte is `0x00` or
  `0xFF`. This mirrors sidecartos-config `448fd03`.
- [ ] The self-test's catalog check (`src/common/selftest.c`, EPIC-03 STORY-08) walks with the
  same bound. If it has its own walk, it uses the same rule.
- [ ] Harness: a session on a debug-test image whose catalog record 0 starts with `0xFF` shows
  "No ROMs found" (now followed by STORY-01's path). A full catalog of 240 records shows 240
  entries. Release payload sizes before and after.

## Acceptance

An erased catalog (all `0xFF`) gives "No ROMs found" in the emulators, not a walk past the
buffer. The usual test catalog still lists its 33 entries.

## Notes

- From iteration 1's candidates (2026-10-09). The firmware zero-fills the catalog after its last
  record and caps it at 64 (`ROM_MAX_FILES`). Only a flash that was never indexed is all `0xFF`.
  The bound costs a compare per record.
