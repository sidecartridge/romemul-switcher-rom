---
id: EPIC-06
iteration: 2
title: The A19 line on the Amiga, and a 1 MB rescue image
status: todo
---

## Goal

The self-test checks the Amiga's A19 line, which picks the half of a 1 MB image on the RP2350B:
`ok`, `stuck`, or `skipped` when the board serves only 512 KB. For that, the Amiga rescue
image becomes 1 MB, with two halves that differ, and the ROM finds the reply channel in
whichever half the firmware writes it. Asked by Diego through the firmware session, 2026-11-11,
after his A600 measurements.

## Scope

- In scope:
  - the 1 MB image: two finalized 512 KB halves, `$E00000` half first;
  - the reply channel chosen at boot;
  - the A19 line in the self-test;
  - the harness;
  - the release name and the firmware's pin (D-03).
- Out of scope:
  - the firmware's slot staging and its owner record (firmware EPIC-17, and firmware EPIC-03
    STORY-01 / D-13);
  - the ST and STE, whose 192 and 256 KB windows have no such line;
  - naming the A600 in the title line, which says "Amiga 500/2000". That is a candidate if
    Diego wants it.

## Platforms

`amiga` only. The verification needs:
- the RP2350B on an A600 with option 1 firmware, both with the slot staged and without;
- the RP2350B on an A500 or A2000;
- the RP2350A on an A500 or A2000.

## Stories

- STORY-01: The 1 MB image
- STORY-02: The reply channel, found from the marker
- STORY-03: The A19 line in the self-test
- STORY-04: The harness on a 1 MB Kickstart
- STORY-05: Verification (Diego's checkpoint)

## Notes

Facts from the firmware session, 2026-11-11, measured on Diego's A600 with the RP2350B:

- The A600 drives CPU A19 high at `$F80000` and low at `$E00000`. The board receives it on
  GPIO 25 (its `ST_A18`), the read PIO's JMP pin. Low serves the image's first 512 KB from
  SRAM. High serves the second 512 KB from the flash slot at `0x10F00000`.
- A 1 MB image for this board is therefore the `$E0` half first. The kickstart builder's
  27C800 order (`$F8` first) is wrong for this board.
- With the firmware's mirror fix (in the re-issued firmware v4.0.0), an image of 512 KB or
  less is served on both halves. Today's 512 KB rescue cannot see A19 at all.
- emul writes every reply into SRAM only. When the upper half is staged, reads at `$F80000`
  come from the slot, so the replies are at `$E00000`.
- The magic sequence matches the low 16 bits of the address, so it works with A19 high or low
  (`tosemul.c:475`).
- The RP2350A has no slot and never serves more than 512 KB.

Diego chose option 1 on 2026-11-11: the rescue shares the slot with the default ROM. Its upper
half is staged only when the default ROM is 512 KB or less, otherwise the test says
`skipped`.

Open, and blocking STORY-02 and STORY-03:

- **How the ROM learns whether the slot holds the rescue's upper half.** The firmware first
  proposed bit 0x4 of `romflags` (parameters byte 532). It is now on hold: the slot gets an
  owner record (firmware EPIC-03 STORY-01, D-13), in a parameters-page word or a sector after
  the slot. The meaning stays "the slot holds the rescue image's half, yes or no", and the
  host SWITCHER carries the record unchanged. The firmware will send the offset and values
  with its commit hash.
- **GPIO 25 on the A500 and A2000 carriers.** It is an input with no pull
  (`tosemul_2350b.pio.h:177-179`), and the firmware asked Diego how the carriers wire it.
  Unwired or high, a staged rescue would boot an A500 from the slot half. That works only if
  the A500 can reach the replies at `$E00000`, and nobody has checked whether its Gary
  decodes that range as ROM. A pull-down on GPIO 25 in the firmware would keep the A500 on
  SRAM.
- **Which images v4.0.0 publishes for the Amiga.** One option is the 1 MB image alone,
  `RESCUE_SWITCHER_v4.0.0_1024KB.img`, the name the firmware accepted. The other is that
  image beside today's 512 KB one. That depends on whether the RP2350A firmware accepts a
  1 MB rescue and serves its first half (D-08: one image per computer for both boards).

Depends on firmware EPIC-17 (option 1 staging and the 512 KB rescue cap lifted on the RP2350B)
and on the owner record. It comes after EPIC-05, so the 1 MB image carries a reset that does
not read the ROM.
