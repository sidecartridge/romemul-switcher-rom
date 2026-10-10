---
id: STORY-11
epic: EPIC-03
title: Stress reads and a 30-second soak
status: done
---

## Goal

The address and data lines are checked with many reads, not a few: intermittent faults
(marginal timing, a poor contact, noise) only show over thousands of reads. A quick stress pass
runs inside the self-test, and S on the self-test screen runs a 30-second soak with live counts.
Diego, 2026-10-10: "use the computed pattern, 30 second soak, add it".

## Tasks

- [x] The unused space of every image is a computed pattern, not random bytes: the word at an
      even offset is mixed from the offset (`fill_word()` in `scripts/finalize_rom.py`,
      mirrored in `src/common/selftest.c`), so the ROM can check a read anywhere in it. Images
      become reproducible: a clean build of a commit gives the same bytes anywhere, CI included
      (C-06 amended)
- [x] The stress region: from `0x10000` to 4 KB below the end, skipping the self-test's words
      (any offset one address bit from a reference) and the Amiga's kickety split; the finalizer
      refuses a payload that reaches `0x10000`
- [x] Each step reads a random address of the region as a word, as two bytes and as a long, and
      the address with every line flipped; a mismatch one bit off counts against that data line,
      one that matches the pattern of an address one line away counts against that address line
- [x] A quick pass in the self-test ("Stress reads": reads, errors, and the lines at fault);
      `@@ selftest stress reads=.. errors=.. data=.... addr=.....`
- [x] The soak: S on the self-test screen runs the same steps for 30 seconds of video frames
      (the beam position on the Amiga, the video counter on the Atari), with live counts, ESC to
      stop early; `@@ soak start`, `@@ soak end seconds=.. reads=.. errors=.. data=.. addr=..`
- [x] Harness: the quick pass with no error in every session; the soak in one Hatari and one
      FS-UAE session; `--stress-fault K` (data line K flipped in every pattern word of the region,
      checksums recomputed) reported against that line and no other
- [x] Release payload sizes before and after; `CLAUDE.md`, `README.md` and `tools/dev/README.md`
      updated

## Acceptance

In the emulators: no error in the quick pass or the soak; the fault runs name the right line. On
the bench: STORY-15.

## Notes

**Done 2026-10-10.**

- The pattern: `fill_word()` / `selftestFillWord()`, a shift-and-add mix of the offset (no
  multiply, which needs libgcc). The first version was a xorshift32, which is linear: the word
  one address line away then always differed by the same constant (A14: `0xCC08`, `0x08` in
  the odd byte lane), so a stuck A14 read as a D3 fault. The mix gives about 50,000 distinct
  differences per line, under 0.05 % of them a single bit.
- Reproducible: three release builds rebuilt, each byte for byte the same (SHA-256); C-06
  amended.
- The stress region `[0x10000, size - 0x1000)`; `stressReserved()` skips any offset one
  address bit from the address test's reference (or the ST's second one) and the middle 8
  bytes (the Amiga's kickety split); the finalizer refuses a payload past `0x10000`.
- A step: a random even address (xorshift32 of the step), read as a word, two bytes and a
  long, and the address with every line of the window flipped. Byte reads compare only their
  lane. A mismatch one bit off counts against that data line, otherwise against the address
  line whose pattern word it matches. A line is blamed when it carries at least 1/64 of the
  errors, because a 16-bit word matches another address's word by chance about once in 4,000
  errors.
- Quick pass: 4,000 steps, about 11,600 reads on the ST and 20,800 on the Amiga in the debug
  builds; "Stress reads ok 11556 reads, no error".
- Soak: S on the self-test screen; 30 s of frames from `platform_frames()` (the Atari's video
  counter falling back, 50, 60 or 71 Hz from the mode; the Amiga's beam wrapping, 50 or 60 Hz
  from the Agnus); live counts every second; ESC stops it ("(stopped)" in the result). In the
  emulators, debug builds: 148,091 reads on the ST, 179,998 on the Amiga, no error.
- `all_harness.sh`: 20 of 20 PASS. New runs: `--soak` (ST, Amiga), `--stress-fault 5` (STE:
  14,789 errors, D5 alone), `--stress-fault 12` (Amiga: D12 alone), `--stress-alias 14` (ST:
  "A14 x5407" of 5,567 errors, A14 alone), `--stress-alias 18` (Amiga: A18 alone).
- Release payloads: `st`/`ste` 24,228, `amiga` 24,380 bytes (21,768 and 22,016 after
  STORY-10).
