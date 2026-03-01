# ROM Emulator Switcher (ROM build)

Firmware-oriented application for the SidecarTridge ROM Emulator families. The
goal is to provide a self-contained ROM binary (no GEMDOS/BIOS/XBIOS calls) that
runs directly on the target machine and toggles ROM banks by writing to the
hardware control registers. The code base targets both Atari ST and Amiga
platforms, sharing the same hardware abstraction layer.

> **Status:** early scaffold. The Atari ST target builds and exercises the bank
> switching flow; the Amiga target is stubbed and will receive code parity once
> the register map is finalized.

## Highlights

- Uses the [`atarist-toolkit-docker`](https://github.com/sidecartridge/atarist-toolkit-docker)
  image so the toolchain lives in a container (`stcmd`).
- Pure C + 68k assembly. No GEMDOS/BIOS/XBIOS calls – everything goes straight
  through hardware registers.
- Plug-in hardware layer (`src/common/romemul_hw.h`) so the actual register map
  or handshake sequence can be adjusted without touching the rest of the code.
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
│       ├── rom_switcher.c
│       ├── rom_switcher.h
│       └── start.s
└── build/                # generated artefacts (gitignored)
```

## Requirements

1. Docker Desktop (or a compatible container runtime).
2. `stcmd` installed via the
   [`atarist-toolkit-docker` installer](https://github.com/sidecartridge/atarist-toolkit-docker/releases/latest).
3. macOS, Linux, or WSL2 host shell.

## Building (Atari ST)

```bash
# Ensure stcmd is on your PATH
which stcmd

# Run the top-level make. This will invoke `stcmd make -C src/st`.
make st

# Artefacts
ls build/st
# romemul_switcher.prg  romemul_switcher.bin  romemul_switcher.map
```

`romemul_switcher.prg` keeps the TOS headers so the binary can be executed from
RAM while iterating. `romemul_switcher.bin` is a flat image that can be burned
into ROM.

## Configuring the hardware layer

`src/common/romemul_hw.h` contains placeholder addresses for the ROM Emulator
control registers. Update the base address and handshake bits so they match the
actual hardware (SidecarTridge ROM Emulator, Croissant, etc.). The ST sample
writes the bank index to `ROMEMUL_BANK_SELECT` and polls the ready bit to ensure
that bus latched before continuing.

## Roadmap

- [x] Bootstrap Atari ST target with C+ASM split and direct register writes.
- [ ] Implement Amiga-specific start-up and video feedback.
- [ ] Finalize keyboard/controller map for ROM selection.
- [ ] Add integration tests that run under Hatari via `stcmd`.

## License

MIT. See [LICENSE](LICENSE).
