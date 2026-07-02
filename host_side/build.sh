#!/usr/bin/env bash
# build.sh — check dependencies, generate font.h, then build c64terminal
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

RED='\033[0;31m'
GRN='\033[0;32m'
YEL='\033[1;33m'
NC='\033[0m'

ok()   { echo -e "${GRN}[OK]${NC}  $*"; }
warn() { echo -e "${YEL}[!!]${NC}  $*"; }
fail() { echo -e "${RED}[FAIL]${NC} $*"; exit 1; }

echo "─────────────────────────────────────────"
echo " C64+ Terminal — build check"
echo "─────────────────────────────────────────"

# ── 1. gcc ────────────────────────────────────────────────────────────────────
if command -v gcc &>/dev/null; then
    ok "gcc found: $(gcc --version | head -1)"
else
    fail "gcc not found — install with: sudo apt install gcc"
fi

# ── 2. SDL2 dev headers ───────────────────────────────────────────────────────
if command -v sdl2-config &>/dev/null; then
    ok "SDL2 found: $(sdl2-config --version)"
else
    warn "sdl2-config not found — attempting install..."
    sudo apt install -y libsdl2-dev || fail "Could not install libsdl2-dev"
    ok "SDL2 installed: $(sdl2-config --version)"
fi

# ── 3. Python3 ────────────────────────────────────────────────────────────────
if command -v python3 &>/dev/null; then
    ok "python3 found: $(python3 --version)"
else
    fail "python3 not found — install with: sudo apt install python3"
fi

# ── 4. c64-roms.h (needed by extract_font.py) ────────────────────────────────
ROMS_H="$SCRIPT_DIR/../src/c64-roms.h"
if [ -f "$ROMS_H" ]; then
    ok "c64-roms.h found"
else
    fail "c64-roms.h not found at $ROMS_H — needed to extract the PETSCII font"
fi

# ── 5. Generate font.h ────────────────────────────────────────────────────────
if [ -f font.h ]; then
    ok "font.h already exists (delete it and re-run to regenerate)"
else
    echo "Generating font.h from c64-roms.h ..."
    python3 extract_font.py || fail "extract_font.py failed"
    ok "font.h generated"
fi

# ── 6. Build ──────────────────────────────────────────────────────────────────
echo ""
echo "Building c64terminal ..."
make || fail "Build failed"

echo ""
ok "Build complete → ./c64terminal"
echo ""
echo "Usage:"
echo "  ./c64terminal /dev/ttyACM0       (2× scale, default)"
echo "  ./c64terminal /dev/ttyACM0 3     (3× scale)"
