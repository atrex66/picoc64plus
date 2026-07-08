# New BASIC Commands — SYS 49152

## GPIO — General Purpose I/O

Physical pins GP0–GP29. The onboard LED is GP25.

---

### `PINMODE pin, dir`
Set a GPIO pin's direction.

| Argument | Range | Description |
|----------|-------|-------------|
| `pin`    | 0–29  | GPIO pin number |
| `dir`    | 0–1   | `0` = input, `1` = output |

```basic
PINMODE 25, 1        : REM GP25 (LED) as output
PINMODE 10, 0        : REM GP10 as input
```

---

### `PINOUT pin, val`
Drive a GPIO output pin high or low.

| Argument | Range | Description |
|----------|-------|-------------|
| `pin`    | 0–29  | GPIO pin number (must be set as output first) |
| `val`    | 0–1   | `0` = LOW, `1` = HIGH |

```basic
PINOUT 25, 1         : REM LED on
PINOUT 25, 0         : REM LED off
```

---

### `PINPULL pin, val`
Enable or disable the internal pull-up resistor on a GPIO pin.

| Argument | Range | Description |
|----------|-------|-------------|
| `pin`    | 0–29  | GPIO pin number |
| `val`    | 0–1   | `0` = pull-up off, `1` = pull-up on |

```basic
PINPULL 10, 1        : REM enable pull-up on GP10
```

---

### `PINGET(pin)` — *function*
Read the current state of a GPIO pin. Returns `1` if the pin is HIGH,
`0` if LOW.

| Argument | Range | Description |
|----------|-------|-------------|
| `pin`    | 0–29  | GPIO pin number |

```basic
PINMODE 10, 0        : REM set GP10 as input
PINPULL 10, 1        : REM enable pull-up
PRINT PINGET(10)     : REM prints 0 or 1

10 IF PINGET(10) = 0 THEN PRINT "BUTTON PRESSED"
```

---

## PWM — Pulse Width Modulation

The RP2350 has 8 PWM slices, each with two channels (A and B).

| Slice | Channel A | Channel B | Channel A | Channel B |
|-------|-----------|-----------|-----------|-----------|
| 0     | GP0       | GP1       | GP16      | GP17      |
| 1     | GP2       | GP3       | GP18      | GP19      |
| 2     | GP4       | GP5       | GP20      | GP21      |
| 3     | GP6       | GP7       | GP22      | GP23      |
| 4     | GP8       | GP9       | GP24      | GP25      |
| 5     | GP10      | GP11      | GP26      | GP27      |
| 6     | GP12      | GP13      | GP28      | GP29      |
| 7     | GP14      | GP15      | GP30      | GP31      |
--------------------------------------------------------|
---

### `PWMSEL slice`
Select the active PWM slice for subsequent `PWMWRP` and `PWMLVL` commands.

| Argument | Range | Description      |
|----------|-------|------------------|
| `slice`  | 0–7   | PWM slice number |

```basic
PWMSEL 0             : REM configure slice 0 (GP0/GP1)
```

---

### `PWMWRP wrap`
Set the counter wrap (top) value for the selected slice. This controls the
PWM period and resolution.

| Argument | Range    | Description       |
|----------|----------|-------------------|
| `wrap`   | 1–65535  | Counter top value |

```basic
PWMWRP 255           : REM 8-bit resolution (period = 256 counts)
PWMWRP 46874         : REM ~20 ms period at 150 MHz / div 64 (50 Hz servo)
```

---

### `PWMLVL chan, level`
Set the duty-cycle level for channel A (`0`) or B (`1`) of the selected
slice. `level` ranges from `0` (always off) to `wrap` (always on).

| Argument | Range   | Description                      |
|----------|---------|----------------------------------|
| `chan`   | 0–1     | `0` = channel A, `1` = channel B |
| `level`  | 0–65535 | Duty cycle (0 = 0%, wrap = 100%) |

```basic
PWMLVL 0, 127        : REM 50% duty on channel A (wrap=255)
PWMLVL 1, 200        : REM ~78% duty on channel B (wrap=255)
```

---

### `PWMON slice`
Enable a PWM slice (start generating the waveform).

| Argument | Range | Description |
|----------|-------|-------------|
| `slice`  | 0–7   | PWM slice to enable |

```basic
PWMON 0              : REM start PWM on slice 0
```

---

### `PWMOFF slice`
Disable a PWM slice (stop the waveform, output goes LOW).

| Argument | Range | Description |
|----------|-------|-------------|
| `slice`  | 0–7   | PWM slice to disable |

```basic
PWMOFF 0             : REM stop PWM on slice 0
```

#### Full PWM example — 50% duty cycle on GP0

```basic
10 PWMSEL 0
20 PWMWRP 255
30 PWMLVL 0, 127
40 PWMON 0
```

#### Full PWM example — servo on GP2 (slice 1, channel A)

```basic
10 REM 50 Hz: 150 MHz sys_clk, divider 64 -> 2343750 ticks/s
20 REM WRAP = 46874 -> 46874/2343750 ~= 20 ms period
30 REM Centre pulse 1.5 ms: 46874 * 1.5 / 20 ~= 3515
40 PWMSEL 1
50 PWMWRP 46874
60 PWMLVL 0, 3515
70 PWMON 1
```

---

## I2C — Inter-Integrated Circuit Bus

Hardware: I2C0, **SDA = GP4**, **SCL = GP5**.  
Up to 8 bytes per transaction. Use `POKE` to fill the data buffer at
`$D051–$D058` (dec 53329–53336) before writing, or `PEEK` to read back
results after reading.

---

### `I2CADR addr`
Set the target device's 7-bit I2C address.

| Argument | Range  | Description |
|----------|--------|-------------|
| `addr`   | 0–127  | Device address (e.g. `60` = `$3C` for SSD1306 OLED) |

```basic
I2CADR 60            : REM target device at 0x3C
```

---

### `I2CWRT len`
Transmit `len` bytes from the data buffer (`$D051`+) to the selected device.
Fill the buffer with `POKE` first.

| Argument | Range | Description |
|----------|-------|-------------|
| `len`    | 1–8   | Number of bytes to send |

```basic
POKE 53329, 0        : REM I2C_DATA[0] = command byte
POKE 53330, 255      : REM I2C_DATA[1] = data byte
I2CWRT 2             : REM send 2 bytes
```

---

### `I2CRDT len`
Request `len` bytes from the selected device into the data buffer. Read
results back with `PEEK` after the transaction.

| Argument | Range | Description |
|----------|-------|-------------|
| `len`    | 1–8   | Number of bytes to receive |

```basic
I2CRDT 2             : REM receive 2 bytes
PRINT PEEK(53329)    : REM I2C_DATA[0]
PRINT PEEK(53330)    : REM I2C_DATA[1]
```

---

### `I2CSPD spd`
Set the I2C bus speed.

| Argument | Value | Description |
|----------|-------|-------------|
| `spd`    | `0`   | 100 kHz — standard mode (default) |
| `spd`    | `1`   | 400 kHz — fast mode |

```basic
I2CSPD 1             : REM switch to 400 kHz
```

#### Full I2C example — write 1 byte to SSD1306 OLED at 0x3C

```basic
10 I2CADR 60
20 POKE 53329, 0     : REM control byte (Co=0, D/C=0 -> command)
30 I2CWRT 1
```

#### Full I2C example — read temperature from device at 0x48

```basic
10 I2CADR 72         : REM 0x48
20 I2CRDT 2          : REM read 2 bytes (16-bit result)
30 HI = PEEK(53329)
40 LO = PEEK(53330)
50 PRINT (HI * 256 + LO) / 256 ; " C"
```

---

## DMA — Direct Memory Access

DMA channel 11 (reserved). Transfers are **non-blocking** — the 6502 keeps
running while the DMA is active. Operates within RP2350 SRAM; addresses are
the same 16-bit addresses used by PEEK/POKE (zero-extended to 32-bit).

---

### `DMASIZE n`
Set the transfer unit width for subsequent `DMACPY` calls.

| Argument | Value | Transfer unit |
|----------|-------|---------------|
| `n`      | `0`   | 1 byte (default) |
| `n`      | `1`   | 2 bytes (halfword) |
| `n`      | `2`   | 4 bytes (word) |

```basic
DMASIZE 0            : REM byte transfers (default)
```

### `DMAINCR i,j`
Set the transfer unit width for subsequent `DMACPY` calls.

| Argument | Value | Transfer unit |
|----------|-------|---------------|
| `i`      | `0 or 1`| increment the read address or not |
| 'j`      | `0 or 1`| increment the write address or not |

```basic
DMAINCR 0,1    : REM  FILLS the target with the data on the source
DMAINCR 1,1    : REM  COPY the source data to the target
DMAINCR 1,0    : REM  STREAM copy the source data to one single memory location
```

---

### `DMACPY src, dst, cnt`
Copy `cnt` units from address `src` to address `dst`. Both addresses
auto-increment. The transfer width is set by `DMASIZE`. Read and write
increment flags are forced to 1 (incrementing) — use `DMAINCR` for
non-standard increment modes.

| Argument | Range    | Description |
|----------|----------|-------------|
| `src`    | 0–65535  | Source address |
| `dst`    | 0–65535  | Destination address |
| `cnt`    | 1–65535  | Number of units to transfer |

```basic
DMACPY $0400, $0800, 1000  : REM copy 1000 bytes from screen to $0800
```

#### Full DMA example — clear screen faster than MEMFILL

```basic
10 POKE $FB, 32      : REM store a space character at $FB
20 DMASIZE 0
30 DMAINCR 0,1
40 DMACPY $FB, $0400, 1000  : REM NOTE: read-inc is always on, so use MEMFILL for fills
```

> **Note:** Both source and destination auto-increment, so `DMACPY` is best
> suited for block **copies**. For fills (same byte repeated), use `MEMFILL`,
> or use `DMAINCR 0,1` followed by a POKE-based DMA setup.

---

### `DMAINCR rinc, winc`
Explicitly set the read (source) and write (destination) address increment
flags. Use this when you need the DMA to read from or write to a fixed
address instead of the default auto-incrementing mode.

| Argument | Range | Description |
|----------|-------|-------------|
| `rinc`   | 0–1   | `0` = source address fixed, `1` = increment after each transfer |
| `winc`   | 0–1   | `0` = dest address fixed, `1` = increment after each transfer |

```basic
DMAINCR 0, 1         : REM fill mode — read same byte, walk destination
DMAINCR 1, 1         : REM copy mode — walk both (same as DMACPY default)
DMAINCR 1, 0         : REM stream to port — walk source, fixed destination
```

> **Tip:** `DMACPY` always forces `DMAINCR 1,1`. Set `DMAINCR` after a
> `DMASIZE` call but before writing `DMA_CTRL` via POKE when doing a manual
> DMA setup.

---

## Token Reference

| Token | Type     | Keyword   |
|-------|----------|-----------|
| `$DF` | command  | `PINMODE` |
| `$E0` | command  | `PINOUT`  |
| `$E1` | command  | `PINPULL` |
| `$E2` | command  | `PWMSEL`  |
| `$E3` | command  | `PWMLVL`  |
| `$E4` | command  | `PWMWRP`  |
| `$E5` | command  | `PWMON`   |
| `$E6` | command  | `PWMOFF`  |
| `$E7` | command  | `I2CADR`  |
| `$E8` | command  | `I2CWRT`  |
| `$E9` | command  | `I2CRDT`  |
| `$EA` | command  | `I2CSPD`  |
| `$EB` | command  | `DMACPY`  |
| `$EC` | command  | `DMASIZE` |
| `$ED` | command  | `DMAINCR` |
| `$EE` | function | `PINGET`  |
