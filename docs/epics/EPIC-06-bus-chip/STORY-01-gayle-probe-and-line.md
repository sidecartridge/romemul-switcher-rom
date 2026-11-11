---
id: STORY-01
epic: EPIC-06
title: The Gayle check and the self-test line
status: done
---

## Goal

The Amiga self-test shows a "Bus chip" line: "Gayle (ID 0xD0)" on an A600, "Gayle (ID 0xD1)"
on an A1200, and "Gary (no Gayle ID)" otherwise.

## Tasks

- [x] `src/amiga/probe.s`: a word read and a byte write under a temporary bus-error handler
  (`amiga_probe_word`, `amiga_probe_write_byte`), the pattern of the ST's `st_probe_read`.
  Linked into RAM.
- [x] `src/amiga/platform.c`: read Gayle's ID at `$DE1000`. A write restarts the ID, then eight
  reads each give the next bit, most significant first, in bit 7 of the byte (bit 15 of the
  word), as MAME models it. Read the ID twice, and count only `0xD0` or `0xD1` found both
  times. It reaches `src/common` as `bus_chip` and `bus_chip_id` in `platform_machine_t`,
  which are unset on the ST.
- [x] `src/common/selftest.c`: the "Bus chip" line after the machine line, within 80 columns,
  and `@@ selftest buschip <Gary|Gayle> id=0x<id>` in debug builds. The Amiga's self-test
  goes from 11 lines to 12.
- [x] Release payload sizes before and after.

## Acceptance

In FS-UAE the A500 says Gary and the A600 says Gayle, ID 0xD0. On hardware: STORY-03.

## Notes

- **The ID register**, from MAME (`src/mame/amiga/gayle.cpp`, `gayle_id_r` and `gayle_id_w`;
  `src/mame/amiga/amiga.cpp`, `GAYLE_ID` `0xd0` for the A600 and `0xd1` for the A1200,
  mapped at `0xde0000`). A write resets the bit counter. Each read returns
  `((id << count++) & 0x80) << 8`.
- **Why twice.** A machine without Gayle returns whatever the bus holds. FS-UAE's A500 gave
  `0x22`, then `0xFF` once the code around the reads changed. One exact reading could match by
  chance about 2 times in 256. Two that agree make that about 1 in 32,000, even for a truly
  random bus.
- **Is the read safe on a machine without Gayle?** The firmware session believes Kickstart 3.1
  checks for Gayle on every machine, the A500 included, but neither of us has confirmed it. A
  bus error counts as "no Gayle" in any case.
- The `$E00000` comparison and the `--rom-1mb` harness option of the first attempt are gone.

**Done 2026-11-11**, code in `48be4d2` on `epic-06-bus-chip`.

- **FS-UAE.** The A500 reads `0x0ff 0x0ff` and says Gary. The A600 reads `0x0d0 0x0d0` and
  says Gayle, ID 0xD0.
- **Release payload,** clean builds: ST/STE 25,812 → 25,952 bytes, Amiga 26,048 → 26,444. The
  baseline is `127565f`, EPIC-05. The ST grows too, because the self-test's line is in
  `src/common`, even though the ST never shows it.
