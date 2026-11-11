---
id: STORY-03
epic: EPIC-06
title: The A19 line in the self-test
status: blocked
---

## Goal

The self-test's address line adds A19 on the Amiga:
- `ok` when the two halves read their own markers;
- `stuck` when the slot is staged but both addresses read the same marker;
- `skipped` when the firmware says the slot does not hold the rescue's upper half;
- `n/a` when the machine shows no ROM at `$E00000`, which is what a 512 KB carrier (A500,
  A2000) gives.

## Tasks

- [ ] Read the firmware's "the slot holds the rescue image's half" from where the owner record
  lives: a parameters-page word, or a sector after the slot. The offset and values come from the
  firmware session with its commit hash. Mirror them in `src/common/selftest.c` and in
  `src/common/test.c`'s parameters page (D-03).
- [ ] Tell a 1 MB carrier from a 512 KB one by what `$E00000` holds, not by the machine model.
  GPIO 25 is tied to GND on the 512 KB carriers (A500, A2000). With the half staged, the
  firmware sees exactly what it would see with A19 stuck low on a 1 MB carrier. A Gayle probe
  is not enough, because the 1 MB carriers include machines without one: the A600, plus "CDTV /
  A500+ / other" (Diego, 2026-11-11; the exact list is still to come).
  - After A1 to A18 (`selftestAddress()`), check whether `$E00000` holds this image. Compare
    the address-test signatures, which are fixed words outside the marker and the checksums,
    with the same offsets at `$F80000`.
  - If they don't match, the machine puts no ROM there (`$E00000` is not decoded, or it reads
    open bus), so the socket cannot be a 1 MB carrier: A19 `n/a`.
  - If they match, the machine decodes `$E00000` into this socket. Then compare the marker
    word at each address:
    - low at `$E00000` and high at `$F80000`: `ok`;
    - the record says staged and both markers match: `stuck` (low or high);
    - the record says not staged (one half on both): `skipped`;
    - anything else: `wrong`.
- [ ] Two checks, on hardware or from documentation, before relying on this:
  - reading `$E00000` is harmless on every supported Amiga (an A500 or A2000 with Gary, an
    A500+, a CDTV, an A600);
  - no machine that decodes `$E00000` into the Kickstart socket uses a 512 KB carrier. If one
    did, its staged half would read as "stuck low", and that machine would need the record
    or the carrier to say so.

  Ask the firmware session for Diego's exact list of machines per carrier.

  `rom_size` stays 512 KB, so A1 to A18, `rom_check` and the stress reads are unchanged.
- [ ] The screen says "A1-A19" when A19 passes, "A1-A18, A19 skipped" when the half is not
  staged, and "A1-A18" alone when it is `n/a`, within 80 columns (EPIC-03 STORY-12).
- [ ] `@@ selftest addr A19 ok|stuck|skipped|n/a|wrong` in debug builds, and the harness reads it.
- [ ] A stuck A19 in the harness (`--stuck-line 19` writes the low marker into the high half)
  reports `stuck`.

## Acceptance

In FS-UAE (STORY-04):
- the 1 MB image on a model that maps both halves says A19 `ok`;
- with the high marker overwritten, it says `stuck`;
- a 1 MB file of two low halves (the board's mirror) says `skipped` when the record is clear;
- the 512 KB image on the A500 model, with nothing at `$E00000`, says `n/a`.

## Notes

Blocked until the firmware fixes where the owner record lives (firmware EPIC-03 STORY-01, D-13).
Its first proposal, bit 0x4 of `romflags` at byte 532, is on hold.

The v4.0.0 self-test tests only bits 0x1 and 0x2 of `romflags` (`src/common/selftest.c:864-893`),
so a new bit or word there does not disturb it.
