---
id: STORY-01
epic: EPIC-02
title: Release notes
status: todo
---

## Goal

`CHANGELOG.md` says what v4.0.0 is, since `release.yml` publishes its top section (up to the
first `---`) as the release body.

## Tasks

- [ ] A `v4.0.0` section above `v3.1.0`: protocol 0x40 (firmware v4.0.0), the build line on the
      checksum screen, and the fixes chosen into v4.0.0
- [ ] `version.txt` is `v4.0.0` and every image's title says it (`tools/dev/build.sh` release)
- [ ] The upgrade path, in the release notes and in `README.md`'s "How it works": the firmware's
      update UF2 now replaces the firmware only (firmware `ff57d6c`, D-22 amended there), so a
      board updated from v3.1.0 keeps its v3.1.0 rescue ROM, which stops at the
      incompatibility screen under firmware v4 (protocol 0x31 against 0x40). Copy
      `RESCUE_SWITCHER_v4.0.0_<192KB|256KB|512KB>.img` into `ROMEMUL` and name it in
      `RESCUE.TXT`, worded as the firmware README's "Updating a board" says it

## Acceptance

The section reads well as a GitHub release body; Diego agrees with it.

## Notes
