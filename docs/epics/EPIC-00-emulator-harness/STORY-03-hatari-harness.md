---
id: STORY-03
epic: EPIC-00
title: The Hatari harness
status: done
---

## Goal

`tools/dev/hatari_harness.py` boots the debug+test `st` image on an ST and the `ste` image on an
STE and a Mega STE, as the system ROM, in colour and in monochrome. It drives the chooser with
keys, reads the trace, and ends with PASS or FAIL and the evidence.

## Tasks

- [x] `tools/dev/dev.cfg` (git-ignored) with a committed `dev.cfg.example`: the paths to Hatari
      and FS-UAE. No third-party ROM is needed, since the images under test are our own builds.
      The harness refuses to start without what it needs, and says what.
- [x] Launch: `--tos <image>`, `--machine st` for the 192 KB image, `ste` and `megaste` for the
      256 KB one, `--memsize 0 --ttram 0` (D-07), `--natfeats yes`, `--monitor rgb` or `mono`,
      `--control-socket` served by the harness, `--screenshot-dir` and `--log-file` in a run
      folder, `--sound off`, SDL's dummy video and audio drivers, `--confirm-quit false`,
      `--fast-forward yes --fast-forward-key-repeat false`; output read through a
      pseudo-terminal.
- [x] Hatari boots our image as TOS, unpatched: the checksum line says `ok`. Record what Hatari
      logs about a TOS it does not know.
- [x] Keys through the control socket with the scancodes of `src/common/kbd.h`: `keydown`,
      wait for the trace line the key must produce, `keyup`, pause 0.2 s (not one `keypress`
      event: see the notes). Each key is sent after `@@ waitkey` and resent once if its line
      never arrives, with the retries counted.
- [x] The scripted session: boot, checksum `ok`, `@@ list` with `test.c`'s entry count and its
      default and rescue marks, page right and left, cursor down, RETURN, ESC at the
      confirmation (`@@ select cancelled`), RETURN and confirm (`@@ select`, then what STORY-01
      decided). A screenshot per screen and on every failure, a time limit per step, Hatari
      killed on expiry, and a check that no Hatari process is left.
- [x] Negative run: a copy of the image with one byte flipped shows the checksum FAIL screen with
      both values, `@@ romcheck fail`, and reaches the list after a key.
- [x] Monochrome: the same session on `--monitor mono` passes, and its screenshot shows 640x400
      text. This checks the mono render path only. The power-on race of the monitor-detect line
      is not emulated (C-03).
- [x] The run time of each session recorded here.

## Acceptance

`python3 tools/dev/hatari_harness.py --image st`, `--image ste --machine megaste` and
`--image st --monitor mono` print PASS on this Mac from a fresh debug+test build, in under a
minute each. A deliberate break, such as a change to the list printing, prints FAIL with the
screenshot path.

## Notes

**2026-10-09** (`epic-00-emulator-harness`, uncommitted): `tools/dev/hatari_harness.py`, with
`tools/dev/harness_common.py` (configuration, expectations, trace reader, steps, and the
scripted session STORY-04 reuses) and `tools/dev/dev.cfg.example`. Standard library only.

| run | result | steps | key retries | wall time |
| --- | --- | --- | --- | --- |
| `--image st` (ST, colour) | PASS | 19/19 | 0 | 3 to 4 s |
| `--image st --machine megast` | PASS | 19/19 | 0 | 4 s |
| `--image ste` (STE) | PASS | 19/19 | 0 | 4 s |
| `--image ste --machine megaste` | PASS | 19/19 | 0 | 4 s |
| `--image st --monitor mono` | PASS | 19/19 | 0 | 4 s |
| `--image ste --monitor mono` | PASS | 19/19 | 0 | 4 s |
| `--image st --corrupt` | PASS | 19/19 | 0 | 5 s |
| `--image ste --corrupt` | PASS | 19/19 | 0 | 5 s |
| `--image st`, down arrow deliberately broken in `chooser.c` | FAIL at "cursor on entry 1", with the screenshot path | 7/19 | 0 | 10 s |

The session: boot (`@@ rescue v4.0.0 <build> st|ste`), the screen mode the ROM chose
(`@@ screen medium|mono`, a debug trace added to `src/st/screen.c` for this), checksum `ok`, or
`fail` then a key in `--corrupt`, the list with `test.c`'s 33 entries and its default and rescue
indices, protocol 0x40, page 1 of 2, down, right to page 2 (cursor at entry 17), left, RETURN,
ESC at the confirmation, down, RETURN, a key, `@@ select 1`, `@@ reset rom 1`, and the second
boot after the ROM's own reset. Screenshots of the list, page 2 and the confirmation, and one on
every failure. Every expected value is read from `src/common/test.c` and `chooser.c`.

What it took:

- Hatari boots our image as TOS with no warning: its log says nothing about the unknown OS
  version, and the ROM's checksum passes, so nothing was patched.
- A clean `HOME` per run: Hatari otherwise loads the saved configuration, which here mapped a
  GEMDOS drive to a Downloads folder.
- Keys: see STORY-02's notes. A `keypress` event loses its key to an ACIA overrun in the
  ROM's slow debug poll, and so does a press while the previous release is unread.
  Press, wait for the trace, release, pause 0.2 s: no retry in any run above.
- The control socket's path sits in a short `/tmp/hh*` folder: macOS caps a Unix socket path at
  104 bytes, and the run folder's name is longer.
- Screenshots wait 0.3 s first: under fast-forward Hatari skips rendering frames, and the first
  list screenshot showed a half-drawn highlight. `--statusbar false --drive-led false` keep
  Hatari's own overlay out of them.
- Mono: the screenshot shows the list at 640x400 in two colours. This is the render path only;
  the power-on race of the monitor-detect line is not emulated (C-03).

`--interactive` is written (a window, no fast-forward, no socket, the trace still captured)
but not yet tried by eye. Diego, 2026-10-09: leave it out of this epic; the look by eye is
EPIC-01, before the release gate.
