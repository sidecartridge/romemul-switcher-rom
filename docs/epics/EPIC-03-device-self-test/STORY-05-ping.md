---
id: STORY-05
epic: EPIC-03
title: The command test (ping)
status: todo
---

## Goal

The ROM sends `CMD_PING` several times and says whether the device answers, and how reliably.

## Tasks

- [ ] N pings (N decided here), each with STORY-01's nonce, each answered by the echo of its
      nonce at base+0, each followed by
      `CMD_RESTORE_PREVIOUS_ROM` (0x000E) and a check that base+0 holds the ROM's own word again
- [ ] Results: answered, timed out, checksum error (`0xFFFFFFFD`); `@@ selftest ping` lines in
      debug builds
- [ ] The firmware session told that this ROM now sends `CMD_PING` and
      `CMD_RESTORE_PREVIOUS_ROM` (D-03); no contract change
- [ ] Harness: in the emulators no device answers (C-03), so the test must report "no answer"
      within its time limit and the chooser must stay usable

## Acceptance

In the emulators: "no answer", reported cleanly. On the bench, both boards: every ping
answered (STORY-04).

## Notes

`cmd_ping()` echoes the nonce and restores nothing (`epic.md`); `CMD_RESTORE_PREVIOUS_ROM`
copies the image's first bytes back from flash (`cmd_restore_previous_rom()`, firmware
`emul/src/tosemul.c`). STORY-01's nonce, new on every frame, and the restore between pings
together keep a stale echo from counting as an answer.
