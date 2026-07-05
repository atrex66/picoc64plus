import os
import sys

# Usage:
#   python3 prgtoheader.py                            # legacy: main.prg -> main.prg.h
#   python3 prgtoheader.py in.prg out.h ARRAY PREFIX  # explicit args for Makefile use
#
# ARRAY  = name for the uint8_t array  (e.g. BASIC_EXTENSION)
# PREFIX = prefix for the SIZE/START_ADDRESS defines (e.g. BASIC_EXT)

if len(sys.argv) == 5:
    program    = sys.argv[1]
    out_path   = sys.argv[2]
    array_name = sys.argv[3]
    prefix     = sys.argv[4]
else:
    # Legacy behaviour
    program    = "cartstub.prg"
    out_path   = f"{program}.h"
    array_name = program.upper().replace('.', '_') + "_DATA"
    prefix     = program.upper().replace('.', '_')

with open(program, "rb") as f:
    data = f.read()

start_address = data[0] + (data[1] << 8)
program_data = data[2:]

guard = os.path.basename(out_path).upper().replace('.', '_')

header = f"""#ifndef {guard}
#define {guard}
#include <stdint.h>

#define {prefix}_START_ADDRESS 0x{start_address:04X}
#define {prefix}_SIZE {len(program_data)}

static const uint8_t {array_name}[{prefix}_SIZE] = {{
"""

for i in range(0, len(program_data), 16):
    line_data = program_data[i:i+16]
    line = ", ".join(f"0x{b:02X}" for b in line_data)
    header += f"    {line},\n"

header += "};\n\n#endif\n"

with open(out_path, "w") as f:
    f.write(header)

print(f"Written {len(program_data)} bytes from 0x{start_address:04X} -> {out_path}")
