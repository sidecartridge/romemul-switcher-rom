---
id: STORY-10
epic: EPIC-03
title: A quick ping at boot
status: todo
---

## Goal

Before it reads the parameters page, the ROM pings the device once. A device that does not
answer is reported at once, in plain words, instead of after the long read timeout that ends in
"Error reading the flash page".

## Tasks

- [ ] One ping with a short timeout before the first flash read, restored as in STORY-05;
      release builds: on no answer, a clear message and a key before the read is tried anyway
- [ ] Test builds read the fake flash and carry on after reporting the ping result
- [ ] `@@ bootping ok|noanswer`; harness: `noanswer` in the emulators, with no added delay
- [ ] The release image in Hatari shows the new message instead of only the read error

## Acceptance

In the emulators the release image reports the missing device at once; the test images carry on.

## Notes
