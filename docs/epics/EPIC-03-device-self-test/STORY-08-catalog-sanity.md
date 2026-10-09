---
id: STORY-08
epic: EPIC-03
title: Catalog and parameters sanity
status: todo
---

## Goal

The self-test checks what the chooser loaded and shows the configuration stored in the device,
as the host SWITCHER's configuration page does.

## Tasks

- [ ] Checks: protocol 0x40; 1 to 64 entries; the default and rescue indices inside the catalog;
      every name non-empty and printable; every size non-zero and no larger than the largest
      image the firmware serves
- [ ] The `CONFIG.TXT` values from the parameters page, as sidecartos-config's
      `show_config_page()` decodes them: `READ_BUS_TICKS` (byte 516), `USE_OE` (520),
      `CE_SYNC_BYPASS`, `WRITE_ON_DRIVE`, `DMA_HIGH_PRIORITY` (bus flags at 524, bits 0x1, 0x2,
      0x4), `RESCUE_TIMEOUT` (528, above 600 s shown as 0), `LARGE_ROMS`, `ACCELERATED_BUS`
      (ROM flags at 532, bits 0x1, 0x2); an erased flags word reads as no flags
- [ ] `@@ selftest catalog ...` and `@@ selftest config ...` lines; harness checks them against
      `src/common/test.c`, whose fake parameters page gains non-default values for this

## Acceptance

`ok` with the expected values on all three images in the emulators.

## Notes

Offsets and bits checked against sidecartos-config `src/main.c` (`show_config_page()`) and
`src/include/helper.h` on 2026-10-09; part of the shared contract (D-03).
