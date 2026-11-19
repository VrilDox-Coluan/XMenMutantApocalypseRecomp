#!/usr/bin/env python3
"""Scan a Capcom SNES HiROM (X-Men: Mutant Apocalypse / War of the Gems) for
65816 idioms that affect static recompilation (snesrecomp):
  1. Stack-switching cooperative task schedulers (TCS / TSC)
  2. WAI-based VBlank wait loops
  3. Inline-parameter JSL helpers (reading return addr off stack at entry)
  4. SRAM / open-bus anti-piracy checks ($6000 / $306000)
"""
from __future__ import annotations
import sys
from pathlib import Path


def file_to_hirom(off: int) -> str:
    bank = 0xC0 + (off >> 16)
    addr = off & 0xFFFF
    return f"0x{bank:02X}{addr:04X}"


def scan_rom(rom_path: Path) -> None:
    data = rom_path.read_bytes()
    print(f"=== Scanning {rom_path.name} ({len(data)} bytes) ===")

    # 1. WAI instructions (0xCB) in banks $C0-$CF (or code banks)
    wai_hits = []
    for off in range(len(data)):
        if data[off] == 0xCB:
            # Heuristic: preceded/followed by typical NMI wait (STZ dp / LDA dp / BEQ)
            ctx = data[max(0, off - 6) : off + 6]
            if b"\xf0\xfb" in ctx or b"\xd0\xfb" in ctx or b"\x64" in ctx[:6]:
                wai_hits.append(off)
    print(f"\n[1] WAI wait-loop candidates ({len(wai_hits)}):")
    for off in wai_hits[:20]:
        print(f"  {file_to_hirom(off)}: {data[max(0, off-6):off+8].hex(' ')}")

    # 2. Inline-parameter JSL helpers:
    # Common Capcom patterns right at function entry:
    # - PLY; PLA; PHA; PHY  (7A 68 48 5A)
    # - PLA; STA dp         (68 85 xx)
    # - PLX; PLA; ...       (FA 68 ...)
    # - LDA 1,S / 2,S / 3,S (A3 01 / A3 02 / A3 03)
    inline_patterns = [
        ("PLY;PLA;PHA;PHY", b"\x7a\x68\x48\x5a"),
        ("PLX;PLA", b"\xfa\x68"),
        ("PLY;PLA", b"\x7a\x68"),
        ("PHB;PLY;PLA", b"\x8b\x7a\x68"),
        ("LDA 1,S", b"\xa3\x01"),
    ]
    print("\n[2] Inline-parameter JSL helper candidates:")
    for label, pat in inline_patterns:
        idx = 0
        hits = []
        while True:
            idx = data.find(pat, idx)
            if idx == -1:
                break
            # Verify it's near a JSL target or has RTL (0x6B) nearby within 256 bytes
            if b"\x6b" in data[idx : idx + 180] or b"\x48\x5a" in data[idx : idx + 16]:
                hits.append(idx)
            idx += 1
        print(f"  {label}: {len(hits)} hits -> {[file_to_hirom(h) for h in hits[:12]]}")

    # 3. SRAM / open-bus checks ($6000)
    print("\n[3] SRAM / open-bus $6000 checks:")
    for label, pat in [
        ("LDA $306000,X", b"\xbf\x00\x60\x30"),
        ("STA $306000,X", b"\x9f\x00\x60\x30"),
        ("CMP $6000,X", b"\xdd\x00\x60"),
        ("STA $6000,X", b"\x9d\x00\x60"),
    ]:
        idx = 0
        hits = []
        while True:
            idx = data.find(pat, idx)
            if idx == -1:
                break
            hits.append(file_to_hirom(idx))
            idx += 1
        if hits:
            print(f"  {label}: {hits[:10]}")

    # 4. Cooperative task switchers (REP #$30; LDA dp,X; TCS / TSC)
    print("\n[4] Cooperative stack-switchers (TCS=0x1B / TSC=0x3B with DP indexed):")
    for label, pat in [
        ("REP #$30; LDA dp,X; TCS", b"\xc2\x30\xb5"),
        ("TSC; STA dp,X", b"\x3b\x95"),
    ]:
        idx = 0
        hits = []
        while True:
            idx = data.find(pat, idx)
            if idx == -1:
                break
            if label.startswith("REP") and idx + 4 < len(data) and data[idx + 4] == 0x1B:
                hits.append(file_to_hirom(idx))
            elif label.startswith("TSC"):
                hits.append(file_to_hirom(idx))
            idx += 1
        print(f"  {label}: {len(hits)} hits -> {hits[:15]}")


if __name__ == "__main__":
    for arg in sys.argv[1:]:
        scan_rom(Path(arg))
