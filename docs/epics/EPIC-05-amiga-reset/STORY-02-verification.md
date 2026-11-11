---
id: STORY-02
epic: EPIC-05
title: Verification (Diego's checkpoint)
status: done
---

## Goal

Diego sees selections boot on the Amiga, on both boards, every time.

## Tasks

- [x] RP2350A, A500 or A2000: ten selections in a row, each boots the chosen ROM (Diego)
- [x] RP2350B, A500 or A2000: the same (Diego)
- [x] RP2350B, A600: the same (Diego)
- [x] The red screen shows during the wait, and the wait is not long enough to bother (Diego)

## Acceptance

All four checked by Diego.

## Notes

The A600 runs the 512 KB image here. The firmware's mirror fix serves it on both halves.

**Done 2026-11-11**, the Amiga image of `f5cb491` (`ee6ce3ce…`). Diego: "i can confirm all the new images work in amiga, st 192kb and ste 256kb", and, asked which checks that covers, confirmed all four: the list, the self-test, the soak and ten selections in a row on an A500 or A2000 on both boards; ten selections on the A600 with the RP2350B; the red screen before each reset, a wait that does not bother; and the bus chip line, "Gayle (ID 0xD0)" on the A600 and "Gary (no Gayle ID)" on the A500 or A2000 on both boards, with no hang.
