---
id: STORY-01
epic: EPIC-00
title: Builds the harness can trust
status: done
---

## Goal

Any image, release, debug, test or debug+test, builds out of tree from pinned tools, with a
build ID, without touching `build/` or `dist/`. Every object is rebuilt when anything it depends
on changes. The release path in `./build.sh` uses the same tools and the same finalization.
Follows sidecartos-config's `tools/dev/build.sh` and `toolchain.sh` (its EPIC-00 STORY-01).

## Tasks

- [x] `tools/dev/toolchain.sh`, sourced by every build script: the pinned images and the build
      ID. The Atari GCC image `logronoide/atarist-toolkit-docker-x86_64:1.4.0`, which builds
      all three images, is the tag sidecartos-config pins. A `romtool` image of our own,
      `tools/dev/docker/romtool/`, holds amitools 0.8.1 on `python:3.12-slim` pinned by digest:
      amitools is what sidecartos-config pins, while this Mac has 0.4.0 from Homebrew and CI
      takes whatever pip offers that day. The build ID helper gives `<sha7>`, then
      `-dirty.<diff7>` when `src/`, the makefiles or `version.txt` differ from HEAD, then
      `+debug` and `+test`. It is computed on the host and passed to make.
- [x] One finalizer for every build: `scripts/finalize_rom.py <st|ste|amiga> <raw> <map> <out>`
      does exact size, random fill and our checksum, and on the Amiga the kickety split, the
      `romtool` Kickstart checksum and `romtool info`. It is moved out of `build.sh`'s inline
      Python, not rewritten. Checked against the old code on the same raw image: every byte
      outside the random fill equal, and both checksum fields valid.
- [x] `tools/dev/build.sh <st|ste|amiga> <release|debug|test|debug-test>`: runs the pinned image
      with `docker run` at `/work`, empties `tools/dev/builds/<platform>-<type>/` inside the
      container first, passes `BUILD_DIR` and `BUILD_ID` to make, then finalizes. It never
      touches `build/` or `dist/`. `tools/dev/.gitignore` covers `builds/`, `logs/` and
      `dev.cfg`.
- [x] Makefiles rebuild what changed. The build-config stamp gains the version and the build
      ID, and the compiler writes header dependencies (`-MMD -MP`). Found 2026-10-09: the Amiga
      image pinned by the firmware shows "v3.1.0" in its title, because the Amiga `text.c.o`
      dated from March and nothing told make that `version.txt` had changed. Nor does a header
      change rebuild its users, except the few dependencies listed by hand.
- [x] Both makefiles take `BUILD_ID` and pass `-DBUILD_ID_STR`, defaulting to `unknown`.
- [x] `./build.sh` stays the release build. It uses the pinned image, the build ID and the
      shared finalizer, and refuses `debug` and `test` with a pointer to `tools/dev/build.sh`.
      Both flaws go with them: a debug build published to `dist/` over the image the firmware
      pinned (C-06), and a test build after a release build re-finalized a stale image and
      failed with "payload overlaps the checksum field".
- [x] `README.md` and `CLAUDE.md` updated: the build commands, the gotchas, and the pinned
      tools.

## Acceptance

- `tools/dev/build.sh` builds all four types for `st`, `ste` and `amiga`, and leaves `build/`
  and `dist/` byte-identical (SHA-256 before and after).
- Editing `version.txt` or a header rebuilds the objects that use it.
- A release build through `./build.sh` in a scratch copy of the tree gives images whose
  payload equals the clean baseline, apart from the build ID string.

## Notes

Swapped with the ROM groundwork story on 2026-10-09, before any task was checked: the
groundwork needs finalized debug+test images, which need this story's builds first.

Baseline release payloads, clean `release/v4.0.0` (`ded4ba0`) built with `make` in a scratch
worktree on 2026-10-09: `st` 15,708 bytes, `ste` 15,708 bytes, `amiga` payload ending at
`0x3E88`.

The CI workflows still install amitools unpinned with pip; once `build.sh` runs `romtool` from
the pinned image, that step does nothing. Removing it is CI work, out of this epic's scope.

**Done 2026-10-09** (`epic-00-emulator-harness`, on `ded4ba0`, uncommitted):

- `tools/dev/toolchain.sh`, `tools/dev/build.sh`, `tools/dev/romtool.sh`,
  `tools/dev/docker/romtool/Dockerfile`, `tools/dev/.gitignore`, `scripts/finalize_rom.py`;
  `build.sh` rewritten as the release path; both makefiles; `README.md`, `CLAUDE.md`.
- The `latest` and `1.4.0` Atari images both carry GCC 4.6.4 (MiNT 20251014) and binutils
  2.30, so pinning `1.4.0` changes no code: the release payloads built with it equal the
  baseline byte for byte (`st` and `ste` 15,708 bytes; `amiga` up to `0x3E88`, plus the kickety
  split and the footer after the checksums). Checked against that baseline and by recomputing
  both checksum fields, rather than by running the old inline Python on the same raw image:
  our checksum recomputed in Python for all three images, the Kickstart one reported `ok` by
  both the pinned amitools 0.8.1 and the host's 0.4.0.
- The matrix, all twelve built and finalized, with `dist/` and `build/` unchanged (SHA-256 of
  `dist/`, no file in `build/` newer than the run's start):

| image | release | debug | test | debug-test |
| --- | --- | --- | --- | --- |
| `st` payload / free | 15,708 / 180,896 | 21,816 / 174,788 | 146,732 / 49,872 | 152,740 / 43,864 |
| `ste` payload / free | 15,708 / 246,432 | 21,816 / 240,324 | 146,732 / 115,408 | 152,740 / 109,400 |
| `amiga` payload / free before the split | 16,008 / 246,136 | 21,860 / 240,284 | 147,032 / 115,112 | 152,784 / 109,360 |

- `./build.sh all` run in a scratch copy of the tree (the real `dist/` is pinned, C-06):
  exit 0, the five published files under their canonical names, all three images titled
  "Rescue Switcher - v4.0.0".
- Rebuilds, checked with a direct `make` into `build/amiga`: the new stamp forced a full
  rebuild, after which the title read v4.0.0 instead of v3.1.0; a second `make` compiled
  nothing; touching `text.h` recompiled exactly the four objects that include it.
- Found on the way: Docker Desktop's file sharing hid a file the host had just written into a
  folder a container had recreated, three runs out of three ("No such file or directory" in the
  next container). The finalizer therefore runs in the romtool container, not on the host.
- `./build.sh st debug` now prints the usage and a pointer to `tools/dev/build.sh` and exits 1.
