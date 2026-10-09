---
id: EPIC-03
iteration: 1
title: A self-test of the device, from the rescue ROM
status: todo
---

## Goal

From the chooser, the user runs a self-test that says whether the SidecarTridge serves the ROM
correctly on every address line and answers commands, with a result per line and per test.
The ROM checksum at boot says that something is wrong; the self-test says what. Asked by
Diego, 2026-10-09: "a self test of the device running here ... one of each of the Address
lines of the emulation ... check if the device responds to the commands (there is a ping
command)".

## Scope

- In scope: the nonce fix the ping tests need; the self-test screen; address-line, data-line,
  ping, command-reliability, flash-read, catalog-and-configuration and machine-information
  tests; a ping at boot; trace lines and harness sessions for each; Diego's check on the bench.
- Out of scope: any new firmware command (a shared-contract change, D-03); a RAM test of the
  computer.

## Platforms

All three images: `st` (A1 to A17 over 192 KB), `ste` (A1 to A17 over 256 KB), `amiga` (A1 to
A18 over 512 KB). The verification story covers both boards, RP2350A and RP2350B, on an ST,
an STE and an Amiga.

## Stories

- STORY-01: A new nonce for every command
- STORY-02: The self-test screen
- STORY-03: The address-line test
- STORY-04: The data-line test
- STORY-05: The command test (ping)
- STORY-06: Command reliability
- STORY-07: Flash read consistency
- STORY-08: Catalog and parameters sanity
- STORY-09: Machine information
- STORY-10: A quick ping at boot
- STORY-11: Verification (Diego's checkpoint)

## Notes

Runs after EPIC-01 and before EPIC-02, the release gate: EPIC-02 keeps its number because the
firmware session already cites it. The images change, so the firmware re-pins after this epic
(C-06), and the new screen gets its own look by eye in STORY-04, since EPIC-01's looks predate
it.

Facts the design rests on, checked 2026-10-09:

- `CMD_PING` (0x0012) in the firmware (`emul/src/tosemul.c`, `cmd_ping()`) writes the nonce to
  base+0 and returns; the parameter is ignored and nothing is restored. A ping must therefore
  be followed by `CMD_RESTORE_PREVIOUS_ROM` (0x000E), or base+0, which the ST also reads as the
  start of its reset vector through the low-memory mirror, keeps the nonce. Both commands
  already exist; this ROM has used only `SELECT_ROM` and the two read-block commands so far,
  so the firmware session is told (D-03), though nothing in the contract changes. The host
  SWITCHER sends a ping only inside its write path (`helper.c`).
- The firmware decodes A1 to A18 on both boards (firmware D-21); the RP2350B's A18 also picks
  the half of a 1 MB image (firmware D-03), which a 512 KB rescue image never reaches. The
  68000 has no A0: the two byte lanes are UDS and LDS, which a data-line test would cover.
- With the constant nonce of the candidate in `ITERATIONS.md`, a ping could take a stale echo
  for an answer. Restoring base+0 between pings prevents that; fixing the nonce makes the test
  honest by construction.
- Address lines A1 to A13 address the first 16 KB, where the code the ROM copies to RAM at
  boot lives: if one of them fails, the ROM most likely never reaches the chooser, and only
  the boot checksum, or nothing, can tell. The test is most informative for the higher lines.

Diego, 2026-10-09: "add 1, 2, 3, 4, 5, 6 and 7, then start epic 3". The seven suggestions are
STORY-04 (data lines), STORY-06 (command reliability), STORY-07 (flash reads), STORY-08
(catalog and configuration), STORY-01 (the nonce, first because the ping tests depend on it),
STORY-09 (machine information) and STORY-10 (the boot ping).
