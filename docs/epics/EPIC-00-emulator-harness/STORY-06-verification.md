---
id: STORY-06
epic: EPIC-00
title: Verification (Diego's checkpoint)
status: done
---

## Goal

The harness is seen to work end to end and to fail where it should, and Diego agrees that it
stands in for the machines for everything but the bus (D-06).

## Tasks

- [x] `tools/dev/all_harness.sh` passes on this Mac. Run by Claude at Diego's request
      ("can you run this on your own?", 2026-10-09): PASS, 9 of 9 runs, no key retry.
- [x] A deliberate break makes the right runs FAIL with their evidence. Run by Claude at
      Diego's request: with the down arrow broken in `chooser.c`, all nine runs FAIL at
      "cursor on entry 1", each Hatari run with its screenshot, and the script exits 1. With
      the file restored byte for byte, 9 of 9 PASS again.
- [x] Diego confirms or amends D-06 and D-07. Confirmed 2026-10-09 ("yes to all").

## Acceptance

All three tasks checked. The look by eye with `--interactive` is not part of this epic (Diego,
2026-10-09: "you can leave interactive out and add that tests before the epic of the release
gate"); it is EPIC-01.

## Notes

The failure screenshot of the STE run shows the cursor still on entry 0 after the down key, and
the debug build's own key overlay ("Key: 0x50 Index: 0", `chooser.c`, debug only) drawn over the
help line. That overlay predates this epic and is harmless in debug builds.

This is not a bench pass: nothing here touches a SidecarTridge (C-03).
