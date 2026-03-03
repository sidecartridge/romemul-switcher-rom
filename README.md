# ROM Emulator Switcher (ROM build)

Firmware-oriented application for the SidecarTridge ROM Emulator families. The
goal is to provide a self-contained ROM binary (no GEMDOS/BIOS/XBIOS calls) that
runs directly on the target machine and toggles ROM banks by writing to the
hardware control registers. The code base targets both Atari ST and Amiga
platforms, sharing the same hardware abstraction layer.

> **Status:** Atari ST/STE builds are active. Amiga build is temporarily
> disabled and returns `not implemented yet`.

## Highlights

- Uses the [`atarist-toolkit-docker`](https://github.com/sidecartridge/atarist-toolkit-docker)
  image so the toolchain lives in a container (`stcmd`).
- Pure C + 68k assembly. No GEMDOS/BIOS/XBIOS calls – everything goes straight
  through hardware registers.
- Minimal on-screen feedback implemented via a custom text renderer using
  the public-domain font8x8 dataset (basic ASCII).

## Repository layout

```
romemul-switcher-rom/
├── LICENSE
├── Makefile              # entry point: proxies to the dockerized toolchain
├── README.md
├── src/
│   ├── common/           # shared hardware abstraction headers
│   └── st/               # Atari ST startup + ROM switcher logic
│       ├── Makefile      # invoked inside the container
│       ├── switcher.c
│       ├── switcher.h
│       └── start.s
└── build/                # generated artefacts (gitignored)
```

## Requirements

1. Docker Desktop (or a compatible container runtime).
2. `stcmd` installed via the
   [`atarist-toolkit-docker` installer](https://github.com/sidecartridge/atarist-toolkit-docker/releases/latest).
3. macOS, Linux, or WSL2 host shell.

## Building

```bash
# Ensure stcmd is on your PATH
which stcmd

# Platform is mandatory: st | ste | amiga
./build.sh st
./build.sh ste

# Optional modes
./build.sh ste debug
./build.sh ste test
./build.sh ste debug test
```

Platform behavior:
- `st`: builds with `ROM_BASE_ADDR_UL=0x00FC0000UL` and `startup_st.s`.
- `ste`: builds with `ROM_BASE_ADDR_UL=0x00E00000UL` and `startup_ste.s`.
- `amiga`: currently exits with `amiga build is not implemented yet`.

Release artefacts are copied to `dist/<platform>/` (only when `test` is not enabled):
- ST:
  - `RSWIT192.PRG`
  - `RESCUE_SWITCHER_v<version>_192KB.img`
- STE:
  - `RSWIT256.PRG`
  - `RESCUE_SWITCHER_v<version>_256KB.img`

The generated `.img` file is padded with zeroes up to the next 4KB boundary.

Notes:
- Internal build outputs remain in `build/st/` with canonical names
  (`ROMSWITC.PRG`, `ROMSWITC.BIN`, maps, and `RESCUE_SWITCHER_v<version>.img`).
- `ROM_BASE_ADDR_UL` has no default in Makefiles and must be passed by the
  calling script (`build.sh` does this automatically).

## Code style checks

The repository includes `.clang-format`, `.clang-tidy`, and
`.clang-tidy-ignore` at the top level. Use the helper targets:

```bash
# Auto-format C headers/sources under src/
make format

# Check formatting only (no edits)
make format-check

# Run clang-tidy checks
make tidy

# Run format-check + clang-tidy
make check
```

All targets are implemented through `scripts/clang-checks.sh`.

## Roadmap

- [x] Bootstrap Atari ST target with C+ASM split and direct register writes.
- [ ] Implement Amiga-specific start-up and video feedback.
- [ ] Finalize keyboard/controller map for ROM selection.
- [ ] Add integration tests that run under Hatari via `stcmd`.

## License

MIT. See [LICENSE](LICENSE).
