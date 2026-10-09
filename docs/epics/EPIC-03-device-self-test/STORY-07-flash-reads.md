---
id: STORY-07
epic: EPIC-03
title: Flash read consistency
status: todo
---

## Goal

When the device answers, the self-test reads the parameters page and the catalog several times
through the bus and reports whether every read agrees and how many block retries they needed.

## Tasks

- [ ] A retry counter in `commands.c` for `read_flash_block()`, readable by the self-test
- [ ] The parameters page and the catalog pages read K times (K decided here), compared byte for
      byte with the first read; differences and retries reported
- [ ] Skipped when the ping got no answer; `@@ selftest flash` line; harness: "skipped" in the
      emulators

## Acceptance

In the emulators: skipped cleanly. On the bench: identical reads, the retry count (STORY-11).

## Notes
