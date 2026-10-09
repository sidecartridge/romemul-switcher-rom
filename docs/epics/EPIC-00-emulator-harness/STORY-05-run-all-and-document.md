---
id: STORY-05
epic: EPIC-00
title: Run every image, measure, document
status: todo
---

## Goal

One command builds and runs every image in its emulator, the sizes are on record, and the next
person, or the next agent, can use the harness from the documentation alone.

## Tasks

- [ ] `tools/dev/all_harness.sh`: builds the three debug+test images and runs `st` in colour
      and in mono, `ste` on an STE and a Mega STE, and `amiga`. One line per run, PASS only if
      every run passes.
- [ ] Sizes for each image, release and debug+test: the payload, and the free ROM space before
      the checksum field (ST, STE) or before the kickety split (Amiga).
- [ ] `tools/dev/README.md`: setup, `dev.cfg`, every script and flag, what a PASS proves and
      what it does not (C-03), and the traps found in STORY-03 and STORY-04.
- [ ] `CLAUDE.md`: the harness commands under Common commands, and the rule that every code
      story runs them (D-06).

## Acceptance

`tools/dev/all_harness.sh` prints one PASS line per run and an overall PASS on this Mac. A reader
of `tools/dev/README.md` can set up and run it without this epic.

## Notes
