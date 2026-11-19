# Developer Handover & Reverse-Engineering Notes
**Project:** X-Men: Mutant Apocalypse — Native Static Recompilation  
**Author Status:** Archived / Up for Grabs  
**Target:** SNES 65816 HiROM FastROM (16 Mbit, No SRAM)

---

## 1. Engine Architecture & Capcom Fiber Scheduler

*X-Men: Mutant Apocalypse* (1994) is the direct predecessor to *Marvel Super Heroes in War of the Gems* (1996), sharing Capcom's 65816 cooperative task/fiber architecture.

### Cooperative Fiber/Task Scheduler
The task scheduler is located in Bank `$C0` (`$C0:0000`–`$C0:1FFF`):
- **Stack Swapping**: The game assigns discrete stack frames to different engine actors (Wolverine, Sentinel drones, background hazards, HUD) and context-switches between them via `TSC; STA dp,X` and `LDA dp,X; TCS`.
- **AOT Trap**: When compiled directly into C via `snesrecomp`, standard C call stacks cannot tolerate arbitrary stack pointer rewrites across function boundaries. Naive promotion of scheduler routines leads to stack frame corruption or infinite loops (`interp_cap` at `$C0:00A8`).
- **Solution**: The critical scheduler entry points (`0xC000EB`, `0xC000FD`, `0xC00112`, `0xC00159`, etc.) must remain in the LLE interpreter fallback via [`runner/wotg_interp_targets.txt`](../runner/wotg_interp_targets.txt).

### The Open-Bus Anti-Piracy Check (`$C1:6912`)
- Upon entering Stage 1 (Genosha / Wolverine) at approximately frame 4,437, the game executes:
  `LDA $306000,X` → `INC A` → `STA $306000,X` → `CMP $306000,X` (`BF 00 60 30 ... 9F 00 60 30 DF 00 60 30`).
- This reads unmapped SRAM space (`$6000-$7FFF` with `DB=$30`). If unhandled in `snesrecomp/runner/src/cpu_state.c`, it crashes with `[off-rails-romptr] addr=$30602D`.
- **Fix Applied**: `cpu_read8` in `cpu_state.c` handles `(bank & 0x7f) < 0x40 && addr >= 0x6000 && addr < 0x8000` as `cpu->open_bus` for `CART_HIROM` when `ramSize == 0`. This fix unblocks gameplay through 8,000+ frames.

---

## 2. Bank Layout: Code vs. Data

Profiling instruction densities (`REP`/`SEP` opcodes) revealed that *X-Men: Mutant Apocalypse* concentrates its executable code in only **4 primary banks**:

| Bank | REP/SEP Count | Role & Contents |
| :--- | :--- | :--- |
| **`$C0`** | 701 | Task scheduler, input handler (`$C0:0502`), password system (`$C0:08F6`, `$C0:44BA`), all in-game ASCII dialogue (`$C0:8E26+`) |
| **`$C1`** | 801 | Stage logic, enemy object state machines, open-bus check (`$C1:6912`) |
| **`$C2`** | 1302 | Core character physics, Wolverine combat moves, hitboxes, collision detection |
| **`$C7`** | 459 | Auxiliary stage scripts and bonus missions |
| **`$D6`** | 0   | Balance tables, damage multipliers, enemy HP arrays |

*Note: The remaining banks contain compressed tile graphics, Mode 7 maps, palettes, and SPC700 audio samples.*

---

## 3. Adaptive Widescreen Implementation

The runner includes custom frame preparation logic (`XmenPrepareFrame`) in `runner/desktop_main.c`:
- Calls `SnesDisplayAspect_ComputeAdaptiveFrame` to widen the presentation canvas to 16:9 (`342×224`, `extra = 43`px padding on left and right).
- **Three Capcom Engine Widescreen Limits to Address**:
  1. **Tilemap streaming**: Column updates stop ~16–32px past the original 256px boundary.
  2. **Object culling**: Enemies crossing the original right edge despawn early.
  3. **HUD anchoring**: OAM HUD elements remain anchored to the original `x = 0` coordinate (`+extra` pixels inset).
- **Next Step**: Patch sprite culling boundaries in Bank `$C2` and extend background column buffer streaming.

---

## 4. Input & Controls Layout

- Input polling occurs in Bank `$C0:0502` (`LDA $4218; TAX; AND ...`).
- Keyboard controls (default):
  - D-Pad: Arrow keys
  - Jump: **`Z`**
  - Attack / Claw Slash: **`X`**
  - Special Attack: **`C`**
  - Start: **`Enter`**
  - Select: **`Right Shift`**
- In-game debug hotkeys:
  - **`P`**: Pause / unpause
  - **`K`**: Single-frame step advance (when paused)
  - **`F1`** / **`F3`**: Instant savestate save & load

---

## 5. Roadmap for Whoever Continues

1. **Additional Character Stages**:
   - Currently, Stage 1 (Genosha / Wolverine) is fully verified at 60 FPS.
   - Next targets: Cyclops (Genosha Rails), Psylocke (Jungle), Gambit (Harbor), and Beast (Citadel).
2. **True 16:9 Clipping Patch**:
   - Widen camera limits in Bank `$C1` and object culling tests in Bank `$C2`.
3. **Cross-Engine Knowledge**:
   - Refer to the sister project [WarOfTheGemsRecomp](https://github.com/VrilDox-Coluan/WarOfTheGemsRecomp) for in-game launcher menu implementation (`M` key overlay) and savestate thumbnail browser (`F7`).

---

## 6. Community Credits & Acknowledgments

Special recognition to the ROM hackers and researchers whose work helped map the Capcom SNES engine:
- **BlastoBR**: Author of *X-Men: Mutant Apocalypse - Rebalanced*.
- **BillyTime! Games**: Author of *War of the Gems Redux* and extensive Capcom SNES engine research.
- **spinda**: Author of `snesrecomp`.
