import math
import os

name = "sintable.h"

with open(name, "w") as f:
    f.write("#ifndef SINTABLE_H\n")
    f.write("#define SINTABLE_H\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write("static const uint8_t sintab[256] = {\n")

    for i in range(256):
        value = int((math.sin(i * 2 * math.pi / 256) + 1) * 127.5)  # Scale to 0-255
        f.write(f"    0x{value:02X},\n")

    f.write("};\n\n")
    # make a color table
    f.write("static const uint8_t colortab[256] = {\n")
    k = 0
    b = 1
    for i in range(256):
        value = 16 + (k & 0x0F)  # Keep only the lower 4 bits for color
        k+= b
        if k > 15:
            k = 15
            b = -1
        elif k < 0:
            k = 0
            b = 1
        f.write(f"    0x{int(value):02X},\n")
    f.write("};\n\n")
    f.write("#endif // SINTABLE_H\n\n")
