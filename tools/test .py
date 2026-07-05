
sprite_height = 5
sprite_pos_y = 1

print(f"sprite height: {sprite_height}, sprite pos y: {sprite_pos_y}")

# azt kell elérni hogy 8 bottom-al kezdődjön, majd 9 top-al, majd 10 bottom-al, majd 11 top-al, majd 12 bottom-al, majd 13 top-al, majd 14 bottom-al, majd 15 top-al, majd 16 bottom-al, majd 17 top-al, majd 18 bottom-al, majd 19 top-al, majd 20 bottom-al, majd 21 top-al
even = sprite_pos_y % 2

for i in range(0, sprite_height):
    if even:
        j = int((i-even) / 2 + sprite_pos_y)
        k = even - i % 2
    else:
        j = int((i-even) / 2 + sprite_pos_y)
        k = i % 2 - even
    print(f"screen row:{j} {'top' if k == 0 else 'bottom'}")