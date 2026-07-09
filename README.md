# C64 minimal emulator on Raspberry Pico 2

Program the Pico peripherals like in the 80's

The emulator do not try to emulate the real hardware there is no other chips emulation.
in theory you can modify the code to run with different rom files eg. simulate other machines
or you can run the emulator on different microcontrollers if you implement the syscalls for
the different microcontroller, and the keyboard input for the terminal.

**The Basic extension autostart now (8k cartridge on $8000)**

**You can load basic programs from your PC trough PUTTY** by copy the code to the clipboard (ctrl+c)
and press shift+ins in the terminal program to paste the basic program trough the terminal,
there is no screen refresh when the program loading, no worry its not freezing just a littlebit slow.

You can find the precompiled binary .uf2 file for Pico and Pico2 under the releases.

** There are some compromises because of the terminal mode ** you need set CAPS-LOCK in ON state
there. 
---

## Features

- Full MOS 6502 CPU emulation (all standard opcodes)
- **SysCall** new opcode, you can call native ARM code from 6502 assembly ($#19)
- KERNAL & BASIC ROM emulation from embedded header file
- **Custom BASIC tokens** — new commands and functions for GPIO, PWM, I2C and DMA
- Memory-mapped hardware registers — same hardware also accessible via `PEEK`/`POKE`
- **Host terminal — Recommended to use PUTTY for maximum compatibility (full color mode) **
- Runs entirely from **SRAM** (`copy_to_ram`) for maximum speed
- support of rendering sprites on the terminal (2pixel on one glyph) max 64 sprites with 255x255 size and 256 colors
- the terminal sprite registers are in the shadow ram from $A000 you can write it but can not read back because it is ROM area

---

## Tools

- Added a simple sprite editor in the 'tools' directory, it generates a c header file and also a basic samlple for load the sprite

## Hardware

|----------------------------------------------------------------------|
| Component | Details                                                  |
|-----------|----------------------------------------------------------|
| MCU       | Raspberry Pi Pico 2 (RP2350)                             |
| I/O       | USB CDC serial — use the included SDL2 terminal or PuTTY |
| I2C       | GP4 (SDA) / GP5 (SCL), pull-ups enabled automatically    |
| DMA       | DMA channel 11 dedicated for the virtual machine         |
|----------------------------------------------------------------------|

---

## Building the Pico firmware

### Prerequisites

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) v2.2.0+
- CMake ≥ 3.13, ARM GCC toolchain, Ninja
- C64 ROM files compiled into `src/c64-roms.h` (not included — supply your own legal copies)

### Build

```bash
mkdir build && cd build
cmake ..
ninja
```

Flash `build/c64_plus.uf2` — hold BOOTSEL and drag-and-drop, or:

```bash
picotool load build/c64_plus.uf2 -fx
```

---

## Host terminal

Start PUTTY (recommended), change to serial port, find your serial port of the pico and start
it is recommended to disable autowrap in the settings

---

## Custom BASIC Commands

The BASIC extension is assembled from `CustomBasicCommands/` and linked into the firmware as `src/basicext.h`.

Build the extension (requires KickAssembler):

```bash
cd CustomBasicCommands
make
```

| Script | Purpose |
|--------|---------|
| `tools/prgtoheader.py` | Convert `.prg` to C `uint8_t[]` header |
| `tools/terminal.py` | Simple serial terminal client |
| `tools/c64_palette.py` | Generate C64 palette data |
| `tools/dma_gen.py` | DMA setup code generator |
| `tools/gpio_gen.py` | GPIO setup code generator |
| `host_side/extract_font.py` | Extract CHARROM bitmap from `c64-roms.h` → `font.h` |

---

the rom files are used is found here:
https://github.com/floooh/chips-test/blob/master/examples/roms/c64-roms.h

