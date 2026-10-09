---
id: STORY-05
epic: EPIC-00
title: Run every image, measure, document
status: done
---

## Goal

One command builds and runs every image in its emulator, the sizes are on record, and the next
person, or the next agent, can use the harness from the documentation alone.

## Tasks

- [x] `tools/dev/all_harness.sh`: builds the three debug+test images and runs `st` in colour
      and in mono, `ste` on an STE and a Mega STE, and `amiga`. One line per run, PASS only if
      every run passes.
- [x] Sizes for each image, release and debug+test: the payload, and the free ROM space before
      the checksum field (ST, STE) or before the kickety split (Amiga).
- [x] `tools/dev/README.md`: setup, `dev.cfg`, every script and flag, what a PASS proves and
      what it does not (C-03), and the traps found in STORY-03 and STORY-04.
- [x] `CLAUDE.md`: the harness commands under Common commands, and the rule that every code
      story runs them (D-06).

## Acceptance

`tools/dev/all_harness.sh` prints one PASS line per run and an overall PASS on this Mac. A reader
of `tools/dev/README.md` can set up and run it without this epic.

## Notes

**Done 2026-10-09** (`epic-00-emulator-harness`, uncommitted):

- `tools/dev/all_harness.sh` builds the three debug+test images (logs in
  `tools/dev/logs/build-<platform>-debug-test.log`), then runs nine sessions: the five the task
  asks for, plus a Mega ST, the STE in mono, and `--corrupt` on the ST and the Amiga. Two runs
  in a row: all nine PASS, no key retry, about 55 s in all, builds included.
- `tools/dev/measure_builds.sh` prints the table below; free space is before the checksum field
  on the ST and STE, and before the kickety split on the Amiga.

| image | type | payload (bytes) | free (bytes) |
| --- | --- | --- | --- |
| `st` | release | 15,844 | 180,760 |
| `st` | debug-test | 154,004 | 42,600 |
| `ste` | release | 15,844 | 246,296 |
| `ste` | debug-test | 154,004 | 108,136 |
| `amiga` | release | 16,144 | 246,000 |
| `amiga` | debug-test | 154,364 | 107,780 |

  The release payloads before EPIC-00 were 15,708 (ST, STE) and 16,008 (Amiga); the 136 bytes
  are the build ID row on the checksum screen and the formatter's sink. 128 KB of each
  debug-test payload is the fake flash of `test.c`, of which 64 KB (`flashStorageRaw`) nothing
  reads.
- `tools/dev/README.md`: builds, harnesses, flags, the session, what a PASS proves and what it
  does not, configuration, and the traps found.
- `CLAUDE.md`: the commands, the rule that every code change runs `all_harness.sh` (D-06), and
  a gotcha on the `@@` lines the harnesses match. `README.md`: the same validation step.
