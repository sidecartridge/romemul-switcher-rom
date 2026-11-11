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

- [ ] An exit screen on hardware: power the board in Rescue Mode with an empty or freshly
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
