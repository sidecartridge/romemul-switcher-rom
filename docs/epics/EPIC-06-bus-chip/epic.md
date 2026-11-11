---
id: EPIC-06
iteration: 2
title: The Amiga's bus chip, Gary or Gayle, in the self-test
status: done
---

## Goal

The Amiga self-test names the chip that decodes the machine's bus: Gayle, with its ID, on an
A600 (or A1200), and Gary on the A500, A500+, A2000 and CDTV. Diego, 2026-11-11: "i think we
cannot figure if 1MB or 512KB. So we are only going to show Gary or Gayle if possible."

## Scope

- In scope:
  - the Gayle ID check, under a bus-error handler;
  - a "Bus chip" line in the self-test, and its trace;
  - an A600 session in FS-UAE;
  - Diego's check on the bench.
- Out of scope:
  - saying whether the machine takes a 512 KB or a 1 MB Kickstart. The ROM cannot tell (see
    the notes).
  - everything dropped before that: a 1 MB rescue image, an A19 test, a reply channel at
    `$E00000`, a firmware flag for it.
  - the ST and STE.

## Platforms

`amiga` only. The verification covers both boards on an A500 or A2000, and on Diego's A600 with
the RP2350B.

## Stories

- STORY-01: The Gayle check and the self-test line
- STORY-02: The harness
- STORY-03: Verification (Diego's checkpoint)

## Notes

**Why the ROM size cannot be shown.** This epic first set out to show whether the machine takes
a 512 KB or a 1 MB Kickstart, by checking whether `$E00000` reads the rescue image. In FS-UAE's
A500 it did: FS-UAE mirrors the Kickstart at `$E00000` for an A500 (`cs_ksmirror_e0`). The
real machines do the same, by Commodore's A500/A2000 Gary specification, which decodes
`E00000-E7FFFF` as `NROM`, the ROM chip select. A 1 MB machine differs only in whether A19
reaches the socket:
- the A600's 1 MB carrier wires it;
- the A500 and A2000 carriers tie it low;
- the A500+ has a 42-pin socket meant for 1 MB ROMs, which a Gayle check could not see.

The board serves a 512 KB image on both halves, so the ROM sees the same thing everywhere. Only
the firmware could say whether A19 ever goes high. Diego chose to show the bus chip instead.

**History.** EPIC-06 was "The A19 line on the Amiga, and a 1 MB rescue image" (`71cfe89` to
`bd4652c`), then "The ROM size the Amiga supports" (`5b455e0` onwards), and this since
2026-11-11.

**Started 2026-11-11** on `epic-06-bus-chip`, cut from `epic-05-amiga-reset`. Diego: "start epic
06".

**Done 2026-11-11.** Diego's bench check passed with the release images of `f5cb491` (see the verification story). The code reaches `release/v4.0.0` through EPIC-07.
