---
id: EPIC-05
iteration: 2
title: The Amiga reset waits for the firmware
status: todo
---

## Goal

After a selection, the Amiga resets once the firmware serves the new image. Today it resets
about 2 µs after `SELECT_ROM`, while the old image is still on the bus (C-07).

## Scope

- In scope:
  - the wait and the reset in `src/amiga/startup.s` and `src/amiga/platform.c`;
  - the decision that replaces `CLAUDE.md`'s rule to keep the reset stub in ROM;
  - harness and bench checks.
- Out of scope:
  - the ST, which already waits about 2.9 s;
  - any firmware change: neither board has a line to RESET (C-07).

## Platforms

`amiga` only. The verification covers both boards, on an A500 or A2000 and on Diego's A600.

## Stories

- STORY-01: A reset that does not depend on the old image
- STORY-02: Verification (Diego's checkpoint)

## Notes

From iteration 1's candidates (2026-10-09), where it was "Diego's call" because it contradicts
`CLAUDE.md`: "keep the true cold-reset stub in `startup.s` ROM code".

This epic comes before EPIC-06 for two reasons. EPIC-06's 1 MB image runs from the board's flash
slot, where today's ROM-resident stub would run from too. A reset that no longer reads the old
image removes that dependency before the image layout changes.

Diego reported selections that boot "several times" on the Amiga (EPIC-03 STORY-15). The race
has never been seen to fail, so this epic is about margin, and the bench has to show it changes
nothing visible.
