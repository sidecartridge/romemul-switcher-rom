---
id: STORY-03
epic: EPIC-00
title: The Hatari harness
status: todo
---

## Goal

`tools/dev/hatari_harness.py` boots the debug+test `st` image on an ST and the `ste` image on an
STE and a Mega STE, as the system ROM, in colour and in monochrome. It drives the chooser with
keys, reads the trace, and ends with PASS or FAIL and the evidence.

## Tasks

- [ ] `tools/dev/dev.cfg` (git-ignored) with a committed `dev.cfg.example`: the paths to Hatari
      and FS-UAE. No third-party ROM is needed, since the images under test are our own builds.
      The harness refuses to start without what it needs, and says what.
- [ ] Launch: `--tos <image>`, `--machine st` for the 192 KB image, `ste` and `megaste` for the
      256 KB one, `--memsize 0 --ttram 0` (D-07), `--natfeats yes`, `--monitor rgb` or `mono`,
      `--control-socket` served by the harness, `--screenshot-dir` and `--log-file` in a run
      folder, `--sound off`, SDL's dummy video and audio drivers, `--confirm-quit false`,
      `--fast-forward yes --fast-forward-key-repeat false`; output read through a
      pseudo-terminal.
- [ ] Hatari boots our image as TOS, unpatched: the checksum line says `ok`. Record what Hatari
      logs about a TOS it does not know.
- [ ] Keys through the control socket, one `keypress` event per key, with the scancodes of
      `src/common/kbd.h`. Each key is sent after `@@ waitkey`, then waited for by its `@@ key`
      line and resent once if it never arrives, with the retries counted.
- [ ] The scripted session: boot, checksum `ok`, `@@ list` with `test.c`'s entry count and its
      default and rescue marks, page right and left, cursor down, RETURN, ESC at the
      confirmation (`@@ select cancelled`), RETURN and confirm (`@@ select`, then what STORY-01
      decided). A screenshot per screen and on every failure, a time limit per step, Hatari
      killed on expiry, and a check that no Hatari process is left.
- [ ] Negative run: a copy of the image with one byte flipped shows the checksum FAIL screen with
      both values, `@@ romcheck fail`, and reaches the list after a key.
- [ ] Monochrome: the same session on `--monitor mono` passes, and its screenshot shows 640x400
      text. This checks the mono render path only. The power-on race of the monitor-detect line
      is not emulated (C-03).
- [ ] `--interactive`: the same launch with a window and no socket, handed over to the user.
- [ ] The run time of each session recorded here.

## Acceptance

`python3 tools/dev/hatari_harness.py --image st`, `--image ste --machine megaste` and
`--image st --monitor mono` print PASS on this Mac from a fresh debug+test build, in under a
minute each. A deliberate break, such as a change to the list printing, prints FAIL with the
screenshot path.

## Notes
