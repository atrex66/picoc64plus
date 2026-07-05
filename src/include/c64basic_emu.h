#ifndef C64BASIC_EMU_H
#define C64BASIC_EMU_H

#include "memmap.h"

#define turbo_mode 0
#define use_host 0

// ── Overclocking settings ────────────────────────────────────────────────────
// Adjust these two defines to tune performance / stability.
// RP2350 default: 150 MHz @ VREG_VOLTAGE_1_10
// Tested stable:  300 MHz @ VREG_VOLTAGE_1_20  (flash speed irrelevant: copy_to_ram)
#if turbo_mode
    #define CPU_CLOCK_KHZ   375000              // target system clock in kHz
    #define CPU_CORE_VOLTAGE VREG_VOLTAGE_1_30  // core voltage (see hardware/vreg.h)
#else
    #define CPU_CLOCK_KHZ   150000              // target system clock in kHz
    #define CPU_CORE_VOLTAGE VREG_VOLTAGE_1_10  // core voltage (see hardware/vreg.h)
#endif
// ─────────────────────────────────────────────────────────────────────────────

uint16_t temp_address;
uint8_t temp;

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