#!/usr/bin/env python3
"""Convert a Snes9x .smv movie file to a raw per-frame uint16 joypad-1 tape."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


def convert_smv_to_tape(smv_path: Path, out_path: Path) -> tuple[int, int]:
    data = smv_path.read_bytes()
    if len(data) < 32 or data[:4] != b"SMV\x1a":
        raise ValueError(f"Invalid SMV header: {smv_path}")

    version, _uid, _rerecords, frames, ctrl_mask = struct.unpack_from("<IIIIB", data, 4)
    savestate_offset, controller_data_offset = struct.unpack_from("<II", data, 0x18)

    # Snes9x .smv v1 stores savestate_offset at 0x18 and controller_data_offset at 0x1C.
    num_controllers = bin(ctrl_mask & 0x1F).count("1")
    if num_controllers < 1:
        raise ValueError(f"No active controllers in SMV (mask=0x{ctrl_mask:02x})")

    stride = num_controllers * 2
    needed = controller_data_offset + frames * stride
    if needed > len(data):
        raise ValueError(
            f"SMV truncated: need {needed} bytes from offset {controller_data_offset}, have {len(data)}"
        )

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as f:
        for i in range(frames):
            smv_word = struct.unpack_from("<H", data, controller_data_offset + i * stride)[0]
            # Convert SNES $4218 bit order (bits 15..4 = B,Y,Sel,Start,U,D,L,R,A,X,L,R)
            # to RtlRunFrame / keybinds.c bit order (bits 0..11).
            rtl_word = 0
            for k in range(12):
                if smv_word & (1 << (15 - k)):
                    rtl_word |= 1 << k
            f.write(struct.pack("<H", rtl_word))

    return frames, version


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert Snes9x .smv to raw uint16 joypad-1 tape.")
    parser.add_argument("smv", type=Path, help="Input .smv file")
    parser.add_argument("output", type=Path, help="Output .tape file")
    args = parser.parse_args()

    frames, version = convert_smv_to_tape(args.smv, args.output)
    print(f"Converted SMV v{version}: {frames} frames -> {args.output} ({args.output.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
