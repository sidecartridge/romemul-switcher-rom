---
id: STORY-02
epic: EPIC-06
title: The harness
status: done
---

## Goal

FS-UAE checks both answers before the bench.

## Tasks

- [x] Every A500 session expects `@@ selftest buschip Gary id=0x00`.
- [x] `fsuae_harness.py --model a600` boots the image on FS-UAE's A600, and expects an ECS Agnus
  (`chip=0x20`) and `@@ selftest buschip Gayle id=0xd0`.
- [x] The A600 session in `all_harness.sh`, and the whole run passing.

## Acceptance

`all_harness.sh` passes with the new session.

## Notes

FS-UAE answers no bus commands (C-03). These sessions check the Gayle reads and the screen.

**Done 2026-11-11.** `all_harness.sh` passed 32 of 32 with 0 key retries, `fsuae-amiga-a600`
included, its Agnus `0x20` and Denise `0xfc`.
