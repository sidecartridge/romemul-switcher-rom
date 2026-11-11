---
id: STORY-04
epic: EPIC-06
title: The harness on a 1 MB Kickstart
status: todo
---

## Goal

FS-UAE runs the 1 MB image the way the RP2350B serves it to an A600, so STORY-02 and STORY-03
are exercised before the bench.

## Tasks

- [ ] Find out how FS-UAE maps a 1 MB `kickstart_file`, and on which models. UAE is expected to
  put the first 512 KB at `$E00000` and the second at `$F80000` on an A1200 or A600 model, the
  board's own order, but this is not checked yet. Record what it does.
- [ ] `tools/dev/fsuae_harness.py --rom-1mb` (or a model option) boots the 1 MB debug-test image
  that way.
- [ ] Sessions:
  - the 1 MB image with both halves served (A19 `ok`);
  - the 1 MB image with the high marker overwritten (A19 `stuck`);
  - the 512 KB image on a Gayle model (A19 `skipped`), and on the A500 model (A19 `n/a`).

  All four are added to `all_harness.sh`.
- [ ] The test build's parameters page carries the "slot holds the rescue half" value that each
  session needs.

## Acceptance

`all_harness.sh` passes with the four new sessions, and the 25 earlier ones are unchanged.

## Notes

FS-UAE answers no bus commands (C-03). The test build's fake flash stands in for the reply
channel, so the harness checks the marker logic and the screen, not the board's half selection.
