# ROM Emulator Switcher

Bare-metal ROM switcher firmware for the SidecarTridge ROM Emulator family.

This repository builds ROM-resident switcher binaries for:

- Atari ST
- Atari STE
- Amiga 500
- Amiga 2000

The runtime is fully freestanding. It does not call TOS, GEMDOS, BIOS, XBIOS,
Kickstart, or AmigaOS. All platform code talks directly to the hardware.

## How it works

1. Download the ROM image that matches your computer model and ROM size:
   - Atari ST/ Mega ST: `192 KB`
   - Atari STE / Mega STE: `256 KB`
   - Amiga 500/2000: `512 KB`
   - Latest releases: <https://github.com/sidecartridge/romemul-switcher-rom/releases>
2. Copy the downloaded `.img` file into the `ROMEMUL` folder of the device.
3. Rename the `RESCUE.TXT` file so it points to that ROM image name.
4. Safely eject the device from your computer.
5. Enter Rescue Mode and boot the machine.

Once the machine enters Rescue Mode, this ROM will boot and show up as the
rescue ROM/switcher image.

For Rescue Mode usage on the SidecarTridge TOS Emulator, see the [official
guide](https://docs.sidecartridge.com/sidecartridge-tos/user-guideV2/#rescue-mode).

For Rescue Mode usage on the SidecarTridge Kickstart Emulator, see the [official
guide](https://docs.sidecartridge.com/sidecartridge-kickstart/user-guide/#rescue-mode).

## Status

- Atari ST ROM build: active
- Atari STE ROM build: active
- Amiga Kickstart-replacement ROM build: active

## Highlights

- Shared chooser, ROM protocol, allocator, and text rendering in `src/common/`
- Shared full-ROM checksum verification in `src/common/rom_check.c`
- Atari ST/STE hardware-specific code in `src/st/`
- Amiga 500/2000 hardware-specific code in `src/amiga/`
- Direct hardware keyboard handling on both platforms
- `80`-column menu on Amiga using hires `2`-bitplane display (`4` colors)
- Toolchain lives in Docker: the pinned [`atarist-toolkit-docker`](https://github.com/sidecartridge/atarist-toolkit-docker)
  image builds all three images, and a small pinned image runs `romtool` and the image finalizer

## Repository Layout

```text
romemul-switcher-rom/
├── CLAUDE.md
├── LICENSE
├── Makefile
├── README.md
├── build.sh               # release build: publishes to dist/
├── dist/                  # published artifacts
├── docs/epics/            # backlog: iterations, epics, stories, decisions
├── scripts/               # finalize_rom.py, clang-checks.sh
├── src/
│   ├── amiga/             # Amiga 500/2000 platform code
│   ├── common/            # shared chooser/text/protocol code
│   └── st/                # Atari ST/STE platform code
├── tools/dev/             # out-of-tree builds and pinned toolchain images
└── build/                 # intermediate outputs of a direct `make`
```

## Requirements

1. Docker Desktop or compatible container runtime
2. `git` and `bash` on macOS, Linux, or WSL2

The toolchain images are pulled or built on first use: the Atari GCC image
`logronoide/atarist-toolkit-docker-x86_64:1.4.0`, pinned by digest, and
`romemul-switcher-rom/romtool:1` (amitools 0.8.1), built from
`tools/dev/docker/romtool/`. Both are named in `tools/dev/toolchain.sh`.

## Build

Release images, published to `dist/<platform>/`. Platform is mandatory unless you use `all`.

```bash
./build.sh st
./build.sh ste
./build.sh amiga
./build.sh all
```

Debug and test images are built out of tree into `tools/dev/builds/<platform>-<type>/` and
never reach `dist/`:

```bash
tools/dev/build.sh st debug        # -O0 -g, trace on
tools/dev/build.sh ste test        # catalog and parameters from src/common/test.c
tools/dev/build.sh amiga debug-test
tools/dev/build.sh st release      # a release image, without publishing it
```

Every build carries a build ID: the commit, `-dirty.<hash>` when the sources differ from it,
and `+debug` or `+test`.

Platform build parameters:

- `st`
  - `ROM_BASE_ADDR_UL=0x00FC0000UL`
  - startup ROM entry: `startup_st.s`
- `ste`
  - `ROM_BASE_ADDR_UL=0x00E00000UL`
  - startup ROM entry: `startup_ste.s`
- `amiga`
  - `ROM_BASE_ADDR_UL=0x00F80000UL`
  - full `512 KB` Kickstart-replacement ROM image

`build.sh all` builds `st`, `ste`, and `amiga` sequentially.

## Artifacts

Release artifacts are published under `dist/<platform>/` by `./build.sh` only.
Only canonical artifact names are published; timestamped variants are not generated.

### Atari ST

- `dist/st/RSWIT192.PRG`
- `dist/st/RESCUE_SWITCHER_v<version>_192KB.img`

The final ROM image is finalized to exactly `192 KB`, with random filler in
unused ROM space and a 32-bit big-endian checksum in the last 4 bytes.

### Atari STE

- `dist/ste/RSWIT256.PRG`
- `dist/ste/RESCUE_SWITCHER_v<version>_256KB.img`

The final ROM image is finalized to exactly `256 KB`, with random filler in
unused ROM space and a 32-bit big-endian checksum in the last 4 bytes.

### Amiga

- `dist/amiga/RESCUE_SWITCHER_v<version>_512KB.img`

The Amiga image is always emitted as a full `512 KB` Kickstart-style ROM,
including kickety-split/footer/checksum information, and is validated with
`romtool`. Unused ROM space is filled with random data, and a dedicated
32-bit big-endian checksum field is stored immediately before the footer.

## Release Workflow

- GitHub Releases are built from `dist/`
- Only the canonical `.img` files are uploaded:
  - `dist/st/*.img`
  - `dist/ste/*.img`
  - `dist/amiga/*.img`
- AWS/S3 upload is not part of the release workflow

## Platform Notes

### Atari ST / STE

- Internal 8.3 build names are preserved for generated binaries:
  - `ROMSWITC.PRG`
  - `ROMSWITC.BIN`
  - `ROMSWITC.MAP`
- Runtime is fully polled and bare-metal
- Keyboard input uses IKBD ACIA directly
- Screen and glyph rendering are handled in `src/st/`
- The title/model string is selected from the ROM base address:
  - `Atari ST` for `0x00FC0000UL`
  - `Atari STE` for `0x00E00000UL`

### Amiga 500 / 2000

- ROM-emulator protocol is the same as ST, but the ROM window is `0xF80000`
- Magic values use the `F8` prefix
- `startup.s` clears the low-memory ROM overlay before touching RAM
- Runtime code is copied from ROM to RAM during startup
- `.data` is copied to RAM and `.bss` is cleared in startup
- The hard reset stub lives in ROM bootstrap code, not in relocated RAM code
- Current display target is `80` columns in hires mode with `2` bitplanes (`4` colors)

## Shared Runtime Notes

- `src/common/text.c` implements the terminal-style text renderer used by both platforms
- `src/common/chooser.c` contains the catalog parser and menu flow
- `src/common/commands.c` contains the ROM-emulator protocol access
- `src/common/rom_check.c` verifies the full ROM checksum before the chooser starts
- `_TEST` builds use embedded catalog/parameter data from `src/common/test.c`
- `text_run_feature_tests()` is intentionally kept in the tree

At boot, ST/STE/Amiga compute a full-ROM checksum before entering the chooser.
If verification fails, the stored and computed values are shown and the user is
asked to press a key before continuing.

## Code Style Checks

The repository includes `.clang-format`, `.clang-tidy`, and `.clang-tidy-ignore`.

```bash
make format
make format-check
make tidy
make check
```

These targets are implemented through `scripts/clang-checks.sh`.

## Development Notes

- Keep `src/common` platform-agnostic
- Keep hardware details in `src/st` or `src/amiga`
- Do not introduce TOS, GEMDOS, BIOS, XBIOS, Kickstart, or AmigaOS runtime dependencies
- If you change memory layout constants, update startup and linker assumptions together
- `version.txt` may contain a leading `v`; build logic strips it before generating artifact names
- New `.c`, `.h`, and `.s` files should use the standard project file header block
- Preferred validation after any change:
  - `tools/dev/all_harness.sh`: builds the three debug+test images and runs them in Hatari
    and FS-UAE (see `tools/dev/README.md`)
  - `tools/dev/build.sh <platform> release` for each of `st`, `ste` and `amiga`

## License

GNU GPL v3.0. See [LICENSE](LICENSE).
