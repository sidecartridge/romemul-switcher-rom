---
id: STORY-08
epic: EPIC-03
title: Catalog and parameters sanity
status: done
---

## Goal

The self-test checks what the chooser loaded and shows the configuration stored in the device,
as the host SWITCHER's configuration page does.

## Tasks

- [x] Checks: protocol 0x40; 1 to 64 entries; the default and rescue indices inside the catalog;
      every name non-empty and printable; every size non-zero and no larger than the largest
      image the firmware serves
- [x] The `CONFIG.TXT` values from the parameters page, as sidecartos-config's
      `show_config_page()` decodes them: `READ_BUS_TICKS` (byte 516), `USE_OE` (520),
      `CE_SYNC_BYPASS`, `WRITE_ON_DRIVE`, `DMA_HIGH_PRIORITY` (bus flags at 524, bits 0x1, 0x2,
      0x4), `RESCUE_TIMEOUT` (528, above 600 s shown as 0), `LARGE_ROMS`, `ACCELERATED_BUS`
      (ROM flags at 532, bits 0x1, 0x2); an erased flags word reads as no flags
- [x] `@@ selftest catalog ...` and `@@ selftest config ...` lines; harness checks them against
      `src/common/test.c`, whose fake parameters page gains non-default values for this

## Acceptance

`ok` with the expected values on all three images in the emulators.

## Notes

Offsets and bits checked against sidecartos-config `src/main.c` (`show_config_page()`) and
`src/include/helper.h` on 2026-10-09; part of the shared contract (D-03).

**Done 2026-10-09.** `selftestCatalog()`: protocol 0x40, 1 to 64 entries, the default and
rescue indices inside the catalog, every name non-empty and printable, every size from 1 to
256 blocks of 4 KB (1 MB). `selftestConfig()`: the six words at 512 to 532, little-endian as the
chooser reads the page, decoded as `show_config_page()` does, on three lines that fit 80
columns. `@@ selftest catalog ok|fail entries=... badnames=... badsizes=...` and `@@ selftest
config ticks=... romflags=...`; the harness takes the expected values from `test.c`, whose
fake page now holds `READ_BUS_TICKS` 4, `USE_OE` 1, bus flags 0x5, `RESCUE_TIMEOUT` 30 and ROM
flags 0x1 (they were all 0). `all_harness.sh`: 14 of 14 PASS.

Found on the way:

- The sizes are little-endian, as the chooser's size column reads them: the first version read
  them the other way, all 33 entries failed, and each session waited out its 60 s step timeout
  (the slow run Diego asked about); stopped, fixed, back to about 6 s.
- Lines overflowed 80 columns and wrapped onto the next row; the label column went from 24 to
  20 characters (details from column 28, 52 characters), the configuration to three lines, and
  the flash and address details got shorter.
- In mono every green "ok" was invisible: the ST glyph renderer kept only bit 0 of the colour
  index, so green (2) drew black. Since before this epic that also hid "ROM checksum OK" on a
  mono screen. Mono now draws any colour but black in white (`src/st/glyph.c`); the mono
  screenshot shows every result.
- Release payloads: `st` 20,800, `amiga` 21,100 bytes.
