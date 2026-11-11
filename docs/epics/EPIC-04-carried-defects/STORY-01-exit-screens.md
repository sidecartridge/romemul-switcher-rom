---
id: STORY-01
epic: EPIC-04
title: Every exit screen leads somewhere
status: in-progress
---

## Goal

After "Press any key to exit..." and every other way out of `chooser_loop()`, the machine does
something defined and useful. Today it falls into `rts` from a function that was entered with
`jmp`.

## Tasks

- [x] List every way `chooser_loop()` can return (`src/common/chooser.c`):
  - the two out-of-memory messages, which return without waiting for a key;
  - the parameters page read error;
  - the catalog page read error;
  - the protocol mismatch;
  - "No ROMs found".
- [ ] Diego chooses what a key does on these screens: start again from the boot ping, or a cold
  reset into the rescue ROM, which the firmware still serves. Recommended: start again. A board
  that answered late, or a page that failed once, gets a second chance without a power cycle,
  and a cold reset adds nothing a power cycle doesn't. Record the choice here.
- [x] `rom_switcher_main()` never returns on either platform (`src/st/switcher.c`,
  `src/amiga/switcher.c`). Starting again resets the `palloc` heap first, so the 61,440-byte
  catalog buffer is not allocated twice.
- [x] Every one of these screens says what the key does: "Press any key to try again...", not
  "exit".
- [x] Harness: a debug-test session for each path the test data can reach, with a trace line per
  path. Release payload sizes before and after.

## Acceptance

In the emulators, each path the test build can reach shows its message, and a key starts again
from the boot ping. The release image in Hatari (which has no SidecarTridge, C-03) shows the
flash read error and comes back to it after a key, instead of hanging. On hardware: STORY-05.

## Notes

- From the candidate added by Diego on 2026-10-11 (`ITERATIONS.md`, iteration 2). The ST/STE
  startup enters `rom_switcher_main` with `jmp` (`src/st/startup_st.s:119`), and so does the
  Amiga startup (`src/amiga/startup.s:71`). On the ST and STE the `rts` pops the first long of
  screen memory, because `ST_STACK_TOP_ADDR_UL` is the screen base. On the Amiga it pops the
  long at `0x80000`.
- The v4.0.0 user docs tell users to switch off after any exit message. Tell the firmware
  session when this changes, so the docs can follow.

**Built and run 2026-11-11**, on `epic-04-carried-defects`, uncommitted. Open: Diego's choice.

- **The choice.** Built as the recommendation, start again, because Diego said "ok, start"
  without choosing. The task stays open until he confirms it, or picks the cold reset.
- **The loop.** `rom_switcher_main()` loops forever on both platforms. Each pass calls
  `pa_init()`, clears the screen, runs the checksum screen and `chooser_loop()`, then traces
  `@@ restart`. Only the chooser and the self-test allocate, so resetting the heap is enough.
- **The screens.** All six return paths in `chooser_loop()` end in `wait_to_try_again()`:
  "Press any key to try again...", `@@ waitkey`, then a key press. The two out-of-memory paths
  now wait too. Read errors trace `@@ flash read error <address>`, and "Exiting." is gone from
  their message.
- **The harness.** `--exit mismatch|empty|erased` on both harnesses (`exit_copy()` and
  `run_exit_session()` in `tools/dev/harness_common.py`). Each patches the test image's built-in
  data and recomputes the checksums:
  - `mismatch`: the protocol word one higher;
  - `empty` and `erased`: record 0 starting with `0x00` or `0xFF`.

  The session then expects the screen, a key, `@@ restart`, the checksum again and the same
  screen again. Six sessions were added to `all_harness.sh` (ST mismatch, STE empty, ST erased,
  and the three on the Amiga). The run passed 31 of 31 with 0 key retries.
- **The release image in Hatari** (ST, no cartridge), by screenshots: the boot ping message,
  then a key, then "Error reading the flash page at address 00fff000." and "Press any key to
  try again...". A key brings the checksum screen and the same error again, twice in a row. The
  machine never hangs.
- **Release payload.** ST/STE 25,672 → 25,788 bytes, Amiga 25,832 → 25,944, measured from the
  first fill word. v4.0.0 is the `561b13d` build.
- The test build cannot reach the read-error and out-of-memory paths: its data is built in.
  The release image covers the read error.
