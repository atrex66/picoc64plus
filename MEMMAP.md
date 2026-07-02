# C64+ Memory-Mapped Peripherals (Pico 2 / RP2350)

The following addresses within the `$D000–$DFFF` I/O area control Pico 2
hardware. They are accessible from BASIC using **POKE** (write) and **PEEK**
(read). All addresses are listed in decimal as well.

---

## GPIO — General Purpose Digital I/O

**Physical pins:** GP0–GP29 (RP2350). Every register is **4 bytes**, little-endian,
32-bit bitmask (bit 0 = GP0, bit 25 = GP25 = onboard LED).

| Register         | Hex    | Dec   | Size  | Description                               |
|------------------|--------|-------|-------|-------------------------------------------|
| GPIO\_STATE      | $D02F  | 53295 | 4 B   | Output state (1 = HIGH, 0 = LOW)          |
| GPIO\_DIRECTION  | $D033  | 53299 | 4 B   | Direction (1 = output, 0 = input)         |
| GPIO\_PULLUP     | $D037  | 53303 | 4 B   | Pull-up enable (1 = pull-up active)       |

### Write order
1. Set the direction (`GPIO_DIRECTION`).
2. Optionally enable pull-ups (`GPIO_PULLUP`).
3. Write the output state (`GPIO_STATE`).

> **Note:** A 32-bit value must be written with 4 POKEs (byte 0 = lowest, byte 3 = highest).

### BASIC example — toggle onboard LED (GP25)

```basic
10 REM GP25 = output  (bit 25 set in direction register)
20 REM 33554432 = 1<<25 = $02000000 -> B0=0, B1=0, B2=0, B3=2
30 POKE 53299,0 : POKE 53300,0 : POKE 53301,0 : POKE 53302,2
40 REM LED ON
50 POKE 53295,0 : POKE 53296,0 : POKE 53297,0 : POKE 53298,2
60 REM LED OFF
70 POKE 53295,0 : POKE 53296,0 : POKE 53297,0 : POKE 53298,0
```

---

## PWM — Pulse Width Modulation

**12 slices, 24 channels (A + B per slice).**  
First select the active slice index (`PWM_SELECT`), then configure its
parameters. The `PWM_ENABLE` bitmask enables slices simultaneously.

| Register          | Hex    | Dec   | Size | Description                                              |
|-------------------|--------|-------|------|----------------------------------------------------------|
| PWM\_ENABLE       | $D040  | 53312 | 1 B  | Slice enable bitmask (bit N = slice N)                   |
| PWM\_SELECT       | $D041  | 53313 | 1 B  | Active slice index (0–11)                                |
| PWM\_WRAP\_LO     | $D042  | 53314 | 1 B  | Counter top value, low byte (default 255 → WRAP=65535)   |
| PWM\_WRAP\_HI     | $D043  | 53315 | 1 B  | Counter top value, high byte                             |
| PWM\_LEVEL\_A\_LO | $D044  | 53316 | 1 B  | Channel A duty cycle, low byte (0 .. WRAP)               |
| PWM\_LEVEL\_A\_HI | $D045  | 53317 | 1 B  | Channel A duty cycle, high byte                          |
| PWM\_LEVEL\_B\_LO | $D046  | 53318 | 1 B  | Channel B duty cycle, low byte                           |
| PWM\_LEVEL\_B\_HI | $D047  | 53319 | 1 B  | Channel B duty cycle, high byte                          |

### GPIO ↔ PWM slice/channel map (RP2350)

| Slice | Channel A (GPIO) | Channel B (GPIO) |
|-------|------------------|------------------|
| 0     | GP0              | GP1              |
| 1     | GP2              | GP3              |
| 2     | GP4              | GP5              |
| 3     | GP6              | GP7              |
| 4     | GP8              | GP9              |
| 5     | GP10             | GP11             |
| 6     | GP12             | GP13             |
| 7     | GP14             | GP15             |
| 8     | GP16             | GP17             |
| 9     | GP18             | GP19             |
| 10    | GP20             | GP21             |
| 11    | GP22             | GP23             |

### BASIC example — 50% PWM on GP0 (slice 0, channel A)

```basic
10 REM Select slice 0
20 POKE 53313, 0
30 REM WRAP = 255 (8-bit resolution)
40 POKE 53314, 255 : POKE 53315, 0
50 REM 50% duty cycle: LEVEL_A = 127
60 POKE 53316, 127 : POKE 53317, 0
70 REM Enable slice 0
80 POKE 53312, 1
```

### BASIC example — servo on GP2 (slice 1, channel A, 50 Hz)

```basic
10 REM 50 Hz PWM: sys_clk=150 MHz, div=64 -> 2343750 ticks/s
20 REM WRAP = 46874 -> 46874/2343750 ~= 20 ms period
30 REM POKE slice select
40 POKE 53313, 1
50 REM WRAP = 46874 = $B72A
60 POKE 53314, 42 : POKE 53315, 183
70 REM 1.5 ms centre pulse: 46874 * 1.5/20 ~= 3515 = $0DBB
80 POKE 53316, 187 : POKE 53317, 13
90 REM Enable slice 1
100 POKE 53312, 2
```

---

## I2C — Inter-Integrated Circuit Bus

**Hardware:** I2C0, SDA = GP4, SCL = GP5.  
Default speed: 100 kHz. Maximum transfer: 8 bytes per transaction.

| Register   | Hex    | Dec         | Size | Description                                        |
|------------|--------|-------------|------|----------------------------------------------------|
| I2C\_ADDR  | $D050  | 53328       | 1 B  | Target device 7-bit I2C address                    |
| I2C\_DATA  | $D051  | 53329–53336 | 8 B  | Data buffer ($D051–$D058), used for read and write |
| I2C\_LEN   | $D059  | 53337       | 1 B  | Number of bytes to transfer/receive (1–8)          |
| I2C\_CTRL  | $D05A  | 53338       | 1 B  | Control: **1** = start write, **2** = start read   |
| I2C\_SPEED | $D05B  | 53339       | 1 B  | Speed: **0** = 100 kHz (standard), **1** = 400 kHz |

### BASIC example — write 1 byte to device 0x3C (60 dec)

```basic
10 REM Device address = 60 (0x3C, e.g. SSD1306 OLED)
20 POKE 53328, 60
30 REM Data byte = 0 (control byte), transfer length = 1
40 POKE 53329, 0
50 POKE 53337, 1
60 REM Start write
70 POKE 53338, 1
```

### BASIC example — read 1 byte from device 0x3C

```basic
10 POKE 53328, 60 : POKE 53337, 1
20 REM Start read
30 POKE 53338, 2
40 REM Retrieve result
50 PRINT PEEK(53329)
```

---

## DMA — Direct Memory Access

**Channel:** 11 (RP2350, reserved for the 6502 emulator).  
DMA copies within Pico 2 SRAM — not inside the emulated C64 64 KB space, but
in the RP2350's physical RAM. Useful for large block copies between areas also
accessible by ARM code (e.g. frame buffers, lookup tables).

| Register          | Hex    | Dec         | Size | Description                                                    |
|-------------------|--------|-------------|------|----------------------------------------------------------------|
| DMA\_CTRL         | $D060  | 53344       | 1 B  | Control: **1** = start transfer, **2** = abort                 |
| DMA\_SIZE         | $D061  | 53345       | 1 B  | Transfer width: **0** = byte, **1** = halfword (2B), **2** = word (4B) |
| DMA\_READ\_ADDR   | $D062  | 53346–53349 | 4 B  | Source address (32-bit, little-endian)                         |
| DMA\_WRITE\_ADDR  | $D066  | 53350–53353 | 4 B  | Destination address (32-bit, little-endian)                    |
| DMA\_COUNT        | $D06A  | 53354–53355 | 2 B  | Number of transfers (1–65535, little-endian)                   |
| DMA\_READ\_INC    | $D06C  | 53356       | 1 B  | **1** = increment source address after each transfer           |
| DMA\_WRITE\_INC   | $D06D  | 53357       | 1 B  | **1** = increment destination address after each transfer      |

> **Note:** DMA transfers are non-blocking — the 6502 emulator continues running
> while the DMA is active. There is currently no completion status flag.

---

## Quick Reference — all addresses in decimal

| Dec   | Hex    | Register             | Peripheral |
|-------|--------|----------------------|------------|
| 53295 | $D02F  | GPIO\_STATE (B0)     | GPIO       |
| 53296 | $D030  | GPIO\_STATE (B1)     | GPIO       |
| 53297 | $D031  | GPIO\_STATE (B2)     | GPIO       |
| 53298 | $D032  | GPIO\_STATE (B3)     | GPIO       |
| 53299 | $D033  | GPIO\_DIRECTION (B0) | GPIO       |
| 53300 | $D034  | GPIO\_DIRECTION (B1) | GPIO       |
| 53301 | $D035  | GPIO\_DIRECTION (B2) | GPIO       |
| 53302 | $D036  | GPIO\_DIRECTION (B3) | GPIO       |
| 53303 | $D037  | GPIO\_PULLUP (B0)    | GPIO       |
| 53304 | $D038  | GPIO\_PULLUP (B1)    | GPIO       |
| 53305 | $D039  | GPIO\_PULLUP (B2)    | GPIO       |
| 53306 | $D03A  | GPIO\_PULLUP (B3)    | GPIO       |
| 53312 | $D040  | PWM\_ENABLE          | PWM        |
| 53313 | $D041  | PWM\_SELECT          | PWM        |
| 53314 | $D042  | PWM\_WRAP\_LO        | PWM        |
| 53315 | $D043  | PWM\_WRAP\_HI        | PWM        |
| 53316 | $D044  | PWM\_LEVEL\_A\_LO    | PWM        |
| 53317 | $D045  | PWM\_LEVEL\_A\_HI    | PWM        |
| 53318 | $D046  | PWM\_LEVEL\_B\_LO    | PWM        |
| 53319 | $D047  | PWM\_LEVEL\_B\_HI    | PWM        |
| 53328 | $D050  | I2C\_ADDR            | I2C        |
| 53329 | $D051  | I2C\_DATA\[0\]       | I2C        |
| 53330 | $D052  | I2C\_DATA\[1\]       | I2C        |
| 53331 | $D053  | I2C\_DATA\[2\]       | I2C        |
| 53332 | $D054  | I2C\_DATA\[3\]       | I2C        |
| 53333 | $D055  | I2C\_DATA\[4\]       | I2C        |
| 53334 | $D056  | I2C\_DATA\[5\]       | I2C        |
| 53335 | $D057  | I2C\_DATA\[6\]       | I2C        |
| 53336 | $D058  | I2C\_DATA\[7\]       | I2C        |
| 53337 | $D059  | I2C\_LEN             | I2C        |
| 53338 | $D05A  | I2C\_CTRL            | I2C        |
| 53339 | $D05B  | I2C\_SPEED           | I2C        |
| 53344 | $D060  | DMA\_CTRL            | DMA        |
| 53345 | $D061  | DMA\_SIZE            | DMA        |
| 53346 | $D062  | DMA\_READ\_ADDR (B0) | DMA        |
| 53347 | $D063  | DMA\_READ\_ADDR (B1) | DMA        |
| 53348 | $D064  | DMA\_READ\_ADDR (B2) | DMA        |
| 53349 | $D065  | DMA\_READ\_ADDR (B3) | DMA        |
| 53350 | $D066  | DMA\_WRITE\_ADDR (B0)| DMA        |
| 53351 | $D067  | DMA\_WRITE\_ADDR (B1)| DMA        |
| 53352 | $D068  | DMA\_WRITE\_ADDR (B2)| DMA        |
| 53353 | $D069  | DMA\_WRITE\_ADDR (B3)| DMA        |
| 53354 | $D06A  | DMA\_COUNT (LO)      | DMA        |
| 53355 | $D06B  | DMA\_COUNT (HI)      | DMA        |
| 53356 | $D06C  | DMA\_READ\_INC       | DMA        |
| 53357 | $D06D  | DMA\_WRITE\_INC      | DMA        |
