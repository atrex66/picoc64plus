#!/usr/bin/env python3
"""
extract_font.py — generate font.h from the C64 character ROM.

The C64 character ROM is 4096 bytes (2 sets × 256 chars × 8 bytes/char).
We only need the first set (uppercase/graphics, bytes 0-2047).

Usage:
    python3 extract_font.py c64_charrom.bin
    python3 extract_font.py           (auto-detects from ../src/c64-roms.h)

Output:
    font.h  — uint8_t CHARROM[256][8] array ready to include in main.c
"""

import sys
import os
import re

OUT = "font.h"

def from_bin(path):
    with open(path, "rb") as f:
        data = f.read()
    # Character ROM starts at byte 0, uppercase set = first 2048 bytes
    return data[:2048]

def from_header(path):
    """Pull the hex array out of dump_c64_charrom_bin[] in c64-roms.h"""
    with open(path) as f:
        src = f.read()
    # find the charrom array
    m = re.search(r'dump_c64_charrom_bin\[\]\s*=\s*\{([^}]+)\}', src, re.DOTALL)
    if not m:
        raise RuntimeError("Could not find dump_c64_charrom_bin in " + path)
    nums = re.findall(r'0x([0-9A-Fa-f]{2})', m.group(1))
    data = bytes(int(h, 16) for h in nums)
    return data[:2048]

def write_font_h(chardata):
    lines = ["#pragma once", "#include <stdint.h>", "",
             "// C64 character ROM — 256 glyphs, 8×8 pixels each (1 bit per pixel).",
             "// Row 0 = top of glyph, bit 7 = leftmost pixel.",
             "static const uint8_t CHARROM[256][8] = {"]
    for ch in range(256):
        off = ch * 8
        row = chardata[off:off+8] if off + 8 <= len(chardata) else b'\x00' * 8
        hex_row = ", ".join(f"0x{b:02X}" for b in row)
        lines.append(f"    {{ {hex_row} }},  // ${ch:02X}  '{chr(ch) if 32<=ch<127 else '?'}'")
    lines += ["};", ""]
    with open(OUT, "w") as f:
        f.write("\n".join(lines))
    print(f"Written {OUT}  ({256} glyphs, {len(chardata)} bytes of ROM)")

def main():
    roms_h = os.path.join(os.path.dirname(__file__), "../src/c64-roms.h")

    if len(sys.argv) >= 2:
        chardata = from_bin(sys.argv[1])
    elif os.path.exists(roms_h):
        print(f"Auto-extracting from {roms_h} ...")
        chardata = from_header(roms_h)
    else:
        print("Usage: python3 extract_font.py [c64_charrom.bin]")
        print("       or place ../src/c64-roms.h in the project")
        sys.exit(1)

    # Pad to 2048 if shorter
    if len(chardata) < 2048:
        chardata = chardata.ljust(2048, b'\x00')

    write_font_h(chardata)

if __name__ == "__main__":
    main()
