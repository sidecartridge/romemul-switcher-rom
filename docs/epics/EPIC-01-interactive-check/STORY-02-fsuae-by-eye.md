---
id: STORY-02
epic: EPIC-01
title: FS-UAE, by eye (Diego's checkpoint)
status: done
---

## Goal

Every screen of the Amiga image looks right on FS-UAE's A500, typed on with the real keyboard,
which is the CIA path the harness never uses (it types through the serial port).

## Tasks

- [x] `tools/dev/build.sh amiga debug-test && python3 tools/dev/fsuae_harness.py --interactive`:
      the checksum screen with the build line, the list, both pages, the confirmation, ESC, a
      selection and the reset, all with the keyboard (Diego)
- [x] `--corrupt --interactive`: the checksum-fail screen, then any key reaches the list (Diego)

## Acceptance

Diego agrees with what he saw; anything wrong becomes a candidate or a fix before EPIC-02.

## Notes

**2026-10-09, first try** (`20261009-132347-fsuae-amiga-a500-interactive`), stopped by Diego
with Ctrl-C ("how do we enable the keyboard in fs-uae for this test?"). FS-UAE's log showed
why the arrows did nothing: with no joystick connected it maps the keyboard to a joystick in
port 1 (`configuring joystick port 1 (auto)`, "read config for keyboard for amiga": the cursor
keys become joystick directions, right Alt and right Ctrl fire), so they never reach the
Amiga keyboard. `fsuae_harness.py` now sets `joystick_port_1 = none`: the log shows
`configuring joystick port 1 (none)` and no keyboard mapping for the Amiga any more (the arrow
mappings left are FS-UAE's own F12 menu), and the automated session still passes. The
interrupted run saved no trace: Ctrl-C now ends an interactive run of either harness cleanly,
emulator stopped and trace saved.

**2026-10-09, `--interactive`** (Diego: "yes, it looked right"), run
`tools/dev/logs/20261009-132734-fsuae-amiga-a500-interactive`, 103 trace lines, joystick port 1
empty: boot, checksum `ok`, the list, 32 down and 5 up presses, RIGHT and LEFT (5 page draws),
RETURN on entry 5, ESC at the confirmation (`@@ select cancelled`), RETURN on entry 6, a key,
`@@ select 6`, `@@ reset rom 6`, the second boot. All typed on the real keyboard, so the
Amiga's CIA keyboard path works in the debug build, not only the serial one.

**2026-10-09, `--corrupt --interactive`** (Diego: "yes, it looked right"), run
`tools/dev/logs/20261009-133321-fsuae-amiga-a500-corrupt-interactive`: `@@ romcheck fail
stored=EC1484D4 computed=EC149DD4`, `@@ waitkey`, a key on the real keyboard, the list drawn,
then ESC (a redraw, as designed). FS-UAE accepted the copy's recomputed Kickstart checksum;
our own check caught the flipped byte.
