---
id: STORY-04
epic: EPIC-00
title: The FS-UAE harness
status: done
---

## Goal

`tools/dev/fsuae_harness.py` boots the debug+test `amiga` image as the Kickstart of an A500 in
FS-UAE, drives the chooser through the serial port, reads the trace from it, and ends with PASS
or FAIL and the evidence.

## Tasks

- [x] A generated `.fs-uae` per run: `amiga_model = A500`, `kickstart_file` the image, 512 KB of
      chip RAM and no slow or fast RAM (D-07), no floppy, `serial_port` a pseudo-terminal the
      harness put in raw mode first, `window_hidden = 1`, `audio_driver = dummy`,
      `sound_output = none`, `logs_dir` the run folder. FS-UAE is stopped with SIGTERM, and the
      harness checks that no FS-UAE process is left.
- [x] FS-UAE boots our image as Kickstart. Record what it logs about an unknown ROM, and check
      that the `@@ romcheck ok` line arrives.
- [x] Keys and trace over the serial port (STORY-01). Each key is sent after `@@ waitkey`, then
      waited for by its `@@ key` line, with the retries counted.
- [x] The same scripted session as STORY-03, ending with the selection as STORY-01 decided.
- [x] Negative run: a copy of the image with one byte flipped, then the Kickstart checksum
      recomputed so FS-UAE still boots it. Our own checksum field is not updated, so the run
      must show `@@ romcheck fail` and reach the list after a key.
- [x] Screenshots: try FS-UAE's `screenshot*` options, still unsolved in sidecartos-config, and
      record what works. Without them a failure carries the trace and FS-UAE's log.
- [x] The run time of a session recorded here.

## Acceptance

`python3 tools/dev/fsuae_harness.py` prints PASS on this Mac from a fresh debug+test build. A
deliberate break prints FAIL with the evidence.

## Notes

**2026-10-09** (`epic-00-emulator-harness`, uncommitted): `tools/dev/fsuae_harness.py` on the
shared session of `harness_common.py`.

| run | result | steps | key retries | wall time |
| --- | --- | --- | --- | --- |
| default (A500, 512 KB chip, warp mode) | PASS | 18/18 | 0 | 6 s |
| `--no-warp` | PASS | 18/18 | 0 | 34 s |
| `--corrupt` (twice) | PASS | 18/18 | 0 | 6 s |
| down arrow deliberately broken in `chooser.c` | FAIL at "cursor on entry 1" | 6/18 | 0 | not recorded (ends on the 8 s step timeout) |

The session is STORY-03's without the screen-mode step: boot, checksum, the list from
`test.c`, both pages, the confirmation and ESC, a selection, `@@ reset rom 1`, and the second
boot after the ROM's own reset, which in FS-UAE comes back at once (no firmware to wait for,
C-07). Keys are single bytes written to the serial pseudo-terminal; the ROM polls `SERDATR`
itself, so there is no settle delay and no retry was needed.

What it took:

- FS-UAE boots our image as the Kickstart and logs only `KS ROM 7251eee5 (524288 bytes)`; no
  complaint about an unknown ROM.
- `warp_mode = 1` does not disturb the boot or the serial port, and cuts a session from 34 s to
  6 s.
- The `--corrupt` copy first flipped offset `0x1000`, which in the Amiga image is executed code:
  the checksum failed correctly, and then the chooser misbehaved (it left the page routine
  without drawing, twice, then stopped). A bisect with temporary trace lines found it. The
  flip now lands 32 bytes from the end, in random fill our checksum covers on every image. The
  Kickstart checksum of the copy is recomputed in Python (`fix_kickstart_checksum`), which
  reproduces `romtool copy -c` byte for byte and which `romtool info` accepts.
- Screenshots: this FS-UAE has `screenshots_output_dir`, `screenshots_output_prefix` and
  `screenshots_output_mask`, but a screenshot is a user action (a key in its window, hidden
  here) with no trigger from outside. A failure therefore carries the trace and FS-UAE's log.
  If pictures turn out to matter, the debug build could send its two bitplanes over the serial
  port on request and the harness could write the PNG.

`--interactive` is written (a window, the real keyboard, the trace still read) but not yet tried
by eye. Diego, 2026-10-09: leave it out of this epic; the look by eye is EPIC-01, before the
release gate.
