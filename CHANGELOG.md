# Changelog

## v4.0.0 (2026-10-10, re-issued 2026-11-11)

For the SidecarTridge TOS Emulator and Kickstart Emulator with firmware v4.0.0, on both the RP2350A and RP2350B boards. It talks protocol 0x40 and works with firmware v4.0.0 only.

These images replace the first v4.0.0 images of 2026-10-10. The version and the image names are the same; the build on the boot screen tells them apart (the first images show build `561b13d`).

### Upgrading from the first v4.0.0

1. Connect the board to the computer. The `ROMEMUL` volume mounts with its files.
2. Copy the new image that matches the computer into `ROMEMUL`, over the old one of the same name.
3. `RESCUE.TXT` already names it: leave it as it is.
4. Eject the volume. The board rebuilds its ROM list with the new rescue ROM.

### Upgrading from v3.1.0

The firmware's update file replaces the firmware only, so a board updated from v3.1.0 keeps its v3.1.0 rescue ROM, which stops at its incompatibility screen under firmware v4.0.0. After updating the firmware:

1. Connect the board to the computer. The `ROMEMUL` volume mounts with its files.
2. Copy the image that matches the computer into `ROMEMUL`:
   - `RESCUE_SWITCHER_v4.0.0_192KB.img` for an ST or Mega ST
   - `RESCUE_SWITCHER_v4.0.0_256KB.img` for an STE or Mega STE
   - `RESCUE_SWITCHER_v4.0.0_512KB.img` for an Amiga 500, 500+, 600 or 2000
3. Write its name in `RESCUE.TXT`.
4. Eject the volume. The board rebuilds its ROM list with the new rescue ROM.

The firmware's README, "Updating a board", also covers the SWITCHER and `CONFIG.TXT`.

### New

- A self-test of the cartridge: **T** in the ROM list. It checks every address line and data line of the image, thousands of random reads, that the cartridge answers commands (8 pings, then 1,000 for reliability), that its flash reads the same every time, and the ROM list and settings stored in it. It also names the machine, the monitor line on the Atari, the RAM (the ST's two banks, the Amiga's chip and slow RAM) and the rescue image itself.
- **S** on the self-test screen: a 30-second soak of random reads with live counts.
- A ping at boot: a cartridge that does not answer commands is reported at once, with a hint to check that it is seated well and runs firmware v4, instead of after a long read timeout.
- The boot screen shows the version, the image size and the build.
- New in the re-issue: the Amiga 500+ and the Amiga 600 join the Amiga 500 and 2000, with the same 512 KB image.
- New in the re-issue: on the Amiga the self-test names the chip that decodes the bus, Gayle on an A600 or A1200, Gary on the other models.
- Experimental: Kickstart images larger than 512 KB, up to 1 MB, on the RP2350B board with the firmware's `LARGE_ROMS` setting in `CONFIG.TXT` (`LARGE_ROMS = 1`). They can be tested on an Amiga 500+ or 600. An image above 1 MB, or above 512 KB without `LARGE_ROMS`, is refused, and the board serves the rescue ROM instead.
  - A 1 MB image holds the `$E00000` half first and the `$F80000` half second: the reverse of the merged ROM that Kickstart builders make for burning a 27C800.
  - Known issue: a 1 MB image chosen from the rescue switcher or the SWITCHER runs with whatever upper half the board last stored, until the board is next connected to a computer over USB. Name the image in `DEFAULT.TXT` instead, or connect the board over USB once after choosing it.

### Fixed

- New in the re-issue: the error screens no longer leave the computer hung. After a flash read error, an incompatible firmware or an empty ROM list, the screen now says "Press any key to try again..." and a key starts again from the checksum. It used to say "Press any key to exit..." with nothing to exit to.
- New in the re-issue: a ROM list that was never written (erased flash) shows "No ROMs found" instead of reading past the end of the list.
- New in the re-issue: Atari ST and STE no longer lose keys pressed in quick succession.
- New in the re-issue: Amiga, after a ROM is selected the screen turns red for a second while the cartridge switches ROM, and only then does the computer reset. It used to reset at once, while the cartridge was still serving the old ROM.
- Atari ST and STE on a monochrome monitor: after a cold boot the display could come up shifted, the end of every line at its start. The resolution is now set in the vertical blank.
- Every command to the cartridge carries a new random number. A single one served the whole session before, so a stale reply could pass for a fresh one.
- No line runs past 80 columns any more, so long messages and ROM names no longer overwrite the next line.

### Builds

- The toolchain is pinned Docker images, and a clean build of a commit gives the same bytes anywhere, the release included.
- The free space of each image holds a computed pattern instead of zeros: the self-test reads it back.

---

## v3.1.0 (2026-03-11)
- First version for Atari ST, STE and Amiga 500 and 2000.

---
