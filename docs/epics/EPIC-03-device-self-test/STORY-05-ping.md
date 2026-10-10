---
id: STORY-05
epic: EPIC-03
title: The command test (ping)
status: done
---

## Goal

The ROM sends `CMD_PING` several times and says whether the device answers, and how reliably.

## Tasks

- [x] N pings (N decided here), each with STORY-01's nonce, each answered by the echo of its
      nonce at base+0, each followed by
      `CMD_RESTORE_PREVIOUS_ROM` (0x000E) and a check that base+0 holds the ROM's own word again
- [x] Results: answered, timed out, checksum error (`0xFFFFFFFD`); `@@ selftest ping` lines in
      debug builds
- [x] The firmware session told that this ROM now sends `CMD_PING` and
      `CMD_RESTORE_PREVIOUS_ROM` (D-03); no contract change
- [x] Harness: in the emulators no device answers (C-03), so the test must report "no answer"
      within its time limit and the chooser must stay usable
- [x] A test build reports the ping `skipped`, not `FAIL`, when no device answers: it runs in
      the emulators, which have none; the 8 pings still go out. The harness expects no failure
      but the injected one and three skipped

## Acceptance

In the emulators: "no answer", reported cleanly. On the bench, both boards: every ping
answered (STORY-04).

## Notes

`cmd_ping()` echoes the nonce and restores nothing (`epic.md`); `CMD_RESTORE_PREVIOUS_ROM`
copies the image's first bytes back from flash (`cmd_restore_previous_rom()`, firmware
`emul/src/tosemul.c`). STORY-01's nonce, new on every frame, and the restore between pings
together keep a stale echo from counting as an answer.

**Done 2026-10-09.** `command_ping()` in `commands.c`: one `CMD_PING` (param 0, timeout
`0x4000` polls), then `restore_first_long()`, which sends `CMD_RESTORE_PREVIOUS_ROM` only while
base+0 differs from the image's own first long (captured in `init_rom_address()`, before any
command) and waits for it, up to 16 tries of `0xFFFF` polls. Results: 0 answered, -1 no answer,
-2 checksum error, -3 answered but not restored. The self-test sends 8 pings, all of them even
with no answer; `@@ selftest ping <i> answered|noanswer|checksum|norestore` and `@@ selftest ping
answered=<a> sent=8`; the screen says "Ping ok 8 of 8 answered", or "FAIL no answer: the device
does not take commands", or the counts. `device_answers` gates the device tests that follow.

- Emulators: all 8 `noanswer`, about 1.5 s; `all_harness.sh` 14 of 14 PASS (51 steps).
- STORY-01's evidence: in every run the 8 ping frames carry 8 distinct nonces, checked by the
  harness. No restore frame is sent there, since base+0 never changed.
- The firmware session was told (D-03) on 2026-10-09, with two questions: whether a burst of
  about 1,000 ping and restore pairs is fine for its main loop (STORY-06), and whether a
  restore sent when base+0 is already right is harmless.

**2026-10-10.** Diego, at the look in Hatari (STORY-15): "Ping failed. I think we should skip it
in hatari and fs-uae". `selftestPing()` in a test build: with all 8 pings unanswered the line is
"Ping skipped test build: 8 sent, no device to answer". Release and debug builds still say
FAIL, as a device that does not answer must. The emulators can only be told apart by the build:
the harnesses always run debug+test images, and no image probes for an emulator.
- `all_harness.sh` 25 of 25 PASS, every session ending `failed=0 skipped=3`, or `failed=1` with
  the injected fault. The self-test screen: "9 passed, 0 failed, 3 skipped."
