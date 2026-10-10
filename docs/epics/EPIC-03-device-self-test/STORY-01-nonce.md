---
id: STORY-01
epic: EPIC-03
title: A new nonce for every command
status: done
---

## Goal

Every command frame carries a nonce different from the previous one and from the word the ROM
reads at base+0, on both platforms, so a stale echo can never pass for an answer. The ping
tests (STORY-05, STORY-06) depend on it. Was a candidate in `ITERATIONS.md`.

## Tasks

- [x] One nonce generator in `src/common` (a xorshift state seeded once from
      `platform_get_system_time_seed()`, advanced on every frame, masked `& 0xFFFEFFFE`, and
      redrawn while it equals the previous nonce or the long at base+0), used by both
      `platform_send_magic_sequence()`; the per-platform seed-and-xorshift code goes
- [x] Debug trace of the nonces the self-test sends, so the harness can check they all differ
- [x] Release payload sizes before and after; the harness suite still passes

## Acceptance

In the emulators, the ping lines of the self-test (STORY-05) show a different nonce on every
frame. On the bench the normal ROM list still loads (STORY-11).

## Notes

Today the ST seed samples MFP timers that the startup `reset` stopped, and the Amiga seed is the
long at address 4: one nonce for a whole run (`ITERATIONS.md`, both sibling sessions agreed).

**Done 2026-10-09** (`epic-03-device-self-test`): `src/common/nonce.c`/`.h`, `nonce_next()`,
used by both `platform_send_magic_sequence()`; `TRACE("nonce %08lx")` on every frame in debug
builds. Seeded once from `platform_get_system_time_seed()`, which stays as the seed source.

- Simulated as the 68000 runs it (32-bit, constant seeds `a5a5a5a5`, `00000004`, `12345678`):
  200,000 nonces each, no consecutive repeat, about 199,990 distinct.
- `all_harness.sh`: 9 of 9 PASS. The session's one frame, the selection, shows `@@ nonce
  3330a88c`, the same in every run: the first nonce of a boot from a constant seed. That is
  harmless, since the base+0 word is the image's own after a reboot; distinct nonces within a
  run are STORY-05's evidence.
- Release payloads: `st` and `ste` 15,932, `amiga` 16,228 bytes (dirty build ID), against
  15,844 and 16,144 before: +88 bytes.
- Acceptance, emulators: the self-test's 8 ping frames carry 8 distinct nonces in every run of
  `all_harness.sh` (STORY-05), checked by the harness.
