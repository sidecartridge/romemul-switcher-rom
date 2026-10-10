---
id: STORY-07
epic: EPIC-03
title: Flash read consistency
status: done
---

## Goal

When the device answers, the self-test reads the parameters page and the catalog several times
through the bus and reports whether every read agrees and how many block retries they needed.

## Tasks

- [x] A retry counter in `commands.c` for `read_flash_block()`, readable by the self-test
- [x] The parameters page and the catalog pages read K times (K decided here), compared byte for
      byte with the first read; differences and retries reported
- [x] Skipped when the ping got no answer; `@@ selftest flash` line; harness: "skipped" in the
      emulators

## Acceptance

In the emulators: skipped cleanly. On the bench: identical reads, the retry count (STORY-11).

## Notes

**Done 2026-10-09.** `command_block_retries()` in `commands.c` counts every block read repeated
after a failed command or a checksum mismatch. The self-test re-reads the parameters page and
the catalog pages that hold entries (one page more than the records need: 3 for the 33 test
entries) K = 3 times through a 4 KB page allocated once, compares each with the chooser's
buffers, and reports bytes that differ, failed reads and the retries; skipped when the device
does not answer. `FLASH_CATALOG_START` and `FLASH_PARAMS_START` moved from `chooser.c` to
`commands.h`, one definition for both users. Emulators: skipped, 14 of 14 PASS.

Found here: release builds had not compiled since STORY-05. The ping test's names table is
used by trace lines only, which release builds compile out, so `-Werror` stopped on an unused
variable, and `all_harness.sh` built only the debug+test images. Fixed, and `all_harness.sh`
now builds the three release images too. Release payloads now: `st`/`ste` 19,828, `amiga`
20,128 bytes.
