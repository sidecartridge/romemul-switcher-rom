---
id: STORY-04
epic: EPIC-00
title: The FS-UAE harness
status: todo
---

## Goal

`tools/dev/fsuae_harness.py` boots the debug+test `amiga` image as the Kickstart of an A500 in
FS-UAE, drives the chooser through the serial port, reads the trace from it, and ends with PASS
or FAIL and the evidence.

## Tasks

- [ ] A generated `.fs-uae` per run: `amiga_model = A500`, `kickstart_file` the image, 512 KB of
      chip RAM and no slow or fast RAM (D-07), no floppy, `serial_port` a pseudo-terminal the
      harness put in raw mode first, `window_hidden = 1`, `audio_driver = dummy`,
      `sound_output = none`, `logs_dir` the run folder. FS-UAE is stopped with SIGTERM, and the
      harness checks that no FS-UAE process is left.
- [ ] FS-UAE boots our image as Kickstart. Record what it logs about an unknown ROM, and check
      that the `@@ romcheck ok` line arrives.
- [ ] Keys and trace over the serial port (STORY-01). Each key is sent after `@@ waitkey`, then
      waited for by its `@@ key` line, with the retries counted.
- [ ] The same scripted session as STORY-03, ending with the selection as STORY-01 decided.
- [ ] Negative run: a copy of the image with one payload byte flipped, then `romtool` run on it
      so the Kickstart checksum is valid and FS-UAE still boots it. Our own checksum field is
      not updated, so the run must show `@@ romcheck fail` and reach the list after a key.
- [ ] Screenshots: try FS-UAE's `screenshot*` options, still unsolved in sidecartos-config, and
      record what works. Without them a failure carries the trace and FS-UAE's log.
- [ ] `--interactive`: a window and the keyboard, handed over to the user.
- [ ] The run time of a session recorded here.

## Acceptance

`python3 tools/dev/fsuae_harness.py` prints PASS on this Mac from a fresh debug+test build. A
deliberate break prints FAIL with the evidence.

## Notes
