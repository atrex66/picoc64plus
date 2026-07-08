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

The RP2350 has 8 PWM pins, each with two channels (A and B).

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

### `PWMSEL pin`


| Argument | Range | Description      |
|----------|-------|------------------|
| `pin`  | 0–31   | PWM pin number |

```basic
PWMSEL 0             : REM configure GP0
```

---

### `PWMWFRQ pin,freq`
Set the frequency

| Argument | Range    | Description       |
|----------|----------|-------------------|
| `freq`   | 1–65535  | Counter top value |

```basic
PWMFRQ 5000          : Sets 5khz output
```

---

### `PWMLVL pin, level`
Set the duty-cycle level for channel A (`0`) or B (`1`) of the selected
pin. `level` ranges from `0` (always off) to `wrap` (always on).

| Argument | Range   | Description                      |
|----------|---------|----------------------------------|
| `pin`    | 0–31    | pin number    |
| `level`  | 0–65535 | Duty cycle (0 = 0%, wrap = 100%) |

```basic
PWMLVL 0, 127        : pin0 duty 127 (the resolution depends on the frequency)
```

---

### `PWMON pin`
Enable a PWM pin (start generating the waveform).

| Argument | Range | Description |
|----------|-------|-------------|
| `pin`  | 0–7   | PWM pin to enable |

```basic
PWMON 0              : REM start PWM on gp0
```

---

### `PWMOFF pin`
Disable a PWM pin (stop the waveform, output goes LOW).

| Argument | Range | Description |
|----------|-------|-------------|
| `pin`  | 0–7   | PWM pin to disable |

```basic
PWMOFF 0             : REM stop PWM on pin 0
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

### `DMACPY src, dst, cnt`
Copy `cnt` units from address `src` to address `dst`. Both addresses
auto-increment.

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
40 DMACPY $FB, $0400, 1000  : REM NOTE: read-inc is always on, so use MEMFILL for fills
```

> **Note:** Both source and destination auto-increment, so `DMACPY` is best
> suited for block **copies**. For fills (same byte repeated), use `MEMFILL`,
> or use `DMAINCR 0,1` followed by a POKE-based DMA setup.

---

### `DMAFILL src, dst, cnt`
Explicitly set the read (source) and write (destination) address increment
flags. Use this when you need the DMA to read from or write to a fixed
address instead of the default auto-incrementing mode.

| Argument | Range    | Description |
|----------|----------|-------------|
| `src`    | 0–65535  | Source address |
| `dst`    | 0–65535  | Destination address |
| `cnt`    | 1–65535  | Number of units to transfer |

```basic
10 POKE $FB, 32 
40 DMAFILL $FB, $0400, 1000
```

---
