#ifndef C64BASIC_EMU_H
#define C64BASIC_EMU_H

#define turbo_mode 0
#define use_host 0

// ── Overclocking settings ────────────────────────────────────────────────────
// Adjust these two defines to tune performance / stability.
// RP2350 default: 150 MHz @ VREG_VOLTAGE_1_10
// Tested stable:  300 MHz @ VREG_VOLTAGE_1_20  (flash speed irrelevant: copy_to_ram)
#if turbo_mode
    #define CPU_CLOCK_KHZ   300000              // target system clock in kHz
    #define CPU_CORE_VOLTAGE VREG_VOLTAGE_1_30  // core voltage (see hardware/vreg.h)
#else
    #define CPU_CLOCK_KHZ   150000              // target system clock in kHz
    #define CPU_CORE_VOLTAGE VREG_VOLTAGE_1_10  // core voltage (see hardware/vreg.h)
#endif
    // ─────────────────────────────────────────────────────────────────────────────

#define basic_start_address     0xA000
#define kernal_start_address    0xE000
#define charrom_start_address   0xD000
#define cart_low_start_address  0x8000
#define cart_high_start_address 0xE000

#define cart_data_bus 0        // starting gpio pin for data bus (8 bits)
#define cart_address_bus 8     // starting gpio pin for address bus (16 bits)
#define cart_irq_line 24       // gpio pin for irq line
#define cart_rw_line 25        // gpio pin for read/write line
#define cart_dot_clock_line 26 // gpio pin for clock line
#define cart_io1_line 27       // gpio pin for io1 line
#define cart_game_line 28      // gpio pin for game line
#define cart_exrom_line 29     // gpio pin for exrom line
#define cart_io2_line 30       // gpio pin for io2 line
#define cart_roml_line 31      // gpio pin for roml line
#define cart_ba_line 32        // gpio pin for dma line
#define cart_dma_line 33       // gpio pin for dma line
#define cart_romh_line 34      // gpio pin for romh line
#define cart_reset_line 35     // gpio pin for reset line
#define cart_nmi_line 36       // gpio pin for nmi line

static uint8_t c64_rgb[16][3] = {
    {0x00,0x00,0x00}, // black
    {0xFF,0xFF,0xFF}, // white
    {0xa1,0x4d,0x43}, // red
    {0x6a,0xc1,0xc8}, // cyan
    {0xa2,0x57,0xa5}, // purple
    {0x5c,0xad,0x5f}, // green
    {0x4f,0x44,0x9c}, // blue
    {0xcb,0xd6,0x89}, // yellow
    {0xa3,0x68,0x3a}, // orange
    {0x6e,0x54,0x0b}, // dark brown
    {0xcc,0x7f,0x76}, // light red
    {0x63,0x63,0x63}, // dark grey
    {0x8b,0x8b,0x8b}, // mid grey
    {0x9b,0xe3,0x9d}, // light green
    {0x8a,0x7f,0xcd}, // mid blue
    {0xaf,0xaf,0xaf}  // light grey
};

// C64 16 colours -> ANSI SGR colour codes
// Index = C64 colour number (0-15)
// Value: high byte = ANSI foreground code, low byte = ANSI background code
static const uint8_t c64_ansi_fg[16] = {
    30,  // 0  Black
    97,  // 1  White
    31,  // 2  Red
    36,  // 3  Cyan
    35,  // 4  Purple
    32,  // 5  Green
    34,  // 6  Blue
    33,  // 7  Yellow
    91,  // 8  Orange  (bright red, closest)
    33,  // 9  Brown   (yellow, closest)
    91,  // 10 Light Red
    90,  // 11 Dark Grey
    37,  // 12 Grey
    92,  // 13 Light Green
    94,  // 14 Light Blue
    97,  // 15 Light Grey
};

static const uint8_t c64_ansi_bg[16] = {
    40,  // 0  Black
    107, // 1  White
    41,  // 2  Red
    46,  // 3  Cyan
    45,  // 4  Purple
    42,  // 5  Green
    44,  // 6  Blue
    43,  // 7  Yellow
    101, // 8  Orange
    43,  // 9  Brown
    101, // 10 Light Red
    100, // 11 Dark Grey
    47,  // 12 Grey
    102, // 13 Light Green
    104, // 14 Light Blue
    107, // 15 Light Grey
};

// Jobb konverziós tábla (UTF-8 karaktereket ad vissza)
const char* c64_to_utf8[256] = {
    // 0x00-0x1F
    "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
    "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_",

    // 0x20-0x3F - normál ASCII
    " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",

    // 0x40-0x5F - grafikus terület
    " ", "▌", "▄", "▔", "▁", "▏", "↑", "▕", "█", "▄", "▀", "▔", "▁", "▏", "▒", "▕",
    "♥", "♦", "♣", "♠", "●", "○", "★", "■", "□", "◤", "◥", "◣", "◢", "◆", "◇", " ",

    // 0x60-0x7F
    " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ",
    " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ",

    // 0x80-0x9F - inverz
    "@", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    "p", "↓", "r", "s", "t", "u", "v", "w", "x", "y", "z", "[", "\\", "→", "^", "_",

    // 0xA0-0xBF
    " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",

    // 0xC0-0xDF - vonalak és box drawing (ez a legjobb rész)
    "─", "│", "┌", "┐", "└", "┘", "├", "┤", "┬", "┴", "┼", "▮", "▯", "▬", "▭", " ",
    "█", "↑", "▒", "░", "▕", "▏", "▎", "▍", "▋", "▊", "▉", "◀", "▶", "←", "▼", " ",

   "█", "↓", "→", "←", "↑", "⌂", "♥", "◆", "◀", "♪", "⏎", "▒", "░", "◤", "◥", "▲",  // 0xE0-0xEF
    "▼", "◣", "◢", "■", "□", "▬", "1", "2", "3", "4", "5", "6", "7", "8", " ", " "   // 0xF0-0xFF
};

// ── C64 memory-mapped addresses ───────────────────────────────────────────────
#define KEY_BUFFER               0x0277
#define KEY_COUNT                0x00C6
#define KEY_BUF_MAX              10

#define CURSOR_X_ADDRESS         0xD3
#define CURSOR_Y_ADDRESS         0xD6
#define CURSOR_ENABLE_ADDRESS    0xCC

#define RASTER_LINE              0xD012

#define TEXT_SCREEN_START        0x0400
#define COLOR_RAM_START          0xD800

#define BACKGROUND_COLOR_ADDRESS 0xD021
#define BORDER_COLOR_ADDRESS     0xD020

#define TOD_10THS_ADDRESS        0xDC04
#define TOD_SECONDS_ADDRESS      0xDC05
#define TOD_MINUTES_ADDRESS      0xDC06
#define TOD_HOURS_ADDRESS        0xDC07

// ── Terminal screen layout ────────────────────────────────────────────────────
#define BORDER_SIDE    6           // chars of border on each horizontal side
#define BORDER_TOP     2           // rows of border on top and bottom
#define C64_COLS       40
#define C64_ROWS       25
#define TOTAL_W        (C64_COLS + BORDER_SIDE * 2)
#define TOTAL_H        (C64_ROWS + BORDER_TOP  * 2)

// C64 keyboard buffer ($0277-$0280, max 10 chars) and count ($C6)
#define C64_KEY_BUFFER  0x0277
#define C64_KEY_COUNT   0x00C6
#define C64_KEY_BUF_MAX 0x0289

//
// GPIO registers – each is a 32-bit little-endian value spread over 4 bytes.
// To control GPIO N: byte offset = N/8, bit within that byte = N%8 (value = 1<<(N%8))
//
// ── GPIO25 (onboard LED on Pico) ─────────────────────────────────────────────
// GPIO25 → bit 25 → byte offset 3, bit 1 within that byte → write value 2 (=1<<1)
//
//   POKE 53302, 2   : set GPIO25 as output  (direction register, byte 3)
//   POKE 53298, 2   : GPIO25 HIGH → LED ON  (state register,     byte 3)
//   POKE 53298, 0   : GPIO25 LOW  → LED OFF
//
// C64 BASIC one-liner:
//   POKE 53302,2 : POKE 53298,2      (LED on)
//   POKE 53298,0                     (LED off)
// ─────────────────────────────────────────────────────────────────────────────
#define GPIO_STATE_ADDRESS     0xD02F  // 4 bytes LE: GPIO output state   (dec 53295)
#define GPIO_DIRECTION_ADDRESS 0xD033  // 4 bytes LE: GPIO direction 1=out (dec 53299)
#define GPIO_PULLUP_ADDRESS    0xD037  // 4 bytes LE: GPIO pull-up enable  (dec 53303)

// ── PWM memory-mapped registers ($D040-$D047) ────────────────────────────────
// Pico 2 has 12 PWM slices; slice N drives GPIO N*2 (ch A) and GPIO N*2+1 (ch B).
//
//  $D040  PWM_ENABLE    - bitmask: bit N = enable slice N
//  $D041  PWM_SELECT    - active slice index to configure (0-11)
//  $D042  PWM_WRAP_LO   - wrap (top) counter value, low  byte  (default 65535)
//  $D043  PWM_WRAP_HI   - wrap (top) counter value, high byte
//  $D044  PWM_LEVEL_A_LO- channel A duty cycle, low  byte  (0 .. wrap)
//  $D045  PWM_LEVEL_A_HI- channel A duty cycle, high byte
//  $D046  PWM_LEVEL_B_LO- channel B duty cycle, low  byte
//  $D047  PWM_LEVEL_B_HI- channel B duty cycle, high byte
#define PWM_ENABLE_ADDR    0xD040
#define PWM_SELECT_ADDR    0xD041
#define PWM_WRAP_LO_ADDR   0xD042
#define PWM_WRAP_HI_ADDR   0xD043
#define PWM_LEVEL_A_LO_ADDR 0xD044
#define PWM_LEVEL_A_HI_ADDR 0xD045
#define PWM_LEVEL_B_LO_ADDR 0xD046
#define PWM_LEVEL_B_HI_ADDR 0xD047

// ── I2C memory-mapped registers ($D050-$D05B) ────────────────────────────────
// Uses I2C0, SDA=GPIO4, SCL=GPIO5 by default.
//
//  $D050  I2C_ADDR   - 7-bit target device address
//  $D051  I2C_DATA   - data buffer, 8 bytes ($D051-$D058)
//  $D059  I2C_LEN    - number of bytes to transfer (1-8)
//  $D05A  I2C_CTRL   - write: 0x01=start write, 0x02=start read
//                      read:  0x80=busy, 0x40=NACK/error, 0x00=OK
//  $D05B  I2C_SPEED  - 0=100 kHz (standard), 1=400 kHz (fast)
#define I2C_ADDR_ADDR   0xD050
#define I2C_DATA_ADDR   0xD051  // 8-byte buffer: $D051..$D058
#define I2C_LEN_ADDR    0xD059
#define I2C_CTRL_ADDR   0xD05A
#define I2C_SPEED_ADDR  0xD05B

#define I2C_SDA_PIN  4
#define I2C_SCL_PIN  5
#define I2C_BUS      i2c0

// ── DMA memory-mapped registers ($D060-$D06D) ────────────────────────────────
// RP2350 has 16 DMA channels. One channel is reserved here for 6502 use.
// The transfer runs entirely on the ARM DMA controller; the 6502 is free
// to continue executing while the transfer is in progress.
//
//  $D060         DMA_CTRL      write: $01=start, $02=abort
//                               read:  $80=busy, $40=error, $00=idle
//  $D061         DMA_SIZE      0=byte, 1=halfword (2B), 2=word (4B)
//  $D062-$D065   DMA_READ_ADDR  source address, little-endian 32-bit
//  $D066-$D069   DMA_WRITE_ADDR destination address, little-endian 32-bit
//  $D06A-$D06B   DMA_COUNT      number of transfers (1-65535), little-endian
//  $D06C         DMA_READ_INC   1 = increment source address after each transfer
//  $D06D         DMA_WRITE_INC  1 = increment destination address after each transfer
//
// Addresses refer to the RP2350 physical memory map.
// The 6502 memory array starts at: (uintptr_t)memory  (in SRAM)
// Convenience: set DMA_READ_ADDR / DMA_WRITE_ADDR to
//   (uintptr_t)memory + 0xC000  to copy from $C000 in the 6502 address space.
//
#define DMA_CTRL_ADDR       0xD060
#define DMA_SIZE_ADDR       0xD061
#define DMA_READ_ADDR_ADDR  0xD062  // 4 bytes, LE
#define DMA_WRITE_ADDR_ADDR 0xD066  // 4 bytes, LE
#define DMA_COUNT_ADDR      0xD06A  // 2 bytes, LE
#define DMA_READ_INC_ADDR   0xD06C
#define DMA_WRITE_INC_ADDR  0xD06D

#define DMA_CHANNEL 11  // RP2350 DMA channel reserved for 6502 use

#pragma pack(push, 1)
typedef struct {
    uint8_t  magic[4];
    uint8_t  screen[TOTAL_W * TOTAL_H];   // PETSCII codes
    uint8_t  color[TOTAL_W * TOTAL_H];    // fg colour index 0-15
    uint8_t  background[TOTAL_W * TOTAL_H];  // $D021
    uint8_t  border;                    // $D020
    uint8_t  cursor_x;
    uint8_t  cursor_y;
    uint8_t  cursor_on;                 // 0 = on (visible), else off
    uint8_t  _pad;
} C64ScreenPacket;
#pragma pack(pop)


#endif // C64BASIC_EMU_H