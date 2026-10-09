---
id: STORY-06
epic: EPIC-00
title: Verification (Diego's checkpoint)
status: todo
---

## Goal

Diego sees the harness work and agrees that it stands in for the machines for everything but the
bus (D-06).

## Tasks

- [ ] Diego watches `tools/dev/all_harness.sh` pass on this Mac.
- [ ] Diego makes a deliberate break and sees the right run FAIL with its evidence.
- [ ] Diego runs `--interactive` on each image and agrees with what he sees, including the mono
      screen.
- [ ] Diego confirms or amends D-06 and D-07.

## Acceptance

All four tasks checked by Diego.

## Notes

This is not a bench pass: nothing here touches a SidecarTridge. The bench checks of v4.0.0 belong
to the hardware-verification candidate in `ITERATIONS.md`.
