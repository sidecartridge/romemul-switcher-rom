---
id: STORY-05
epic: EPIC-06
title: Verification (Diego's checkpoint)
status: todo
---

## Goal

Diego sees the A19 line right on every board and machine that can serve the 1 MB image, and the
image still works where it cannot.

## Tasks

- [ ] RP2350B, A600, a default ROM of 512 KB or less (slot staged): A19 `ok`, the list and a
  selection that boots (Diego)
- [ ] RP2350B, A600, a 1 MB default ROM (slot not staged): A19 `skipped`, the list and a
  selection that boots (Diego)
- [ ] RP2350B, A500 or A2000: the list, the self-test and a selection that boots, with GPIO 25
  as the carrier wires it (Diego)
- [ ] RP2350A, A500 or A2000: the same, with A19 `skipped` (Diego)
- [ ] The 30-second soak on the A600 with the slot staged: no error (Diego)

## Acceptance

All five checked by Diego.

## Notes

Needs firmware EPIC-17 on both boards, with the option 1 staging and the owner record.
