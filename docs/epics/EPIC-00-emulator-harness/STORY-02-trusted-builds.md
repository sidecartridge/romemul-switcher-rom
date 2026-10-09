---
id: STORY-02
epic: EPIC-00
title: Builds the harness can trust
status: todo
---

## Goal

`./build.sh` builds what the harness runs without touching what the firmware has pinned, from a
toolchain image that does not drift.

## Tasks

- [ ] A debug build never publishes to `dist/`. Today `build.sh` publishes whenever `TEST=0`,
      so `./build.sh amiga debug` would copy a debug image over the release image the firmware
      pinned (C-06). This has not happened only because debug builds do not link yet.
- [ ] A test build no longer fails after a release build. The last step finalizes
      `build/st/RESCUE_SWITCHER_v<ver>.img` whenever that file exists, so a test build run after
      a release build fails with "payload overlaps the checksum field" (seen 2026-09-09).
- [ ] Debug+test images land in a fixed place the harness reads (`build/<platform>-debug-test/`
      or a `tools/dev/` folder; decide) and never in `dist/`.
- [ ] Pin the toolchain image. `stcmd` uses `logronoide/atarist-toolkit-docker-x86_64:latest`
      unless `STCMD_IMAGE_TAG` is set, and `latest` differs from `1.4.0` on this Mac. Pin
      `1.4.0`, the tag sidecartos-config pins, in `build.sh`, which both workflows call. Compare the
      release payloads built with each image.
- [ ] `CLAUDE.md`'s gotchas updated: the two `build.sh` flaws gone, the pinned image recorded.

## Acceptance

`./build.sh st`, `st debug`, `st test`, `st debug test` run in any order, each succeeds, and only
the release build changes `dist/`. Checked with the SHA-256 of `dist/` before and after. The
same holds for `ste` and `amiga`.

## Notes

Before this story `dist/` holds the 2026-09-09 images the firmware pinned. A release build here
replaces them, so tell the firmware session before running one (C-06), or build the release
case into a copy of the tree.
