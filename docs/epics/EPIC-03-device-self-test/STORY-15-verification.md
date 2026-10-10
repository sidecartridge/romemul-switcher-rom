---
id: STORY-15
epic: EPIC-03
title: Verification (Diego's checkpoint)
status: done
---

## Goal

Diego sees the self-test pass on real hardware and agrees with its screen.

## Tasks

- [x] The self-test screen by eye in Hatari and FS-UAE (`--interactive`), colour and mono (Diego)
- [x] On the RP2350A, on an ST and an Amiga: address and data lines `ok`, pings answered, the
      reliability counts, identical flash reads, the catalog and configuration right, the
      machine named right (Diego)
- [x] On the RP2350B: the same (Diego)
- [x] An STE or Mega STE with the 256 KB image (Diego)
- [x] The 30-second soak (S on the self-test screen) on both boards: no error (Diego)
- [x] The boot ping: with the device in place, the list loads as before; and the normal
      selection of a ROM still boots it, now that the nonce changes on every frame (Diego)
- [x] Every line of the boot screen, the list and the self-test within 80 columns, and the
      rescue image's size and the RAM line right for each machine on the bench (Diego)
- [x] The mono display starts in place after a cold boot: power on into rescue mode several
      times on the Mega ST in mono, and once on a colour monitor: no wrapped columns (Diego)

## Acceptance

All eight checked by Diego.

## Notes

- 2026-10-10, Diego, on the Mega ST in mono with the 256 KB image, after STORY-14: "i think it
  is working now." The cold-boot task stays open until he has seen several power-ons. The Amiga
  is still to be tested: "I have to test in an Amiga. We will do it later."
- 2026-10-10, Diego, on an Amiga with the 512 KB release image (build `b302aa1-dirty.354d9d6`,
  SHA-256 `7c37ad0f…236d30`) and release firmware: "everything works great for the amiga
  computer too." No task is checked yet: which board it ran on, and whether the soak and a
  selection were part of the run, are still to be confirmed.
- 2026-10-10, Diego, on both boards: "RP2350A and RP2350B, all of them"; every self-test line
  right; the soak "Yes, no errors"; a selection "worked. Several times"; the 80 columns, the
  image size and the RAM line "Yes, it worked"; and "It also works in mono and color without
  any issue in ST and STE." Seven tasks checked. The look at the self-test screen in Hatari and
  FS-UAE with `--interactive` is the one left.
- 2026-10-10, the look in the emulators. At the first, Diego: "Ping failed. I think we should
  skip it in hatari and fs-uae"; a test build now reports it `skipped` (STORY-05), and
  `all_harness.sh` passed 25 of 25 on it. Then four `--interactive` sessions on the images of
  the current sources, each with T and S, all "ok" by Diego:

  | session | self-test | soak |
  |---|---|---|
  | Hatari ST, colour, 192 KB | 9 passed, 0 failed, 3 skipped | 30 s, 147,413 reads, 0 errors |
  | Hatari Mega ST, mono, 192 KB | 9, 0, 3 | 28 s (ESC), 137,337 reads, 0 errors |
  | Hatari Mega STE, colour, 256 KB | 9, 0, 3 | 30 s, 179,691 reads, 0 errors |
  | FS-UAE A500, 512 KB | 8, 0, 3 (no monitor line) | 30 s, 178,771 reads, 0 errors |

  No `@@ overflow` in any of them. All eight tasks checked.
- The release images built from the final sources are byte for byte the ones Diego ran on the
  bench (the ping change is in test builds only): 192 KB `65b0d0c3…`, 256 KB `56e94d56…`,
  512 KB `7c37ad0f…`.

**Done 2026-10-10.**
