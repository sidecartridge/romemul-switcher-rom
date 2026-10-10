---
id: STORY-06
epic: EPIC-03
title: Command reliability
status: done
---

## Goal

When the device answers, the self-test sends many pings in a row and reports how many were
answered, timed out or failed their checksum: a measure of the bus losses firmware EPIC-06
found under TOS, here with interrupts masked.

## Tasks

- [x] N pings (N decided here, long enough to see a 0.6 % loss rate), each restored as in
      STORY-05; counts of answered, timeouts and checksum errors (`0xFFFFFFFD`)
- [x] Skipped, and said so, when STORY-05's ping got no answer
- [x] `@@ selftest reliability` line; harness: "skipped" in the emulators (C-03)

## Acceptance

In the emulators: skipped cleanly. On the bench, both boards: the counts (STORY-11).

## Notes

**Done 2026-10-09.** N = 1,000 pings (`kReliabilityCount`), each through `command_ping()`, with
the progress shown every 100; `@@ selftest reliability answered=<a> noanswer=<n> checksum=<c>
norestore=<r>`, or `@@ selftest reliability skipped` when STORY-05's pings got no answer. The
screen says "Command reliability ok 1000 of 1000 answered, 0 lost, ..." or "FAIL" with the
counts. Emulators: skipped, `all_harness.sh` 14 of 14 PASS.

The firmware session confirmed on 2026-10-09 (firmware `tosemul.c` at `bbd6c75`) that 1,000
ping and restore pairs are no load: the IRQ queues each frame in a 16-entry ring and the main
loop runs it in microseconds, reading flash only; the ROM waits for each reply, so the ring
never holds more than one. A repeated restore is harmless. A frame that fails its checksum
makes base+0 read `0xFFFFFFFD`: no nonce can be that value, since the mask clears bit 0, and
`command_ping()` counts it as a checksum error. On hardware the firmware's own counters
(`swd.py counters`: `tosemul_cmd_detected`, `_handled`, `_errors`, `_aborted`) can cross-check
the test; with interrupts masked no loss is expected (firmware D-21).
