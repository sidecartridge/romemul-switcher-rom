---
id: STORY-01
epic: EPIC-04
title: Every exit screen leads somewhere
status: todo
---

## Goal

After "Press any key to exit..." and every other way out of `chooser_loop()`, the machine does
something defined and useful. Today it falls into `rts` from a function that was entered with
`jmp`.

## Tasks

- [ ] List every way `chooser_loop()` can return (`src/common/chooser.c`):
  - the two out-of-memory messages, which return without waiting for a key;
  - the parameters page read error;
  - the catalog page read error;
  - the protocol mismatch;
  - "No ROMs found".
- [ ] Diego chooses what a key does on these screens: start again from the boot ping, or a cold
  reset into the rescue ROM, which the firmware still serves. Recommended: start again. A board
  that answered late, or a page that failed once, gets a second chance without a power cycle,
  and a cold reset adds nothing a power cycle doesn't. Record the choice here.
- [ ] `rom_switcher_main()` never returns on either platform (`src/st/switcher.c`,
  `src/amiga/switcher.c`). Starting again resets the `palloc` heap first, so the 61,440-byte
  catalog buffer is not allocated twice.
- [ ] Every one of these screens says what the key does: "Press any key to try again...", not
  "exit".
- [ ] Harness: a debug-test session for each path the test data can reach, with a trace line per
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
