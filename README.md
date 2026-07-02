# C64 minimal emulator on Raspberry Pico 2

Program the Pico peripherals like in the 80's

The emulator do not try to emulate the real hardware there is no VIC2 or CIA chips emulation
the 6502 speed with a Pico is ~3.8 MOPS on a standard Pico2 it is ~4.01 MOPS

The Basic extension currently needs to be enabled by SYS 49152

You can find the precompiled binary .uf2 file for Pico and Pico2 under the releases.

---

## Features

- Full **MOS 6502 CPU** emulation (all standard opcodes)
- **KERNAL & BASIC ROM** emulation from embedded binary
- **Custom BASIC tokens** — new commands and functions for GPIO, PWM, I2C and DMA
- **Memory-mapped hardware registers** — same hardware also accessible via `PEEK`/`POKE`
- **Host terminal** — Recommended to use PUTTY for maximum compatibility (full color mode)
- Runs entirely from **SRAM** (`copy_to_ram`) for maximum speed
- To activate the basic extension need to run the commnad SYS 49152

---

## Hardware

| Component | Details |
|-----------|---------|
| MCU | Raspberry Pi Pico 2 (RP2350) |
| I/O | USB CDC serial — use the included SDL2 terminal or PuTTY |
| I2C | GP4 (SDA) / GP5 (SCL), pull-ups enabled automatically |

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

An SDL2 app on Linux that renders the full C64 display and sends keystrokes back over serial.

```bash
cd host_side
./build.sh          # checks deps, generates font.h, compiles
./c64terminal /dev/ttyACM0        # 2× scale (default)
./c64terminal /dev/ttyACM0 3      # 3× scale
```

Press **ESC** to quit. All standard C64 keys are mapped including function keys and cursor keys.

---

## Custom BASIC Commands

The BASIC extension is assembled from `CustomBasicCommands/` and linked into the firmware as `src/basicext.h`.

Build the extension (requires KickAssembler):

```bash
cd CustomBasicCommands
make
```

### Token map

| Token | Keyword | Description |
|-------|---------|-------------|
| `$DF` | `PINMODE` | Set GPIO pin direction |
| `$E0` | `PINOUT` | Drive GPIO pin high/low |
| `$E1` | `PINPULL` | Enable/disable GPIO pull-up |
| `$E2` | `PWMSEL` | Select active PWM slice |
| `$E3` | `PWMLVL` | Set PWM duty level |
| `$E4` | `PWMWRP` | Set PWM wrap (top) value |
| `$E5` | `PWMON` | Enable PWM slice(s) |
| `$E6` | `PWMOFF` | Disable PWM slice(s) |
| `$E7` | `I2CADR` | Set I2C target address |
| `$E8` | `I2CWRT` | Write bytes over I2C |
| `$E9` | `I2CRDT` | Read bytes over I2C |
| `$EA` | `I2CSPD` | Set I2C speed (0=100 kHz, 1=400 kHz) |
| `$EB` | `DMACPY` | Start DMA transfer |
| `$EC` | `DMASIZE` | Set DMA transfer unit size |
| `$ED` | `DMAINCR` | Set DMA read/write increment mode |
| `$EE` | `PINGET` *(fn)* | Read GPIO pin state (returns 0 or 1) |

### GPIO examples

```basic
REM Blink GP25 (onboard LED)

10 PINMODE 25,1
20 PINOUT 25,1
30 FOR T=1 TO 1500:NEXT T
20 PINOUT 25,0
30 FOR T=1 TO 1500: NEXT T
30 GOTO 10

REM Read a button on GP15 with pull-up
PINMODE 15,0 : PINPULL 15,1
PRINT PINGET(15)
```

### PWM example

```basic
REM 50% duty on GP0 (slice 0, wrap=255)
10 PWMSEL 0 
20 PWMWRP 255 
30 PWMLVL 128,0 
40 PWMON 0
```

### I2C example

```basic
REM Write 1 byte to SSD1306 OLED (address $3C = 60)
10 I2CADR 60
20 POKE 53329,0
30 I2CWRT 1
```

### DMA example

```basic
REM Copy 256 bytes (set src/dst addresses via POKE to $D062/$D066 first)
DMASIZE 0
DMAINCR 0,1 
REM COPY $0 to the screen memory
DMACPY 0,1024,1000
```

---

## Memory-Mapped Hardware Registers

The same hardware is also accessible via `PEEK`/`POKE` for machine-code programs.

### GPIO — `$D02F–$D03A`

| Address | Size | Register | Description |
|---------|------|----------|-------------|
| `$D02F` | 4 B | `GPIO_STATE` | Pin output state (1 bit per pin) |
| `$D033` | 4 B | `GPIO_DIRECTION` | Pin direction: `1` = output |
| `$D037` | 4 B | `GPIO_PULLUP` | Pull-up enable: `1` = enabled |

### PWM — `$D040–$D047`

| Address | Register | Description |
|---------|----------|-------------|
| `$D040` | `PWM_ENABLE` | Slice enable bitmask |
| `$D041` | `PWM_SELECT` | Active slice index (0–11) |
| `$D042–$D043` | `PWM_WRAP` | Counter top, lo/hi byte |
| `$D044–$D045` | `PWM_LEVEL_A` | Channel A duty, lo/hi byte |
| `$D046–$D047` | `PWM_LEVEL_B` | Channel B duty, lo/hi byte |

### I2C — `$D050–$D05B`

| Address | Register | Description |
|---------|----------|-------------|
| `$D050` | `I2C_ADDR` | 7-bit target address |
| `$D051–$D058` | `I2C_DATA` | 8-byte data buffer |
| `$D059` | `I2C_LEN` | Byte count (1–8) |
| `$D05A` | `I2C_CTRL` | `$01`=write, `$02`=read; status: `$80`=busy, `$40`=error |
| `$D05B` | `I2C_SPEED` | `0`=100 kHz, `1`=400 kHz |

### DMA — `$D060–$D06D`

| Address | Register | Description |
|---------|----------|-------------|
| `$D060` | `DMA_CTRL` | `$01`=start, `$02`=abort; status: `$80`=busy |
| `$D061` | `DMA_SIZE` | Transfer size: `0`=byte, `1`=halfword, `2`=word |
| `$D062–$D065` | `DMA_READ_ADDR` | Source address (RP2350 physical), 32-bit LE |
| `$D066–$D069` | `DMA_WRITE_ADDR` | Destination address, 32-bit LE |
| `$D06A–$D06B` | `DMA_COUNT` | Transfer count, 16-bit LE |
| `$D06C` | `DMA_READ_INC` | `1` = increment source after each transfer |
| `$D06D` | `DMA_WRITE_INC` | `1` = increment destination after each transfer |

---

## Tools

| Script | Purpose |
|--------|---------|
| `tools/prgtoheader.py` | Convert `.prg` to C `uint8_t[]` header |
| `tools/terminal.py` | Simple serial terminal client |
| `tools/c64_palette.py` | Generate C64 palette data |
| `tools/dma_gen.py` | DMA setup code generator |
| `tools/gpio_gen.py` | GPIO setup code generator |
| `host_side/extract_font.py` | Extract CHARROM bitmap from `c64-roms.h` → `font.h` |

---

> ROM files (`c64-roms.h`) are copyrighted by their respective owners and are not included — supply your own legally obtained copies.

