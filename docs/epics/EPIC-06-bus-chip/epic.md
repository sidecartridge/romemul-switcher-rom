---
id: EPIC-06
iteration: 2
title: The ROM size the Amiga supports, in the self-test
status: todo
---

## Goal

The Amiga self-test says whether the computer supports a 512 KB or a 1 MB Kickstart. A 1 MB
machine (the A600) also maps the Kickstart socket at `$E00000`, while a 512 KB machine (the A500
or A2000) maps it only at `$F80000`. The user can then see whether the machine can take a 1 MB
ROM before choosing one. Diego, 2026-11-11: "We drop the 1MB romemul-switcher-rom. Now we only
are going to limit the scope to display in the test that the computer supports 512KB or 1MB."

## Scope

- In scope:
  - an Amiga probe of `$E00000`;
  - a self-test line with the result;
  - its trace;
  - the harness sessions;
  - Diego's check on the bench.
- Out of scope, dropped on 2026-11-11:
  - a 1 MB rescue image;
  - an A19 line test;
  - a reply channel at `$E00000`;
  - any firmware "staged" flag or owner record for the rescue.

  The rescue stays one 512 KB image for both boards (D-08), and the shared contract does not
  change.
- Also out of scope: the ST and STE.

## Platforms

`amiga` only. The verification covers both boards on an A500 or A2000 and on Diego's A600.

## Stories

- STORY-01: The probe and the self-test line
- STORY-02: The harness
- STORY-03: Verification (Diego's checkpoint)

## Notes

How the probe works with a 512 KB image. With the firmware's mirror fix (in the re-issued
firmware v4.0.0), an image of 512 KB or less is served on both halves, so A19 is ignored, like a
512 KB chip.

- **1 MB machine.** On the A600, a 1 MB carrier wires the CPU's A19 to GPIO 25, so reads at
  `$E00000` (A19 low) and at `$F80000` (A19 high) both return this image.
- **512 KB machine.** On an A500 or A2000, the 512 KB carrier ties GPIO 25 to GND, and
  `$E00000` is not the Kickstart socket at all.

So "`$E00000` holds this image" means the machine supports a 1 MB ROM. The rescue boots on the
A600 only with the mirror fix: without it, its `$F80000` reads went to the empty slot. So a
rescue that reaches the self-test on an A600 always has `$E00000` served.

These facts come from the firmware session, 2026-11-11: the A600 measurements, and Diego's
answer on the carriers.

**Rolled back the same day.** EPIC-06 was first "The A19 line on the Amiga, and a 1 MB rescue
image". Its five stories planned:
- a 1 MB image with two halves;
- a reply channel chosen from a marker;
- an A19 test that read a firmware "staged" record;
- FS-UAE on a 1 MB Kickstart;
- a verification.

Diego dropped the 1 MB image. The plan is in git history (`71cfe89` to `bd4652c`).
