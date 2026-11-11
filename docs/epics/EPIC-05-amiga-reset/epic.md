---
id: EPIC-05
iteration: 2
title: The Amiga reset waits for the firmware
status: done
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

It needs no firmware work, so its place in the order is free. It comes after EPIC-04, whose
fixes are smaller.

Diego reported selections that boot "several times" on the Amiga (EPIC-03 STORY-15). The race
has never been seen to fail, so this epic is about margin, and the bench has to show it changes
nothing visible.

**Started 2026-11-11** on `epic-05-amiga-reset`, before EPIC-04's bench check (Diego: "go with
epic 05"). STORY-01 is done in the emulators. The bench check is STORY-02.

**Done 2026-11-11.** Diego's bench check passed with the release images of `f5cb491` (see the verification story). The code reaches `release/v4.0.0` through EPIC-07.
