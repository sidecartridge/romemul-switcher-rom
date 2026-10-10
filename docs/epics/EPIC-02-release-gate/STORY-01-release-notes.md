---
id: STORY-01
epic: EPIC-02
title: Release notes
status: done
---

## Goal

`CHANGELOG.md` says what v4.0.0 is, since `release.yml` publishes its top section (up to the
first `---`) as the release body.

## Tasks

- [x] A `v4.0.0` section above `v3.1.0`: protocol 0x40 (firmware v4.0.0), the build line on the
      checksum screen, and the fixes chosen into v4.0.0
- [x] `version.txt` is `v4.0.0` and every image's title says it (`tools/dev/build.sh` release)
- [x] The upgrade path, in the release notes and in `README.md`'s "How it works": the firmware's
      update UF2 now replaces the firmware only (firmware `ff57d6c`, D-22 amended there), so a
      board updated from v3.1.0 keeps its v3.1.0 rescue ROM, which stops at the
      incompatibility screen under firmware v4 (protocol 0x31 against 0x40). Copy
      `RESCUE_SWITCHER_v4.0.0_<192KB|256KB|512KB>.img` into `ROMEMUL` and name it in
      `RESCUE.TXT`, worded as the firmware README's "Updating a board" says it

## Acceptance

The section reads well as a GitHub release body; Diego agrees with it.

## Notes

**2026-10-10.** `CHANGELOG.md` has a v4.0.0 section (52 lines up to the first `---`): the
firmware it needs, the upgrade from v3.1.0, what is new (the self-test, the soak, the boot
ping, the boot line), what is fixed (the mono display, the nonce, 80 columns) and the builds.
The date is 2026-10-10; it changes if the tag comes later. `README.md`'s "How it works" now
says to write the image's name in `RESCUE.TXT` (it said to rename the file) and explains the
upgrade, pointing at the firmware README's "Updating a board".

The release builds of `bff73ec` (`tools/dev/build.sh <platform> release`, not `dist/`): all three
named `RESCUE_SWITCHER_v4.0.0_<size>KB.img`, the "Rescue Switcher - v4.0.0" title in each, build
ID `bff73ec`.

Diego, 2026-10-10: "ok to the notes".

**Done 2026-10-10.**
