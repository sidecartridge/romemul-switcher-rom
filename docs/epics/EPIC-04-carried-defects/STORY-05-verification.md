---
id: STORY-05
epic: EPIC-04
title: Verification (Diego's checkpoint)
status: todo
---

## Goal

Diego sees the four fixes on real hardware, and the release images still work as v4.0.0's
did.

## Tasks

- [ ] An error screen on hardware: power the board in Rescue Mode with an empty or freshly
  formatted flash, or force a read error. The message says what a key does, and a key does
  it. On an ST and an Amiga (Diego).
- [ ] Quick keys on an ST or Mega ST: no key lost in the list, the confirmation and the
  self-test (Diego)
- [ ] The list, the self-test, the soak and a selection that boots: the same as v4.0.0, on both
  boards, on an ST, an STE and an Amiga (Diego)
- [ ] The release images' build ID and checksum line right (Diego)

## Acceptance

All four checked by Diego.

## Notes

Debug images are still never put on hardware, even after STORY-04.

- 2026-11-11, the images of `f5cb491` (ST `dbc7479f…`, STE `9a84e72d…`), on Diego's Mega ST,
  which has a 256 KB ROM decoder, so it runs both images. Diego: "ST and STE works. 192KB and
  256KB work."
  - The firmware session loaded them onto the RP2350B (firmware `da87d2e+debug`). The board's
    `rom-check` matched the 256 KB image, and the board counted 345,233 reads and 258 commands
    with no error. Only the start-up path was exercised there.
  - No task is checked yet. Still to confirm: the RP2350A, the STE, the self-test, the soak, a
    selection, quick keys and an error screen.

