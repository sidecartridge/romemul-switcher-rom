---
id: STORY-10
epic: EPIC-03
title: A quick ping at boot
status: done
---

## Goal

Before it reads the parameters page, the ROM pings the device once. A device that does not
answer is reported at once, in plain words, instead of after the long read timeout that ends in
"Error reading the flash page".

## Tasks

- [x] One ping with a short timeout before the first flash read, restored as in STORY-05;
      release builds: on no answer, a clear message and a key before the read is tried anyway
- [x] Test builds read the fake flash and carry on after reporting the ping result
- [x] `@@ bootping ok|noanswer`; harness: `noanswer` in the emulators, with no added delay
- [x] The release image in Hatari shows the new message instead of only the read error

## Acceptance

In the emulators the release image reports the missing device at once; the test images carry on.

## Notes

**Done 2026-10-09.** `chooser_loop()` sends one `command_ping()` right after
`init_rom_address()`, in every build; `@@ bootping ok|noanswer|checksum|norestore`. Release
builds, on no answer: "The SidecarTridge does not answer commands: check that it is seated
well and running its v4 firmware. Press any key to try reading it anyway...", then the reads
as before. Test builds print "Device ping: answered|no answer (test build: the ROM list is
built in)" and carry on.

- Emulators: `@@ bootping noanswer` on every boot; about 0.2 s in FS-UAE (romcheck at 6.7 s,
  boot ping and list at 6.9 s). `all_harness.sh` 14 of 14 PASS, 57 steps.
- The release ST image in Hatari, 2.5 s after boot: the checksum screen, the build line, then
  the new message and its prompt, where before only "Error reading the flash page" came, after
  the read's timeout.
- Fixed on the same screen: the build-line collision found on 2026-10-09 (candidate in
  `ITERATIONS.md`). Both `switcher.c` leave the cursor two rows below the build line, so the
  chooser's "Loading available ROM images..." no longer overwrites it; the screenshot shows
  both lines intact.
- Release payloads: `st` and `ste` 21,768, `amiga` 22,016 bytes.
