---
id: STORY-01
epic: EPIC-01
title: Hatari, by eye (Diego's checkpoint)
status: todo
---

## Goal

Every screen of the ST and STE images looks right on every machine Hatari offers us, typed on
with the real keyboard.

## Tasks

- [ ] `tools/dev/build.sh st debug-test && python3 tools/dev/hatari_harness.py --image st --interactive`:
      the checksum screen with the build line, the list, both pages, the confirmation, ESC, a
      selection and the reset (Diego)
- [ ] The same with `--monitor mono`: 640x400, crisp, nothing cut off (Diego)
- [ ] `--image ste --interactive` on an STE, and with `--machine megaste` (Diego)
- [ ] `--corrupt --interactive`: the checksum-fail screen shows both values and the build line,
      and a key reaches the list (Diego)

## Acceptance

Diego agrees with what he saw on each run; anything wrong becomes a candidate in
`ITERATIONS.md` or a fix before EPIC-02.

## Notes
