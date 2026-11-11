---
id: STORY-02
epic: EPIC-06
title: The reply channel, found from the marker
status: todo
---

## Goal

The ROM reads replies wherever the firmware writes them: at `$F80000` when the board serves one
512 KB image on both halves, and at `$E00000` when the slot serves the upper half.

## Tasks

- [ ] At boot, before the boot ping, the ROM reads the marker word at `$F80000`:
  - the high half's marker means the slot is being served, so replies are read at
    `$E00000`;
  - the low half's marker, or a 512 KB image's word, means replies are read at `$F80000`.

  No ping waits on a timeout to find this out.
- [ ] `src/common/commands.c`: the magic sequence keeps reading at the ROM base. The reply
  addresses (`magic_number_address`, `remote_rom_address`) take the chosen base, which
  `init_rom_address()` receives separately.
- [ ] The ST and STE do not change: their reply base stays their ROM base.
- [ ] `@@ channel <base>` in debug builds. The harness reads it.
- [ ] Release payload sizes before and after.

## Acceptance

In FS-UAE (STORY-04), the 1 MB image on a machine that maps it on both halves pings and lists
through `$E00000`. The 512 KB image still does so through `$F80000`.

## Notes

Unblocked 2026-11-11 by Diego's answer on the carriers (see the epic's notes). On a 512 KB
carrier, GPIO 25 is tied to GND, so every read comes from SRAM and the marker reads low even with
the half staged. Replies then stay at `$F80000`, and nothing on an A500 or A2000 reads
`$E00000`.

If A19 is stuck high on a staged board, both addresses read the slot and no channel answers.
The boot ping then reports "does not answer", and the self-test cannot run. Stuck low reads the
low half at both, and the channel works at `$F80000`.
