# C64+ — Commodore 64 Emulator on Raspberry Pi Pico 2

A Commodore 64 BASIC emulator running on the **Raspberry Pi Pico 2 (RP2350)**. The RP2350 emulates the 6502 CPU and KERNAL/BASIC ROM, and communicates over USB serial — connect with PuTTY or any serial terminal.

> **Important:** This emulator operates in **software-only mode**. All hardware-dependent C64 functions (raster interrupts, SID, VIC-II registers, hardware timers, etc.) are either stubbed or not implemented. Calling them from BASIC or machine code may **freeze the emulator**.

> **Character display:** Standard serial terminals do not have the PETSCII character set. The emulator maps PETSCII control codes and graphics characters to a hand-picked set of ANSI/UTF-8 characters for approximate display. Some characters will not render correctly.

![Screenshot](docs/Screenshot_2026-06-22_13-42-11.png)

---

## Features

- Full **MOS 6502 CPU** emulation with all standard opcodes
- **KERNAL & BASIC ROM** emulation from embedded binary
- **Memory-mapped hardware peripherals** — GPIO, PWM, I2C and DMA directly accessible from BASIC via `PEEK`/`POKE`
- Runs from **SRAM** (`copy_to_ram`) for maximum performance
- **USB CDC** serial interface — use PuTTY or any serial terminal to interact
- Embedded demo programs: Plasma, Mandelbrot, Supermon64

---

## Hardware

| Component | Details |
|-----------|---------|
| MCU | Raspberry Pi Pico 2 (RP2350, Cortex-M33 @ 150 MHz) |
| I/O | USB CDC serial (PuTTY / any serial terminal) |

---

## Project Structure

```
├── src/              # C source and header files
│   ├── 6502_opcodes.c/h   — 6502 CPU core
│   ├── cart_main.c/h      — Main entry point and hardware init
│   ├── kernal_traps.c/h   — ARM-native KERNAL/BASIC replacements (WIP)
│   ├── memory.h           — Memory map definitions
│   ├── c64-roms.h         — Embedded ROM binaries (BASIC, KERNAL, CHARROM)
│   ├── palette256.h       — C64 colour palette
│   └── terminalninja.h    — Terminal helper
├── programs/         # Demo .prg files and BASIC programs
│   ├── plasma.prg
│   ├── mandelbr8-c64.prg
│   ├── supermon64.prg
│   ├── basictetris.prg
│   └── tetris.bas
├── tools/            # Helper scripts
│   ├── prgtoheader.py     — Convert .prg to C header array
│   ├── terminal.py        — Serial terminal client
│   └── c64_palette.py     — C64 palette generator
├── docs/             # Images and reference material
├── testprogram/      — Native ARM test programs
├── CMakeLists.txt
├── pico_sdk_import.cmake
├── build.sh / build.bat
└── README.md
```

---

## Building

### Prerequisites

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) v2.2.0
- CMake ≥ 3.13
- ARM GCC toolchain (`arm-none-eabi-gcc`)
- Ninja build system

The VS Code **Raspberry Pi Pico** extension handles SDK and toolchain installation automatically.

### Build

```bash
mkdir build && cd build
cmake ..
ninja
```

Or use the provided script:

```bash
./build.sh
```

The output UF2 file is at `build/c64_plus.uf2`.

### Flash

Hold **BOOTSEL** on the Pico while plugging in USB, then copy `c64_plus.uf2` to the mounted drive.  
Or use picotool:

```bash
picotool load build/c64_plus.uf2 -fx
```

---

## Connecting via Serial Terminal

The emulator communicates over **USB CDC serial**. After flashing:

1. Connect the Pico via USB
2. Open PuTTY (or any serial terminal), select the COM port assigned to the Pico, **115200 baud**
3. The C64 BASIC prompt should appear

You can also use the included helper script:

```bash
python tools/terminal.py
```

---

## Tools

| Script | Purpose |
|--------|---------|
| `tools/prgtoheader.py` | Convert a `.prg` file to a C `uint8_t[]` header |
| `tools/terminal.py` | Serial terminal client for USB CDC |
| `tools/c64_palette.py` | Generate C64 colour palette data |

---

## Memory-Mapped Hardware Peripherals

The emulator exposes real Pico 2 hardware directly into the 6502 address space. Programs can control GPIO pins, PWM channels and the I2C bus using plain `POKE`/`PEEK` from BASIC or machine code.

Core1 polls these registers every opcode cycle and applies changes to the hardware with one opcode of latency (dirty-compare, no unnecessary SDK calls).

### GPIO — `$D02B–$D032`

| Address | Size | Register | Description |
|---------|------|----------|-------------|
| `$D02B` | 4 B | `GPIO_DIRECTION` | 1 bit per pin — `1` = output, `0` = input |
| `$D02C` | 4 B | `GPIO_PULLUP` | 1 bit per pin — `1` = pull-up enabled |
| `$D02F` | 4 B | `GPIO_STATE` | Output: set bit to drive pin high; Input: read bit to sample pin |

All 30 usable RP2350 GPIO pins are addressable. Pins used by I2C (`GP4`, `GP5`) or PWM slices are automatically reconfigured by the respective subsystem.

**BASIC example — blink GP25 (onboard LED):**
```basic
10 POKE 53291,1<<25   : REM direction = output
20 POKE 53295,1<<25   : REM state = high
30 FOR T=1 TO 500 : NEXT T
40 POKE 53295,0       : REM state = low
50 FOR T=1 TO 500 : NEXT T
60 GOTO 20
```

---

### PWM — `$D040–$D047`

The RP2350 has 12 PWM slices. Slice N drives GPIO N×2 (channel A) and GPIO N×2+1 (channel B).

| Address | Register | Description |
|---------|----------|-------------|
| `$D040` | `PWM_ENABLE` | Bitmask — bit N enables slice N |
| `$D041` | `PWM_SELECT` | Active slice to configure (0–11) |
| `$D042–$D043` | `PWM_WRAP` | Counter top value, lo/hi byte (default 65535) |
| `$D044–$D045` | `PWM_LEVEL_A` | Channel A duty cycle, lo/hi byte (0 … wrap) |
| `$D046–$D047` | `PWM_LEVEL_B` | Channel B duty cycle, lo/hi byte |

Write to `PWM_SELECT` first to choose the slice, then set wrap and levels, then enable via `PWM_ENABLE`.

**BASIC example — 50% duty cycle on GP0 (slice 0):**
```basic
10 POKE 53313,0   : REM select slice 0
20 POKE 53314,255 : POKE 53315,0  : REM WRAP = 255
30 POKE 53316,128 : POKE 53317,0  : REM LEVEL_A = 128 (50%)
40 POKE 53312,1   : REM enable slice 0
```

---

### I2C — `$D050–$D05B`

Uses **I2C0** on **GP4 (SDA)** / **GP5 (SCL)**. Pull-ups are enabled automatically.

| Address | Register | Description |
|---------|----------|-------------|
| `$D050` | `I2C_ADDR` | 7-bit target device address |
| `$D051–$D058` | `I2C_DATA` | 8-byte data buffer (read or write) |
| `$D059` | `I2C_LEN` | Number of bytes to transfer (1–8) |
| `$D05A` | `I2C_CTRL` | Write: `$01` = send, `$02` = receive; Read: `$80` = busy, `$40` = NACK/error, `$00` = OK |
| `$D05B` | `I2C_SPEED` | `0` = 100 kHz (standard), `1` = 400 kHz (fast) |

Transaction flow: set `I2C_ADDR`, fill `I2C_DATA`, set `I2C_LEN`, then write `$01` or `$02` to `I2C_CTRL` to trigger. Poll `I2C_CTRL` until bit 7 clears.

**BASIC example — write 1 byte to an SSD1306 OLED (`$3C`):**
```basic
10 POKE 53328,60  : REM I2C_ADDR = 0x3C
20 POKE 53329,0   : REM I2C_DATA[0] = 0x00 (command)
30 POKE 53337,1   : REM I2C_LEN = 1
40 POKE 53338,1   : REM I2C_CTRL = write → triggers transfer
50 IF PEEK(53338) AND 128 THEN GOTO 50  : REM wait until done
```

---

### DMA — `$D060–$D06D`

The RP2350 DMA controller runs independently of both CPU cores. Once started, the 6502 can continue executing while the transfer happens in hardware. Channel 11 is reserved for 6502 use.

| Address | Register | Description |
|---------|----------|-------------|
| `$D060` | `DMA_CTRL` | Write: `$01` = start, `$02` = abort; Read: `$80` = busy, `$00` = idle |
| `$D061` | `DMA_SIZE` | Transfer unit: `0` = byte, `1` = halfword (2 B), `2` = word (4 B) |
| `$D062–$D065` | `DMA_READ_ADDR` | Source physical address, 32-bit little-endian |
| `$D066–$D069` | `DMA_WRITE_ADDR` | Destination physical address, 32-bit little-endian |
| `$D06A–$D06B` | `DMA_COUNT` | Number of transfers (1–65535), 16-bit little-endian |
| `$D06C` | `DMA_READ_INC` | `1` = increment source address after each transfer |
| `$D06D` | `DMA_WRITE_INC` | `1` = increment destination address after each transfer |

**Addressing:** Source and destination are **RP2350 physical addresses**. The emulated 6502 `memory[]` array lives in SRAM — its base address can be obtained at startup and stored in a known zero-page location for BASIC programs to use.

Transaction flow: fill `DMA_READ_ADDR`, `DMA_WRITE_ADDR`, `DMA_COUNT`, `DMA_SIZE`, `DMA_READ_INC`, `DMA_WRITE_INC`, then write `$01` to `DMA_CTRL`. The transfer starts immediately on the ARM DMA hardware. Poll `DMA_CTRL` bit 7 to check completion, or fire-and-forget.

**BASIC example — non-blocking 256-byte memory copy:**
```basic
10 REM Fill DMA_READ_ADDR ($D062) with source physical address
20 REM Fill DMA_WRITE_ADDR ($D066) with destination physical address
30 POKE 53354,0   : POKE 53355,1  : REM DMA_COUNT = 256
40 POKE 53345,0                   : REM DMA_SIZE = byte
50 POKE 53356,1                   : REM DMA_READ_INC = 1
60 POKE 53357,1                   : REM DMA_WRITE_INC = 1
70 POKE 53344,1                   : REM DMA_CTRL = start
80 REM 6502 is free to do other work here while DMA runs
90 IF PEEK(53344) AND 128 THEN GOTO 90  : REM optional: wait
```

This project is open source. ROM files (`c64-roms.h`) are copyrighted by their respective owners and are not included in this repository — you must supply your own legally obtained copies.

