#pragma once
#include <stdint.h>

// ─────────────────────────────────────────────────────────────────────────────
// Wire protocol: Pico → host
//
// The Pico sends one C64ScreenPacket per frame over USB-CDC serial.
// The host reads exactly sizeof(C64ScreenPacket) bytes each frame.
//
// Frame format (little-endian, packed):
//   [4 bytes]  magic          0xC6 0x40 0xDE 0xAD
//   [1508 bytes] screen       PETSCII codes, row-major 52×29 (full frame incl. border)
//   [1508 bytes] color        fg colour index (0-15) per cell
//   [1 byte]   background     VIC background colour index ($D021)
//   [1 byte]   border         VIC border colour index   ($D020)
//   [1 byte]   cursor_x       cursor column in full-frame coords (0-51)
//   [1 byte]   cursor_y       cursor row    in full-frame coords (0-28)
//   [1 byte]   cursor_on      0 = cursor visible, non-zero = hidden
//   [1 byte]   _pad           alignment padding
//   ─────────────────────────────────────────────────────────────────────────
//   Total: 4 + 1508 + 1508 + 6 = 3026 bytes
// ─────────────────────────────────────────────────────────────────────────────

// Screen content area (excludes border)
#define C64_COLS        40
#define C64_ROWS        25

// Border thickness in character cells
#define BORDER_SIDE     6                                   // chars left+right
#define BORDER_TOP_ROWS 2                                   // chars top+bottom

// Total display size including border (52×29 chars)
#define TOTAL_COLS      (C64_COLS + BORDER_SIDE * 2)        // 52
#define TOTAL_ROWS      (C64_ROWS + BORDER_TOP_ROWS * 2)    // 29

#define C64_SCREEN_SIZE (TOTAL_COLS * TOTAL_ROWS)           // 1508 bytes

#define FRAME_MAGIC_0   0xC6
#define FRAME_MAGIC_1   0x40
#define FRAME_MAGIC_2   0xDE
#define FRAME_MAGIC_3   0xAD

#pragma pack(push, 1)
typedef struct {
    uint8_t  magic[4];
    uint8_t  screen[C64_SCREEN_SIZE];   // PETSCII codes
    uint8_t  color[C64_SCREEN_SIZE];    // fg colour index 0-15
    uint8_t  background[C64_SCREEN_SIZE];  // $D021
    uint8_t  cursor_x;
    uint8_t  cursor_y;
    uint8_t  cursor_on;                 // 0 = on (visible), else off
    uint8_t  _pad;
} C64ScreenPacket;
#pragma pack(pop)

#define PACKET_SIZE     sizeof(C64ScreenPacket)   // 3026 bytes

// C64 palette — RGB888, index 0-15
static const uint8_t C64_PALETTE[16][3] = {
    {0x00, 0x00, 0x00}, // 0  Black
    {0xFF, 0xFF, 0xFF}, // 1  White
    {0xa1, 0x4d, 0x43}, // 2  Red
    {0x6a, 0xc1, 0xc8}, // 3  Cyan
    {0xa2, 0x57, 0xa5}, // 4  Purple
    {0x5c, 0xad, 0x5f}, // 5  Green
    {0x4f, 0x44, 0x9c}, // 6  Blue
    {0xcb, 0xd6, 0x89}, // 7  Yellow
    {0xa3, 0x68, 0x3a}, // 8  Orange
    {0x6e, 0x54, 0x0b}, // 9  Brown
    {0xcc, 0x7f, 0x76}, // 10 Light Red
    {0x63, 0x63, 0x63}, // 11 Dark Grey
    {0x8b, 0x8b, 0x8b}, // 12 Mid Grey
    {0x9b, 0xe3, 0x9d}, // 13 Light Green
    {0x8a, 0x7f, 0xcd}, // 14 Mid Blue
    {0xaf, 0xaf, 0xaf}, // 15 Light Grey
};
