---
id: STORY-01
epic: EPIC-01
title: Hatari, by eye (Diego's checkpoint)
status: done
---

## Goal

Every screen of the ST and STE images looks right on every machine Hatari offers us, typed on
with the real keyboard.

## Tasks

- [x] `tools/dev/build.sh st debug-test && python3 tools/dev/hatari_harness.py --image st --interactive`:
      the checksum screen with the build line, the list, both pages, the confirmation, ESC, a
      selection and the reset (Diego)
- [x] The same with `--monitor mono`: 640x400, crisp, nothing cut off (Diego)
- [x] `--image ste --interactive` on an STE (Diego)
- [x] `--image ste --machine megaste --interactive` on a Mega STE (Diego)
- [x] `--corrupt --interactive`: the checksum-fail screen shows both values and the build line,
      and a key reaches the list (Diego)

## Acceptance

Diego agrees with what he saw on each run; anything wrong becomes a candidate in
`ITERATIONS.md` or a fix before EPIC-02.

## Notes

**2026-10-09, `--image st --interactive`** (Diego: "yes, it looked right"), images of `0d827d3`,
run `tools/dev/logs/20261009-131255-hatari-st-st-rgb-interactive`, 54 trace lines: boot,
checksum `ok`, the list (33 entries, protocol 0x40), down five times, RIGHT to page 2, down,
RIGHT on the last page (no change, as designed), LEFT back to page 1 (cursor to entry 0, as
designed), down five times, RETURN, the confirmation of entry 5 (`etos-1.3.0-192us.img`), a key,
`@@ reset rom 5`, the second boot, then ESC on the list (a redraw, as designed). Every key
pressed shows in the trace. ESC at the confirmation, the cancel path, was not exercised in
this run; one of the remaining runs covers it.

**2026-10-09, `--image st --monitor mono --interactive`** (Diego: "yes, it looked right"), run
`tools/dev/logs/20261009-131504-hatari-st-st-mono-interactive`, 125 trace lines: `@@ screen
mono`, checksum `ok`, the list, 45 down presses, RIGHT three times and LEFT once (5 page draws),
RETURN on entry 6 (`KAOS142.img`), **ESC at the confirmation: `@@ select cancelled`**, RETURN
again, a key, `@@ select 6`, `@@ reset rom 6`, and the second boot, again in mono. The cancel
path missing from the colour run is covered here.

**2026-10-09, `--image ste --interactive`** (Diego: "yes, it looked right"), run
`tools/dev/logs/20261009-131720-hatari-ste-ste-rgb-interactive`, 83 trace lines: `@@ rescue
... ste`, `@@ screen medium`, checksum `ok`, the list, 26 down presses, RIGHT and LEFT (5 page
draws), RETURN on entry 6, ESC at the confirmation (`@@ select cancelled`), RETURN, a key,
`@@ select 6`, `@@ reset rom 6`, the second boot. The STE and Mega STE task was split in two on
this day, so each machine gets its own verdict.

**2026-10-09, `--image ste --machine megaste --interactive`** (Diego: "yes, it looked right"),
run `tools/dev/logs/20261009-131942-hatari-ste-megaste-rgb-interactive`, 93 trace lines: boot,
checksum `ok`, the list, 27 down presses, RIGHT and LEFT twice (5 page draws), RETURN on entry 6,
a key, `@@ select 6`, `@@ reset rom 6`, the second boot on the Mega STE, then RETURN and ESC at
the confirmation twice (`@@ select cancelled` both times).

**2026-10-09, `--image st --corrupt --interactive`** (Diego: "yes, it looked right"), run
`tools/dev/logs/20261009-132211-hatari-st-st-rgb-corrupt-interactive`: `@@ romcheck fail
stored=AC251845 computed=AC261745`, `@@ waitkey`, a key, then the list drawn and waiting at
entry 0.

**Done 2026-10-09.** Every Hatari machine and mode seen by Diego on the images of `0d827d3`:
ST colour and mono, STE, Mega STE, and the checksum-fail screen; selections and resets on each,
and the cancel at the confirmation on mono, STE and Mega STE.
