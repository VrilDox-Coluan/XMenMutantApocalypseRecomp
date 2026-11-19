#!/usr/bin/env python3
"""Generate a raw uint16 .tape to boot X-Men: Mutant Apocalypse, select Wolverine,
enter Stage 1 (Genosha), and walk right into the first enemy."""
import struct
from pathlib import Path

BTN_B      = 1 << 0
BTN_Y      = 1 << 1
BTN_SELECT = 1 << 2
BTN_START  = 1 << 3
BTN_UP     = 1 << 4
BTN_DOWN   = 1 << 5
BTN_LEFT   = 1 << 6
BTN_RIGHT  = 1 << 7
BTN_A      = 1 << 8

total_frames = 3500
frames = [0] * total_frames

# Pulse START up to frame 1000 to get to Character Select
for f in range(400, 1020, 45):
    for d in range(4):
        frames[f + d] = BTN_START

# At Character Select (~1050-1350), press RIGHT once to move cursor from Cyclops to Wolverine,
# then press START to enter Stage 1.
for f in [1120]:
    for d in range(4):
        frames[f + d] = BTN_RIGHT
for f in range(1340, 1600, 50):
    for d in range(4):
        frames[f + d] = BTN_START

# Once in Stage 1 (~1700+), hold RIGHT, jump (BTN_B) over the canisters, and walk into the enemy!
for f in range(1800, 3000):
    frames[f] = BTN_RIGHT
for f in range(1950, 1980):
    frames[f] = BTN_RIGHT | BTN_B
for f in range(2100, 2130):
    frames[f] = BTN_RIGHT | BTN_B

out = Path(r"d:\RecompProject\XMenMutantApocalypseRecomp\tapes\wolverine_test.tape")
with out.open("wb") as fp:
    for w in frames:
        fp.write(struct.pack("<H", w))
print(f"Wrote {out} ({len(frames)} frames)")
