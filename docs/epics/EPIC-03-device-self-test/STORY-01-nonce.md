---
id: STORY-01
epic: EPIC-03
title: A new nonce for every command
status: todo
---

## Goal

Every command frame carries a nonce different from the previous one and from the word the ROM
reads at base+0, on both platforms, so a stale echo can never pass for an answer. The ping
tests (STORY-05, STORY-06) depend on it. Was a candidate in `ITERATIONS.md`.

## Tasks

- [ ] One nonce generator in `src/common` (a xorshift state seeded once from
      `platform_get_system_time_seed()`, advanced on every frame, masked `& 0xFFFEFFFE`, and
      redrawn while it equals the previous nonce or the long at base+0), used by both
      `platform_send_magic_sequence()`; the per-platform seed-and-xorshift code goes
- [ ] Debug trace of the nonces the self-test sends, so the harness can check they all differ
- [ ] Release payload sizes before and after; the harness suite still passes

## Acceptance

In the emulators, the ping lines of the self-test (STORY-05) show a different nonce on every
frame. On the bench the normal ROM list still loads (STORY-11).

## Notes

Today the ST seed samples MFP timers that the startup `reset` stopped, and the Amiga seed is the
long at address 4: one nonce for a whole run (`ITERATIONS.md`, both sibling sessions agreed).
