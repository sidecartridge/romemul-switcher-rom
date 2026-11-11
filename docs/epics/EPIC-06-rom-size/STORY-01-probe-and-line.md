---
id: STORY-01
epic: EPIC-06
title: The probe and the self-test line
status: todo
---

## Goal

The Amiga self-test shows a line saying whether the computer supports a 512 KB or a 1 MB
Kickstart, read from whether `$E00000` holds this image.

## Tasks

- [ ] `src/amiga/platform.c`: the probe compares a few fixed words of the image at `$E00000`
  with the same offsets at `$F80000`. Suitable words are the address-test signatures near the
  end and the start of the image. A match means 1 MB, any difference 512 KB. It reaches
  `src/common` through `platform_machine_t` (`src/common/platform.h`) as a ROM size in KB. That
  size is 0 on the ST and STE, where the line is not shown.
- [ ] `src/common/selftest.c`: a line next to "Rescue image" (`selftestImage()`, EPIC-03
  STORY-13), for example "ROM socket ok 1 MB (0xE00000 and 0xF80000)" or "ROM socket ok 512 KB
  (0xF80000)". It is within 80 columns (EPIC-03 STORY-12). The Amiga's self-test goes from 11
  lines to 12.
- [ ] `@@ selftest socket 512|1024` in debug builds. `tools/dev/harness_common.py` expects it on
  the Amiga.
- [ ] Check that reading `$E00000` is harmless on every supported Amiga: an A500 or A2000 with
  Gary, and an A600. Use the Hardware Reference Manual, then the bench. Record the source here.
- [ ] Release payload sizes before and after.

## Acceptance

In FS-UAE (STORY-02) the A500 says 512 KB, and a model that maps `$E00000` says 1 MB. On
hardware: STORY-03.

## Notes

- The words must be ones the image never changes at runtime and a mirror cannot fake. The
  address-test signatures and the fill pattern qualify. Open bus or another ROM at `$E00000`
  will not match several of them.
