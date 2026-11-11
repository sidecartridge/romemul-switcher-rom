---
id: STORY-05
epic: EPIC-04
title: Verification (Diego's checkpoint)
status: in-progress
---

## Goal

Diego sees the four fixes on real hardware, and the release images still work as v4.0.0's
did.

## Tasks

- [x] An error screen on hardware: power the board in Rescue Mode with an empty or freshly
  formatted flash, or force a read error. The message says what a key does, and a key does
  it. On an ST and an Amiga (Diego). Closed without hardware: Diego cannot reproduce one on
  the bench (2026-11-11), so the release image in Hatari stands for it (STORY-01).
- [x] Quick keys on an ST or Mega ST: no key lost in the list, the confirmation and the
  self-test (Diego)
- [ ] The list, the self-test, the soak and a selection that boots: the same as v4.0.0, on both
  boards, on an ST, an STE and an Amiga (Diego)
- [x] The release images' build ID and checksum line right (Diego)

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
- 2026-11-11, Diego, for both images on the Mega ST: "all ok, both boards, no error screen
  tested". The self-test, the soak, a selection and quick keys all OK, on the RP2350A and the
  RP2350B. Quick keys checked.
  - Still open in the third task: the Amiga, and a real STE or Mega STE (so far the 256 KB
    image ran only on the Mega ST, through its 256 KB decoder).
  - The error screen was not tested on hardware. On the bench it can hardly be reached: the
    rescue image is always in the catalog, and a read error cannot be forced. The release
    image in Hatari showed it (STORY-01). Diego to decide whether that is enough.
- 2026-11-11, Diego: "Mega ST with decoder is equivalent to ST and Mega STE." So the 256 KB
  image on the Mega ST counts for the STE, and the Atari part of the third task is done. Only
  the Amiga is left there. Also: "Build ID is correct" (`f5cb491`; neither image stopped at a
  checksum prompt), and "I can't reproduce errror screen on hardware". The first task is closed
  on the Hatari result.

