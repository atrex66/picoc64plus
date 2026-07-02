import json

# --- 1. C64 base palette (0–15) ---
c64 = [
    (0, 0, 0),         # black
    (255, 255, 255),   # white
    (136, 0, 0),       # red
    (170, 255, 238),   # cyan
    (204, 68, 204),    # purple
    (0, 204, 85),      # green
    (0, 0, 170),       # blue
    (238, 238, 119),   # yellow
    (221, 136, 85),    # orange
    (102, 68, 0),      # brown
    (255, 119, 119),   # light red
    (51, 51, 51),      # dark grey
    (119, 119, 119),   # mid grey
    (170, 255, 102),   # light green
    (0, 136, 255),     # light blue
    (187, 187, 187),   # light grey
]

palette = []

# --- 2. add C64 colors first ---
palette.extend(c64)

# --- 3. RGB cube (6x6x6 = 216 colors) ---
steps = [0, 51, 102, 153, 204, 255]

for r in steps:
    for g in steps:
        for b in steps:
            if len(palette) < 256:
                palette.append((r, g, b))

# --- 4. fill remaining with grayscale gradient (if needed) ---
i = len(palette)
while i < 256:
    v = int((i / 255) * 255)
    palette.append((v, v, v))
    i += 1

# --- 5. output ---
print(f"Palette size: {len(palette)}")

# save as JSON
with open("palette256.json", "w") as f:
    json.dump(palette, f, indent=2)

# optional: C header
with open("palette256.h", "w") as f:
    f.write("static const uint8_t palette256[256][3] = {\n")
    for r, g, b in palette:
        f.write(f"    {{0x{r:02X}, 0x{g:02X}, 0x{b:02X}}},\n")
    f.write("};\n")