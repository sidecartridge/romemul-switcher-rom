---
id: STORY-02
epic: EPIC-04
title: A bounded catalog walk
status: done
---

## Goal

`parse_rom_description()` stops at the end of its buffer, and at erased flash, as the host
SWITCHER already does.

## Tasks

- [x] The walk (`src/common/chooser.c`, `parse_rom_description()`) stops at
  `MEMORY_EXCHANGE_SIZE / 256` = 240 records, and at a record whose first byte is `0x00` or
  `0xFF`. This mirrors sidecartos-config `448fd03`.
- [x] The self-test's catalog check (`src/common/selftest.c`, EPIC-03 STORY-08) walks with the
  same bound. If it has its own walk, it uses the same rule.
- [x] Harness: a session on a debug-test image whose catalog record 0 starts with `0xFF` shows
  "No ROMs found" (now followed by STORY-01's path). A full catalog of 240 records shows 240
  entries. Release payload sizes before and after.

## Acceptance

An erased catalog (all `0xFF`) gives "No ROMs found" in the emulators, not a walk past the
buffer. The usual test catalog still lists its 33 entries.

## Notes

- From iteration 1's candidates (2026-10-09). The firmware zero-fills the catalog after its last
  record and caps it at 64 (`ROM_MAX_FILES`). Only a flash that was never indexed is all `0xFF`.
  The bound costs a compare per record.

**Done 2026-11-11**, on `epic-04-carried-defects`, uncommitted.

- **The walk.** It stops at `max_records` = `MEMORY_EXCHANGE_SIZE / kRomDescriptionSize` (240),
  or at a first byte of `0x00` or `kErasedFlashByte` (`0xFF`).
- **The self-test.** It has no walk of its own. `selftestCatalog()` loops over the chooser's
  `entries`, capped at its own `kCatalogMaxEntries`, so it follows the new bound.
- **The harness.** `--exit erased` (record 0 starting with `0xFF`) gives `@@ list 0 entries`,
  then "No ROMs found", in Hatari (ST) and FS-UAE. With the old walk, record 1 would have
  followed and the list would have shown 33 entries. Every other session still lists 33.
- **The 240-record case cannot run in a test build.** The built-in catalog is kept to 33
  records so that the ST test image stays under its 63 KB limit (EPIC-03 STORY-03). The bound
  is checked by reading the code. A device with 240 records cannot exist either: the firmware
  caps the catalog at 64.
- **Release payload, with STORY-03.** ST/STE 25,788 → 25,840 bytes, Amiga 25,944 → 25,988.
