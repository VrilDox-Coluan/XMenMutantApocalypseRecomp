# X-Men: Mutant Apocalypse â€” Native Static Recompilation

> [!WARNING]
> ### âš ï¸  Legal Disclaimer: Bring Your Own Dump
> **No game data is in this repository. No ROM, no assets, no proprietary data. Bring your own dump** (*X-Men - Mutant Apocalypse (USA).sfc*, 2 MiB headerless, SHA-256 `65fe17fd6b297f52df6ce9812ecb02c3bb1bfda3ebc05a19c4a8decbf9a446ae`, CRC32 `5e34822a`).
> The original game assets (graphics, sound, palettes, music sequences) are loaded directly from your own legally obtained ROM file at runtime. This repository contains only reverse-engineered runner code, recompilation tooling, and static translation scaffolding.

> [!NOTE]
> ### ðŸ›‘ Project Status: Archived / Research Foundation (Up for Grabs)
> **This project will NOT be actively maintained or updated by the author.**
> It is released strictly as an educational proof-of-concept and open research foundation for the reverse engineering, decompilation, and emulation communities. The project is fully functional up through active gameplay at 60 FPS and is **up for grabs** for anyone interested in forking, refining AOT coverage, implementing true widescreen viewport expansions, or adding custom modding support.

---

A native PC port of Capcom's 1994 Super Nintendo classic **X-Men: Mutant Apocalypse**, created via static binary recompilation (`snesrecomp`).

This project translates the original SNES 65816 machine code into native C11, running alongside an SDL3-based hardware runner with cycle-accurate SNES PPU/APU emulation.

---

## Estimated Progress & Status

| Subsystem | Progress | Status | Notes |
| :--- | :--- | :--- | :--- |
| **Boot & Intro** | `â–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆ` 100% | Complete | Capcom logo, Marvel intro, Title screen & mode select |
| **Audio (SPC700 / DSP)** | `â–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆ` 100% | Complete | Music & sound effects running via native SPC700 core |
| **Input & Savestates** | `â–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆ` 100% | Complete | Keyboard / gamepad mapping, quick save/load, frame step |
| **Character Physics & Combat** | `â–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–‘â–‘` 85% | Functional | Movement, combos, attacks, collision boxes, health/damage |
| **Stage 1 (Genosha / Wolverine)** | `â–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–‘â–‘` 80% | Playable | Wolverine intro stage playable at full 60 FPS |
| **AOT Code Coverage** | `â–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–‘â–‘â–‘â–‘` 65% | Partial | 171 roots mapped (415 variants); scheduler safe in LLE fallback |
| **Adaptive Widescreen** | `â–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–ˆâ–‘â–‘â–‘` 70% | Experimental | Viewport expanded via SDL3; widescreen camera clipping WIP |

---

## Technical Highlights & Engine Architecture

### Cooperative Fiber Scheduler & Stack Frames
*X-Men: Mutant Apocalypse* shares the foundational engine architecture later used in *Marvel Super Heroes in War of the Gems*. The engine utilizes a cooperative task/fiber scheduler residing primarily in Bank `$C0` (`$C0:0000-$1FFF`):
- To prevent fiber context desynchronization and stack frame corruption during dynamic indirect dispatches, scheduler routines are safely routed through the LLE interpreter fallback via `runner/wotg_interp_targets.txt`.
- Safe Tier-2 promotion compiles **171 roots** into **415 native AOT functions**, allowing the core gameplay loop, rendering pipelines, physics, and enemy logic to run directly on the host CPU.

### Adaptive Widescreen
The runner includes custom frame preparation logic (`XmenPrepareFrame`) in `runner/desktop_main.c` supporting adaptive aspect ratio scaling:
- Expands horizontal viewport coordinates beyond the original 256x224 SNES canvas.
- Future work: Patch camera registers and sprite boundary clipping logic to extend rendering for full 16:9 / 21:9 support.

---

## Prerequisites

### Windows
- [CMake](https://cmake.org/download/) (v3.20 or newer)
- [Ninja](https://github.com/ninja-build/ninja/releases)
- GCC / MinGW-w64 (GCC 14.2+ recommended) or Clang *(e.g., via WinLibs or MSYS2)*

### Linux (Ubuntu / Debian / Fedora / Arch)
- CMake, Ninja, GCC / Clang
- SDL3 development libraries
```bash
sudo apt-get install build-essential cmake ninja-build libsdl3-dev
```

---

## Building

### Quick Build (Windows PowerShell)
```powershell
.\build.ps1
```

### Quick Build (Linux / macOS)
```bash
sh build.sh
```

### Manual CMake Build
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Binaries will be output to `build/`:
- `build/xmen_desktop.exe` (Interactive desktop application with SDL3 window)
- `build/xmen_headless.exe` (Headless test host for parity verification)

---

## Running the Game

### ROM Requirement
Provide a legally obtained, headerless dump of **X-Men: Mutant Apocalypse (USA)**:
- **File**: `X-Men - Mutant Apocalypse (USA).sfc`
- **Size**: 2,097,152 bytes (2 MiB)
- **CRC32**: `5e34822a`
- **SHA-256**: `65fe17fd6b297f52df6ce9812ecb02c3bb1bfda3ebc05a19c4a8decbf9a446ae`

Place the ROM file in the project root or pass it via command line:
```bash
# Windows
.\build\xmen_desktop.exe "path\to\X-Men - Mutant Apocalypse (USA).sfc"

# Linux
./build/xmen_desktop "path/to/X-Men - Mutant Apocalypse (USA).sfc"
```

### Controls (Default Keyboard Mapping)

| SNES Button | PC Keyboard |
| :--- | :--- |
| **D-Pad Up / Down / Left / Right** | Arrow Keys / `W`, `S`, `A`, `D` |
| **B (Jump)** | `Z` / `J` |
| **Y (Attack)** | `X` / `K` |
| **A (Special Attack)** | `C` / `L` |
| **X (Special 2)** | `V` / `I` |
| **L / R** | `A` / `S` |
| **Start** | `Enter` |
| **Select** | `Right Shift` |

### Hotkeys

| Hotkey | Action |
| :--- | :--- |
| **`F1`** | Quick Save |
| **`F3`** | Quick Load |
| **`F5`** | Reset Game |
| **`P`** | Pause / Unpause |
| **`K`** | Step 1 Frame (when paused) |

---

## Project Structure

```text
XMenMutantApocalypseRecomp/
â”œâ”€â”€ config/              # Function signatures, safe promotion sets, and bank definitions
â”‚   â”œâ”€â”€ funcs.h          # Function declarations for AOT code
â”‚   â””â”€â”€ xmen_tier2_safe.json # 171 roots promoted to AOT
â”œâ”€â”€ generated/           # Recompiled C sources from original 65816 machine code
â”œâ”€â”€ runner/              # Custom game runner, lifecycle hooks, and widescreen config
â”‚   â”œâ”€â”€ desktop_main.c   # Desktop entry point and aspect ratio logic
â”‚   â”œâ”€â”€ wotg_game.c      # Shared Capcom engine lifecycle and environment init
â”‚   â””â”€â”€ wotg_interp_targets.txt # Scheduler targets safely delegated to interpreter
â”œâ”€â”€ snesrecomp/          # Vendored snesrecomp runtime, PPU/APU driver, and framework
â”œâ”€â”€ tools/               # Analysis and tape generation utilities
â”œâ”€â”€ CMakeLists.txt       # Unified CMake build configuration
â”œâ”€â”€ build.ps1            # Windows one-click build script
â”œâ”€â”€ build.sh             # Linux one-click build script
â””â”€â”€ README.md
```

---

## Opportunities for Community Contribution

- **Character Coverage**: Test and validate full missions for Cyclops, Psylocke, Beast, and Gambit.
- **Enhanced Widescreen**: Reverse-engineer horizontal sprite and tilemap clipping boundaries to eliminate edge pop-in.
- **IPS Patch Support**: Port popular community patches (e.g., *Mutant Apocalypse Rebalanced*, SRAM save patch, language localizations).
- **In-Game Overlay Menu**: Adapt the debug/stage-select menu from *War of the Gems* for mission select and boss rush.

---

## Legal & Disclaimers

1. This project is a reverse engineering and static recompilation experiment developed strictly for educational, preservation, and interoperability purposes.
2. No proprietary Nintendo, Marvel, or Capcom copyrighted code, ROM binaries, graphic artwork, or audio samples are distributed within this repository.
3. Users must own and provide their own legally acquired copy of the original Super Nintendo cartridge.
4. *X-Men*, *Mutant Apocalypse*, and associated character names and likenesses are trademarks and copyright of Marvel Characters, Inc. and Capcom Co., Ltd. This project is not affiliated with, sponsored by, or endorsed by Marvel, Capcom, or Nintendo.

---

## License

The runner and custom reimplementation files are licensed under the MIT / GPL-3.0 License. The `snesrecomp` framework is subject to its respective upstream license (see `snesrecomp/LICENSE` and `snesrecomp/THIRD_PARTY_ATTRIBUTION.md`).
