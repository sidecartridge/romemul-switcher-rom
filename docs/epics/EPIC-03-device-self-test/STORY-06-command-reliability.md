---
id: STORY-06
epic: EPIC-03
title: Command reliability
status: todo
---

## Goal

When the device answers, the self-test sends many pings in a row and reports how many were
answered, timed out or failed their checksum: a measure of the bus losses firmware EPIC-06
found under TOS, here with interrupts masked.

## Tasks

- [ ] N pings (N decided here, long enough to see a 0.6 % loss rate), each restored as in
      STORY-05; counts of answered, timeouts and checksum errors (`0xFFFFFFFD`)
- [ ] Skipped, and said so, when STORY-05's ping got no answer
- [ ] `@@ selftest reliability` line; harness: "skipped" in the emulators (C-03)

## Acceptance

In the emulators: skipped cleanly. On the bench, both boards: the counts (STORY-11).

## Notes
