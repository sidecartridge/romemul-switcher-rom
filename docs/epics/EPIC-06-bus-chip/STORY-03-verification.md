---
id: STORY-03
epic: EPIC-06
title: Verification (Diego's checkpoint)
status: done
---

## Goal

Diego sees the right bus chip on each Amiga on the bench.

## Tasks

- [x] RP2350B, A600: the self-test says "Gayle (ID 0xD0)", and the rest of it is as before
  (Diego)
- [x] RP2350B, A500 or A2000: it says "Gary (no Gayle ID)" (Diego)
- [x] RP2350A, A500 or A2000: the same (Diego)
- [x] A selection still boots on each (Diego)

## Acceptance

All four checked by Diego.

## Notes

The A600 needs the re-issued firmware v4.0.0 with the 512 KB mirror fix (firmware `3a1bf82`) to
boot the rescue at all.

**Done 2026-11-11**, the Amiga image of `f5cb491` (`ee6ce3ce…`). Diego: "i can confirm all the new images work in amiga, st 192kb and ste 256kb", and, asked which checks that covers, confirmed all four: the list, the self-test, the soak and ten selections in a row on an A500 or A2000 on both boards; ten selections on the A600 with the RP2350B; the red screen before each reset, a wait that does not bother; and the bus chip line, "Gayle (ID 0xD0)" on the A600 and "Gary (no Gayle ID)" on the A500 or A2000 on both boards, with no hang.
