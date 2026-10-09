---
id: STORY-02
epic: EPIC-01
title: FS-UAE, by eye (Diego's checkpoint)
status: todo
---

## Goal

Every screen of the Amiga image looks right on FS-UAE's A500, typed on with the real keyboard,
which is the CIA path the harness never uses (it types through the serial port).

## Tasks

- [ ] `tools/dev/build.sh amiga debug-test && python3 tools/dev/fsuae_harness.py --interactive`:
      the checksum screen with the build line, the list, both pages, the confirmation, ESC, a
      selection and the reset, all with the keyboard (Diego)
- [ ] `--corrupt --interactive`: the checksum-fail screen, then any key reaches the list (Diego)

## Acceptance

Diego agrees with what he saw; anything wrong becomes a candidate or a fix before EPIC-02.

## Notes
