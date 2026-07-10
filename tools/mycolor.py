msg = [
    "  AEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEB  ",
    "  G       picoc64+  basic v0.2       G  ",
    "  CEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEED  ",
]

BLACK = 16
WHITE = 31

all_colors = []

for row in msg:
    colors = [0] * len(row)

    i = 0
    while i < len(row):
        if row[i] == 'E':
            start = i
            while i < len(row) and row[i] == 'E':
                i += 1
            n = i - start

            for k in range(n):
                t = 0.0 if n == 1 else 1.0 - abs(2 * k - (n - 1)) / (n - 1)
                colors[start + k] = round(BLACK + t * (WHITE - BLACK))
        else:
            i += 1

    all_colors.extend(colors)

print("MyColor:")
for i in range(0, len(all_colors), 40):
    line = all_colors[i:i + 40]
    print("        .byte " + ",".join(f"${c:02X}" for c in line))