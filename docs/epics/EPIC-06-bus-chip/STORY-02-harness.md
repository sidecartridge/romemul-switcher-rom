---
id: STORY-02
epic: EPIC-06
title: The harness
status: todo
---

## Goal

FS-UAE exercises both answers before the bench.

## Tasks

- [ ] The A500 sessions say 512 KB. Check what FS-UAE returns at `$E00000` on an A500 with a
  512 KB Kickstart, and record it.
- [ ] One session on a model that maps `$E00000` as ROM, an A600 or A1200, with a 1 MB
  `kickstart_file` made of two copies of the debug-test image. This stands in for the board's
  mirror. Check how FS-UAE maps a 1 MB Kickstart on that model, and record it. The session
  says 1 MB.
- [ ] Both sessions in `all_harness.sh`.

## Acceptance

`all_harness.sh` passes with the new session. The 25 earlier ones are unchanged, apart from the
new line.

## Notes

FS-UAE answers no bus commands (C-03). These sessions check the probe and the screen, not the
board.
