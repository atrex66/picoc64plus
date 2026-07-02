"""
PETSCII character ROM viewer — displays glyphs from c64_us_upper.bin
Each character is 8×8 pixels. Number of glyphs is derived from file size.
"""

import sys
import pygame

SCALE      = 2
COLS       = 16           # glyphs per row
PADDING    = 4            # px gap between glyphs (unscaled)
FG_COLOR   = (170, 170, 255)   # C64-ish light blue
BG_COLOR   = (0,   0,   128)   # C64-ish dark blue
GRID_COLOR = (40,  40,  100)
CHAR_W = CHAR_H = 8
ROM_PATH = "c64_us_upper.bin"

def load_rom(path):
    with open(path, "rb") as f:
        data = f.read()
    if len(data) % 8 != 0:
        raise ValueError(f"ROM size {len(data)} is not a multiple of 8")
    return data

def make_glyph(rom, code, fg, bg, scale):
    """Return a (8*scale) × (8*scale) Surface for glyph `code`."""
    surf = pygame.Surface((CHAR_W * scale, CHAR_H * scale))
    surf.fill(bg)
    for row in range(CHAR_H):
        byte = rom[code * 8 + row]
        for col in range(CHAR_W):
            if (byte >> (7 - col)) & 1:
                pygame.draw.rect(surf, fg,
                    (col * scale, row * scale, scale, scale))
    return surf

# ASCII → PETSCII screen code mapping for printable characters.
# C64 uppercase charset: A-Z = $01-$1A, 0-9 = $30-$39, space = $20,
# punctuation maps directly for $21-$3F.
def _ascii_to_screen_code(ch):
    c = ord(ch)
    if 0x41 <= c <= 0x5A:   return c - 0x40        # A-Z → 1-26
    if 0x61 <= c <= 0x7A:   return c - 0x60        # a-z → 1-26 (uppercase ROM)
    if 0x20 <= c <= 0x3F:   return c               # space, digits, punctuation
    return 0x3F                                     # '?' for unmapped chars

def glyph_print(surface, glyphs, text, x, y, scale=None):
    """
    Blit `text` onto `surface` at pixel position (x, y) using the glyph atlas.

    Each character advances x by CHAR_W * scale pixels.
    Supports any Python format string — just pass the already-formatted result:

        glyph_print(screen, glyphs, f"PI={3.14159:.3f}", 10, 20)
        glyph_print(screen, glyphs, f"GPIO={val:08b}", 10, 40)
    """
    if scale is None:
        scale = SCALE
    cx = x
    glyph_w = CHAR_W * scale
    for ch in text:
        code = _ascii_to_screen_code(ch)
        if code < len(glyphs):
            surface.blit(glyphs[code], (cx, y))
        cx += glyph_w

def main():
    rom = load_rom(ROM_PATH)
    num_chars = len(rom) // 8
    rows = (num_chars + COLS - 1) // COLS

    cell_w = (CHAR_W + PADDING) * SCALE
    cell_h = (CHAR_H + PADDING) * SCALE
    win_w  = COLS * cell_w + PADDING * SCALE
    win_h  = rows * cell_h + PADDING * SCALE

    pygame.init()
    screen = pygame.display.set_mode((win_w, win_h))
    pygame.display.set_caption(f"PETSCII ROM — {num_chars} chars — c64_us_upper.bin")
    sysfont = pygame.font.SysFont("monospace", 10)

    # Build glyph atlas once — one Surface per character code
    glyphs = [make_glyph(rom, code, FG_COLOR, BG_COLOR, SCALE)
              for code in range(num_chars)]

    screen.fill(BG_COLOR)
    for code in range(num_chars):
        col, row = code % COLS, code // COLS
        x = PADDING * SCALE + col * cell_w
        y = PADDING * SCALE + row * cell_h
        screen.blit(glyphs[code], (x, y))
        label = sysfont.render(f"{code:02X}", True, GRID_COLOR)
        screen.blit(label, (x, y + CHAR_H * SCALE))

    pygame.display.flip()

    # Example — print a formatted string with PETSCII glyphs:
    import math
    glyph_print(screen, glyphs, f"PI={math.pi:.4f}  HEX={255:08X}", 8, win_h - CHAR_H * SCALE - 6)
    pygame.display.flip()

    while True:
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT or (
               ev.type == pygame.KEYDOWN and ev.key == pygame.K_ESCAPE):
                pygame.quit()
                sys.exit()

if __name__ == "__main__":
    main()