# Changelog

## v4.0.0 (2026-10-10)

For the SidecarTridge TOS Emulator and Kickstart Emulator with firmware v4.0.0, on both the
RP2350A and RP2350B boards. It talks protocol 0x40 and works with firmware v4.0.0 only.

### Upgrading from v3.1.0

The firmware's update file replaces the firmware only, so a board updated from v3.1.0 keeps
its v3.1.0 rescue ROM, which stops at its incompatibility screen under firmware v4.0.0. After
updating the firmware:

1. Connect the board to the computer. The `ROMEMUL` volume mounts with its files.
2. Copy the image that matches the computer into `ROMEMUL`:
   `RESCUE_SWITCHER_v4.0.0_192KB.img` for an ST or Mega ST,
   `RESCUE_SWITCHER_v4.0.0_256KB.img` for an STE or Mega STE,
   `RESCUE_SWITCHER_v4.0.0_512KB.img` for an Amiga 500 or 2000.
3. Write its name in `RESCUE.TXT`.
4. Eject the volume. The board rebuilds its ROM list with the new rescue ROM.

The firmware's README, "Updating a board", also covers the SWITCHER and `CONFIG.TXT`.

### New

- A self-test of the cartridge: **T** in the ROM list. It checks every address line and data
  line of the image, thousands of random reads, that the cartridge answers commands (8 pings,
  then 1,000 for reliability), that its flash reads the same every time, and the ROM list and
  settings stored in it. It also names the machine, the monitor line on the Atari, the RAM
  (the ST's two banks, the Amiga's chip and slow RAM) and the rescue image itself.
- **S** on the self-test screen: a 30-second soak of random reads with live counts.
- A ping at boot: a cartridge that does not answer commands is reported at once, with a hint
  to check that it is seated well and runs firmware v4, instead of after a long read timeout.
- The boot screen shows the version, the image size and the build.

### Fixed

- Atari ST and STE on a monochrome monitor: after a cold boot the display could come up
  shifted, the end of every line at its start. The resolution is now set in the vertical
  blank.
- Every command to the cartridge carries a new random number. A single one served the whole
  session before, so a stale reply could pass for a fresh one.
- No line runs past 80 columns any more, so long messages and ROM names no longer overwrite
  the next line.

### Builds

- The toolchain is pinned Docker images, and a clean build of a commit gives the same bytes
  anywhere, the release included.
- The free space of each image holds a computed pattern instead of zeros: the self-test reads
  it back.

---

## v3.1.0 (2026-03-11)
- First version for Atari ST, STE and Amiga 500 and 2000.

---
