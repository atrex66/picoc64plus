#ifndef TERMINALNINJA_H
#define TERMINALNINJA_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>

#define F_W 40  
#define F_H 25

#define DEBUG 0

void restore_terminal();
// with pico-sdk the time_us_32() function is already defined in pico/time.h
#include "pico/time.h"
#include "pico/stdlib.h"
#include "pico/stdio.h"

typedef enum{
    SCREEN,
    COLOR,
    BACKGROUND,
    COLOR_BLEND_ADD,
    COLOR_BLEND_SUB,
} BufferType;

typedef struct {
    int width;
    int height;
    char *data;
    char *color_data;
    char *background_data;
    char transparent_char;
    char pixel_char;
} Buffer;

typedef struct{
    Buffer screen_buffer;
    int width;
    int height;
    int position_x;
    int position_y;
    int background_color;
    char *title;
}window_t;

#define INPUT_SIZE 20

typedef struct {
    uint8_t buf[INPUT_SIZE];
    ssize_t bytes;
} input_queue_t;


int FRAME_WIDTH, FRAME_HEIGHT;
uint8_t *screen;
uint8_t *color_data;
uint8_t *background; // Default background color
int cursor_x = 0;
int cursor_y = 0;
int cursor_enable = 0;
int pos = 0;
int max_line_length = 0;
uint32_t total_bytes_sent = 0;
Buffer big_characters[128]; // Array to hold big character buffers for ASCII characters
static const char shades[] = " .,:;irsXA253hMHGS#9B&@";
uint64_t frame_time = 0;
uint64_t render_time = 0;
uint8_t palette[256][3];
char *line_buffer;
int _position = 0;
bool fps_lock = false;
uint32_t current_time = 0;
uint32_t target_frame_ns = 16666ul;
input_queue_t input_queue = {0};
bool use_utf8 = true; // Flag to determine whether to use UTF-8 characters or not

#define send_home() APPEND("\033[H")

static const uint8_t _palette[256][3] = {
{0x00, 0x00, 0x00},
{0x00, 0x00, 0xAA},
{0x00, 0xAA, 0x00},
{0x00, 0xAA, 0xAA},
{0xAA, 0x00, 0x00},
{0xAA, 0x00, 0xAA},
{0xAA, 0x55, 0x00},
{0xAA, 0xAA, 0xAA},
{0x55, 0x55, 0x55},
{0x55, 0x55, 0xFF},
{0x55, 0xFF, 0x55},
{0x55, 0xFF, 0xFF},
{0xFF, 0x55, 0x55},
{0xFF, 0x55, 0xFF},
{0xFF, 0xFF, 0x55},
{0xFF, 0xFF, 0xFF},
{0x00, 0x00, 0x00},
{0x10, 0x10, 0x10},
{0x20, 0x20, 0x20},
{0x35, 0x35, 0x35},
{0x45, 0x45, 0x45},
{0x55, 0x55, 0x55},
{0x65, 0x65, 0x65},
{0x75, 0x75, 0x75},
{0x8A, 0x8A, 0x8A},
{0x9A, 0x9A, 0x9A},
{0xAA, 0xAA, 0xAA},
{0xBA, 0xBA, 0xBA},
{0xCA, 0xCA, 0xCA},
{0xDF, 0xDF, 0xDF},
{0xEF, 0xEF, 0xEF},
{0xFF, 0xFF, 0xFF},
{0x00, 0x00, 0xFF},
{0x41, 0x00, 0xFF},
{0x82, 0x00, 0xFF},
{0xBE, 0x00, 0xFF},
{0xFF, 0x00, 0xFF},
{0xFF, 0x00, 0xBE},
{0xFF, 0x00, 0x82},
{0xFF, 0x00, 0x41},
{0xFF, 0x00, 0x00},
{0xFF, 0x41, 0x00},
{0xFF, 0x82, 0x00},
{0xFF, 0xBE, 0x00},
{0xFF, 0xFF, 0x00},
{0xBE, 0xFF, 0x00},
{0x82, 0xFF, 0x00},
{0x41, 0xFF, 0x00},
{0x00, 0xFF, 0x00},
{0x00, 0xFF, 0x41},
{0x00, 0xFF, 0x82},
{0x00, 0xFF, 0xBE},
{0x00, 0xFF, 0xFF},
{0x00, 0xBE, 0xFF},
{0x00, 0x82, 0xFF},
{0x00, 0x41, 0xFF},
{0x82, 0x82, 0xFF},
{0x9E, 0x82, 0xFF},
{0xBE, 0x82, 0xFF},
{0xDF, 0x82, 0xFF},
{0xFF, 0x82, 0xFF},
{0xFF, 0x82, 0xDF},
{0xFF, 0x82, 0xBE},
{0xFF, 0x82, 0x9E},
{0xFF, 0x82, 0x82},
{0xFF, 0x9E, 0x82},
{0xFF, 0xBE, 0x82},
{0xFF, 0xDF, 0x82},
{0xFF, 0xFF, 0x82},
{0xDF, 0xFF, 0x82},
{0xBE, 0xFF, 0x82},
{0x9E, 0xFF, 0x82},
{0x82, 0xFF, 0x82},
{0x82, 0xFF, 0x9E},
{0x82, 0xFF, 0xBE},
{0x82, 0xFF, 0xDF},
{0x82, 0xFF, 0xFF},
{0x82, 0xDF, 0xFF},
{0x82, 0xBE, 0xFF},
{0x82, 0x9E, 0xFF},
{0xBA, 0xBA, 0xFF},
{0xCA, 0xBA, 0xFF},
{0xDF, 0xBA, 0xFF},
{0xEF, 0xBA, 0xFF},
{0xFF, 0xBA, 0xFF},
{0xFF, 0xBA, 0xEF},
{0xFF, 0xBA, 0xDF},
{0xFF, 0xBA, 0xCA},
{0xFF, 0xBA, 0xBA},
{0xFF, 0xCA, 0xBA},
{0xFF, 0xDF, 0xBA},
{0xFF, 0xEF, 0xBA},
{0xFF, 0xFF, 0xBA},
{0xEF, 0xFF, 0xBA},
{0xDF, 0xFF, 0xBA},
{0xCA, 0xFF, 0xBA},
{0xBA, 0xFF, 0xBA},
{0xBA, 0xFF, 0xCA},
{0xBA, 0xFF, 0xDF},
{0xBA, 0xFF, 0xEF},
{0xBA, 0xFF, 0xFF},
{0xBA, 0xEF, 0xFF},
{0xBA, 0xDF, 0xFF},
{0xBA, 0xCA, 0xFF},
{0x00, 0x00, 0x71},
{0x1C, 0x00, 0x71},
{0x39, 0x00, 0x71},
{0x55, 0x00, 0x71},
{0x71, 0x00, 0x71},
{0x71, 0x00, 0x55},
{0x71, 0x00, 0x39},
{0x71, 0x00, 0x1C},
{0x71, 0x00, 0x00},
{0x71, 0x1C, 0x00},
{0x71, 0x39, 0x00},
{0x71, 0x55, 0x00},
{0x71, 0x71, 0x00},
{0x55, 0x71, 0x00},
{0x39, 0x71, 0x00},
{0x1C, 0x71, 0x00},
{0x00, 0x71, 0x00},
{0x00, 0x71, 0x1C},
{0x00, 0x71, 0x39},
{0x00, 0x71, 0x55},
{0x00, 0x71, 0x71},
{0x00, 0x55, 0x71},
{0x00, 0x39, 0x71},
{0x00, 0x1C, 0x71},
{0x39, 0x39, 0x71},
{0x45, 0x39, 0x71},
{0x55, 0x39, 0x71},
{0x61, 0x39, 0x71},
{0x71, 0x39, 0x71},
{0x71, 0x39, 0x61},
{0x71, 0x39, 0x55},
{0x71, 0x39, 0x45},
{0x71, 0x39, 0x39},
{0x71, 0x45, 0x39},
{0x71, 0x55, 0x39},
{0x71, 0x61, 0x39},
{0x71, 0x71, 0x39},
{0x61, 0x71, 0x39},
{0x55, 0x71, 0x39},
{0x45, 0x71, 0x39},
{0x39, 0x71, 0x39},
{0x39, 0x71, 0x45},
{0x39, 0x71, 0x55},
{0x39, 0x71, 0x61},
{0x39, 0x71, 0x71},
{0x39, 0x61, 0x71},
{0x39, 0x55, 0x71},
{0x39, 0x45, 0x71},
{0x51, 0x51, 0x71},
{0x59, 0x51, 0x71},
{0x61, 0x51, 0x71},
{0x69, 0x51, 0x71},
{0x71, 0x51, 0x71},
{0x71, 0x51, 0x69},
{0x71, 0x51, 0x61},
{0x71, 0x51, 0x59},
{0x71, 0x51, 0x51},
{0x71, 0x59, 0x51},
{0x71, 0x61, 0x51},
{0x71, 0x69, 0x51},
{0x71, 0x71, 0x51},
{0x69, 0x71, 0x51},
{0x61, 0x71, 0x51},
{0x59, 0x71, 0x51},
{0x51, 0x71, 0x51},
{0x51, 0x71, 0x59},
{0x51, 0x71, 0x61},
{0x51, 0x71, 0x69},
{0x51, 0x71, 0x71},
{0x51, 0x69, 0x71},
{0x51, 0x61, 0x71},
{0x51, 0x59, 0x71},
{0x00, 0x00, 0x41},
{0x10, 0x00, 0x41},
{0x20, 0x00, 0x41},
{0x31, 0x00, 0x41},
{0x41, 0x00, 0x41},
{0x41, 0x00, 0x31},
{0x41, 0x00, 0x20},
{0x41, 0x00, 0x10},
{0x41, 0x00, 0x00},
{0x41, 0x10, 0x00},
{0x41, 0x20, 0x00},
{0x41, 0x31, 0x00},
{0x41, 0x41, 0x00},
{0x31, 0x41, 0x00},
{0x20, 0x41, 0x00},
{0x10, 0x41, 0x00},
{0x00, 0x41, 0x00},
{0x00, 0x41, 0x10},
{0x00, 0x41, 0x20},
{0x00, 0x41, 0x31},
{0x00, 0x41, 0x41},
{0x00, 0x31, 0x41},
{0x00, 0x20, 0x41},
{0x00, 0x10, 0x41},
{0x20, 0x20, 0x41},
{0x28, 0x20, 0x41},
{0x31, 0x20, 0x41},
{0x39, 0x20, 0x41},
{0x41, 0x20, 0x41},
{0x41, 0x20, 0x39},
{0x41, 0x20, 0x31},
{0x41, 0x20, 0x28},
{0x41, 0x20, 0x20},
{0x41, 0x28, 0x20},
{0x41, 0x31, 0x20},
{0x41, 0x39, 0x20},
{0x41, 0x41, 0x20},
{0x39, 0x41, 0x20},
{0x31, 0x41, 0x20},
{0x28, 0x41, 0x20},
{0x20, 0x41, 0x20},
{0x20, 0x41, 0x28},
{0x20, 0x41, 0x31},
{0x20, 0x41, 0x39},
{0x20, 0x41, 0x41},
{0x20, 0x39, 0x41},
{0x20, 0x31, 0x41},
{0x20, 0x28, 0x41},
{0x2D, 0x2D, 0x41},
{0x31, 0x2D, 0x41},
{0x35, 0x2D, 0x41},
{0x3D, 0x2D, 0x41},
{0x41, 0x2D, 0x41},
{0x41, 0x2D, 0x3D},
{0x41, 0x2D, 0x35},
{0x41, 0x2D, 0x31},
{0x41, 0x2D, 0x2D},
{0x41, 0x31, 0x2D},
{0x41, 0x35, 0x2D},
{0x41, 0x3D, 0x2D},
{0x41, 0x41, 0x2D},
{0x3D, 0x41, 0x2D},
{0x35, 0x41, 0x2D},
{0x31, 0x41, 0x2D},
{0x2D, 0x41, 0x2D},
{0x2D, 0x41, 0x31},
{0x2D, 0x41, 0x35},
{0x2D, 0x41, 0x3D},
{0x2D, 0x41, 0x41},
{0x2D, 0x3D, 0x41},
{0x2D, 0x35, 0x41},
{0x2D, 0x31, 0x41},
{0x00, 0x00, 0x00},
{0x00, 0x00, 0x00},
{0x00, 0x00, 0x00},
{0x00, 0x00, 0x00},
{0x00, 0x00, 0x00},
{0x00, 0x00, 0x00},
{0x00, 0x00, 0x00},
{0x00, 0x00, 0x00},
};

uint8_t red_palette[32][3] = {
    {0, 0, 0},
    {15, 0, 0},
    {30, 0, 0},
    {46, 0, 0},
    {61, 0, 0},
    {77, 0, 0},
    {96, 4, 0},
    {115, 10, 0},
    {134, 16, 0},
    {154, 22, 0},
    {173, 28, 0},
    {189, 36, 0},
    {204, 46, 0},
    {218, 55, 0},
    {233, 65, 0},
    {247, 75, 0},
    {255, 87, 0},
    {255, 103, 0},
    {255, 118, 0},
    {255, 134, 0},
    {255, 149, 0},
    {255, 163, 7},
    {255, 175, 30},
    {255, 187, 54},
    {255, 198, 77},
    {255, 210, 100},
    {255, 221, 124},
    {255, 227, 150},
    {255, 234, 176},
    {255, 241, 202},
    {255, 248, 228},
    {255, 255, 254},
};

uint8_t green_palette[32][3] = {
    {0, 0, 0},
    {0, 15, 0},
    {0, 30, 0},
    {0, 46, 0},
    {0, 61, 0},
    {0, 77, 0},
    {0, 96, 4},
    {0, 115, 10},
    {0, 134, 16},
    {0, 154, 22},
    {0, 173, 28},
    {0, 189, 36},
    {0, 204, 46},
    {0, 218, 55},
    {0, 233, 65},
    {0, 247, 75},
    {0, 255, 87},
    {0, 255, 103},
    {0, 255, 118},
    {0, 255, 134},
    {0, 255, 149},
    {7, 255, 163},
    {30, 255, 175},
    {54, 255, 187},
    {77, 255, 198},
    {100, 255, 210},
    {124, 255, 221},
    {150, 255, 227},
    {176, 255, 234},
    {202, 255, 241},
    {228, 255, 248},
    {254, 255, 255},
};

uint8_t blue_palette[32][3] = {
    {0, 0, 0},
    {0, 0, 15},
    {0, 0, 30},
    {0, 0, 46},
    {0, 0, 61},
    {0, 0, 77},
    {0, 4, 96},
    {0, 10, 115},
    {0, 16, 134},
    {0, 22, 154},
    {0, 28, 173},
    {0, 36, 189},
    {0, 46, 204},
    {0, 55, 218},
    {0, 65, 233},
    {0, 75, 247},
    {0, 87, 255},
    {0, 103, 255},
    {0, 118, 255},
    {0, 134, 255},
    {0, 149, 255},
    {7, 163, 255},
    {30, 175, 255},
    {54, 187, 255},
    {77, 198, 255},
    {100, 210, 255},
    {124, 221, 255},
    {150, 227, 255},
    {176, 234, 255},
    {202, 241, 255},
    {228, 248, 255},
    {254, 255, 255},
};

char font8x8_basic[128][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0000 (nul)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0001
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0002
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0003
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0004
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0005
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0006
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0007
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0008
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0009
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0010
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0011
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0012
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0013
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0014
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0015
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0016
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0017
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0018
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0019
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0020 (space)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},   // U+0021 (!)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0022 (")
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},   // U+0023 (#)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00},   // U+0024 ($)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},   // U+0025 (%)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00},   // U+0026 (&)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0027 (')
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00},   // U+0028 (()
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00},   // U+0029 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},   // U+002A (*)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00},   // U+002B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+002C (,)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},   // U+002D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+002E (.)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},   // U+002F (/)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},   // U+0030 (0)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},   // U+0031 (1)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},   // U+0032 (2)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},   // U+0033 (3)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},   // U+0034 (4)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},   // U+0035 (5)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},   // U+0036 (6)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},   // U+0037 (7)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},   // U+0038 (8)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},   // U+0039 (9)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+003A (:)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+003B (;)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},   // U+003C (<)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00},   // U+003D (=)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00},   // U+003E (>)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},   // U+003F (?)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},   // U+0041 (A)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},   // U+0042 (B)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},   // U+0043 (C)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},   // U+0044 (D)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},   // U+0045 (E)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},   // U+0046 (F)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},   // U+0047 (G)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},   // U+0048 (H)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0049 (I)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},   // U+004A (J)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},   // U+004B (K)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},   // U+004C (L)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},   // U+004D (M)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},   // U+004E (N)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},   // U+004F (O)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},   // U+0050 (P)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},   // U+0051 (Q)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},   // U+0052 (R)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},   // U+0053 (S)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0054 (T)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},   // U+0055 (U)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0056 (V)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},   // U+0057 (W)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},   // U+0058 (X)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},   // U+0059 (Y)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},   // U+005A (Z)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00},   // U+005B ([)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},   // U+005C (\)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},   // U+005D (])
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00},   // U+005E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},   // U+005F (_)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0060 (`)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00},   // U+0061 (a)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00},   // U+0062 (b)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},   // U+0063 (c)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00},   // U+0064 (d)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00},   // U+0065 (e)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00},   // U+0066 (f)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0067 (g)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00},   // U+0068 (h)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0069 (i)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E},   // U+006A (j)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00},   // U+006B (k)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+006C (l)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00},   // U+006D (m)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00},   // U+006E (n)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},   // U+006F (o)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F},   // U+0070 (p)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78},   // U+0071 (q)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00},   // U+0072 (r)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00},   // U+0073 (s)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00},   // U+0074 (t)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00},   // U+0075 (u)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0076 (v)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00},   // U+0077 (w)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00},   // U+0078 (x)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0079 (y)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00},   // U+007A (z)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00},   // U+007B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},   // U+007C (|)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00},   // U+007D (})
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+007E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}    // U+007F
};


void init_terminal();
void frame_start();
void frame_end();
void set_framerate(int fps);
void char_atc(int x, int y, uint8_t color, char ch);
void char_at(int x, int y, char ch);
void fill_line(int y, char ch);
void fill_linec(int y, uint8_t color, char ch);
void fill_screen(char ch);
void _render_line(int y, bool crlf);
void render_frame();
void hide_terminal_cursor();
void show_terminal_cursor();
void show_cursor();
void hide_cursor();
void blit_add_to_screen(Buffer *buffer, int x, int y);
void blit_sub_to_screen(Buffer *buffer, int x, int y);
void blit_to_screen(Buffer *buffer, int x, int y);
void blit_to(Buffer *buffer, int x, int y, BufferType type);
void blit_to_color(Buffer *buffer, int x, int y);
void blit_to_background(Buffer *buffer, int x, int y);
void change_to_G0();
void fill_background(uint8_t color);

#define clear_terminal() APPEND("\033[2J")  // clear screen and move cursor to home position

#define APPEND(fmt, ...) _position += snprintf(line_buffer + _position, (19 * FRAME_WIDTH) * FRAME_HEIGHT - _position, fmt, ##__VA_ARGS__)

void infinite_loop() {
    while (1) {
        
    }
}

// Convert terminal ASCII to C64 PETSCII (uppercase mode)
static uint8_t ascii_to_petscii(char c) {
    // Lowercase a-z -> uppercase A-Z (PETSCII uppercase/graphics mode)
    if (c >= 'a' && c <= 'z') return (uint8_t)(c - 0x20);
    // Return / linefeed -> PETSCII RETURN
    if (c == '\r' || c == '\n') return 0x0D;
    // Backspace / DEL -> PETSCII DEL (C64 backspace)
    if (c == 0x08 || c == 0x7F) return 0x14;
    // Printable ASCII $20-$5F map 1:1 to PETSCII
    if (c >= 0x20 && c <= 0x5F) return (uint8_t)c;
    return 0;
}

void set_framerate(int fps) {
    if (fps <= 0) {
        fps_lock = false; // Disable FPS lock if fps is zero or negative
    } else {
        fps_lock = true; // Enable FPS lock
        target_frame_ns = 1000000 / fps; // Convert FPS to microseconds per frame
    }
}

void frame_start() {
    current_time = time_us_32();
}

void fps_lock_on() {
    fps_lock = true;
}

void fps_lock_off() {
    fps_lock = false;
}

static inline int read_input(input_queue_t *q)
{
    q->bytes = read(STDIN_FILENO, q->buf, 20);
    return q->bytes > 0 ? q->bytes : -1;
}


void frame_end() {
    if (fps_lock) {
        uint64_t target_time = current_time + target_frame_ns; // Target time for 60 FPS
        // wait_until(target_time);
        while (time_us_32() < target_time) {
            // Busy wait
        }
    }
}

void fill_background(uint8_t color) {
    memset(background, color, FRAME_WIDTH * FRAME_HEIGHT);
}

void fill_background_buffer(Buffer *buffer, uint8_t color) {
    memset(buffer->background_data, color, buffer->width * buffer->height);
}

void char_atc(int x, int y, uint8_t color, char ch) {
    if (x < 0 || x >= FRAME_WIDTH ||
        y < 0 || y >= FRAME_HEIGHT)
    {
        return;
    }

    int pos = y * FRAME_WIDTH + x;

    screen[pos] = ch;
    color_data[pos] = color;
}

void char_at(int x, int y, char ch) {
    int pos = (y) * FRAME_WIDTH + (x);
    if (pos < 0 || pos >= FRAME_WIDTH * FRAME_HEIGHT) {
        return; // Out of bounds, do nothing
    }
    screen[pos] = ch;
}

void char_at_buffer(Buffer *buffer, int x, int y, char ch) {
    if (x < 0 || x >= buffer->width ||
        y < 0 || y >= buffer->height)
    {
        return;
    }

    int pos = y * buffer->width + x;

    buffer->data[pos] = ch;
}

void char_atc_buffer(Buffer *buffer, int x, int y, uint8_t color, char ch) {
    if (x < 0 || x >= buffer->width ||
        y < 0 || y >= buffer->height)
    {
        return;
    }

    int pos = y * buffer->width + x;

    buffer->data[pos] = ch;
    buffer->color_data[pos] = color;
}

void change_to_G0(){
    APPEND("\033(0"); // Switch to G0 character set
}

void fill_line(int y, char ch) {
    memset(screen + (y) * FRAME_WIDTH, (ch), FRAME_WIDTH);
}

void fill_linec(int y, uint8_t color, char ch) {
    memset(screen + (y) * FRAME_WIDTH, (ch), FRAME_WIDTH);
    memset(color_data + (y) * FRAME_WIDTH, (color), FRAME_WIDTH);
}

void fill_screen(char ch) {
    memset(screen, (ch), FRAME_WIDTH * FRAME_HEIGHT);
}

void fill_color(uint8_t color) {
    memset(color_data, (color), FRAME_WIDTH * FRAME_HEIGHT);
}

uint16_t max_length = 0;

void _render_line(int y, bool crlf) {
    uint8_t old_color = 0xFF; // Initialize to an invalid color
    uint8_t old_background_color = 0xFF; // Initialize to an invalid color
    int pos_temp = _position; // Store the starting position of the line
    for (int x = 0; x < FRAME_WIDTH; x++) {
        int _pos = (y) * FRAME_WIDTH + (x);
        uint8_t color = color_data[_pos];
        uint8_t background_color = background[_pos];
        bool is_cursor_position = (cursor_enable && cursor_x == x && cursor_y == y);
        if (is_cursor_position) {
            APPEND("\033[7m"); // Invert colors for cursor position
            total_bytes_sent += 4; // For the escape sequence
        }
        if (color != old_color) {
            APPEND("\033[38;2;%u;%u;%um", palette[color][0], palette[color][1], palette[color][2]);
            total_bytes_sent += 19;
            old_color = color;
        } 
        if (background_color != old_background_color) {
            APPEND("\033[48;2;%u;%u;%um", palette[background_color][0], palette[background_color][1], palette[background_color][2]);
            total_bytes_sent += 19;
            old_background_color = background_color;
        }
        if (!use_utf8)
        {
            APPEND("%c", screen[_pos]);
        } else
        {
            APPEND("%s", c64_to_utf8[screen[_pos]]);
        }
        total_bytes_sent += 1;
        if (is_cursor_position) {
            APPEND("\033[0m"); // Reapply invert colors for cursor position
            old_color = 0xFF; // Reset old_color to force color change on next iteration
            old_background_color = 0xFF; // Reset old_background_color to force background color change on next iteration
            total_bytes_sent += 4; // For the escape sequence
        }
    }
    if (crlf)
    {
    APPEND("\n");
    }
    if (max_line_length < _position - pos_temp) {
        max_line_length = _position - pos_temp; // Update max_line_length if current line is longer
    }
    total_bytes_sent += 1; // For the newline character
    if (max_length < _position) {
        max_length = _position; // Update max_length if current line is longer
    }
}

void render_frame() {
    _position = 0; // Reset position for the next line
    uint64_t now = time_us_32();
    APPEND("\033[0;0H");
    // change_to_G0();
    total_bytes_sent = 0;
    for (int _y = 0; _y < FRAME_HEIGHT-1; _y++) {
        _render_line(_y, true);
    }
    _render_line(FRAME_HEIGHT-1, false); // Last line without newline
    render_time = time_us_32() - now;

    #if DEBUG
    // reset attributes to default after rendering
    APPEND("\033[0m");
    APPEND("\033(B");
    total_bytes_sent += 4; // For the escape sequence
    APPEND("\nFrame render time: %lu us\033[K\n", (unsigned long)render_time);
    APPEND("Total bytes sent: %lu\033[K\n", (unsigned long)total_bytes_sent);
    APPEND("Frame time: %lu us\033[K\n", (unsigned long)frame_time);
    APPEND("Max line length: %d\033[K\n", max_line_length);
    APPEND("Frame dimensions: %dx%d\033[K\n", FRAME_WIDTH, FRAME_HEIGHT);
    if (fps_lock){
        APPEND("FPS: %.2f (locked)\033[K\n", 1000000.0 / target_frame_ns);
        APPEND("MAX FPS: %.2f\033[K\n", 1000000.0 / frame_time);
    } else {
        APPEND("FPS: %.2f\033[K\n", 1000000.0 / frame_time);
    }
    APPEND("FPS lock: %s\033[K", fps_lock ? "ON" : "OFF");
    #endif
    // line_buffer[_position++] = '\n';
    line_buffer[_position] = '\0'; // Null-terminate the string
    puts(line_buffer); // Use puts to write the entire buffer at once
    fflush(stdout);
    frame_time = time_us_32() - now;
}

void init_terminal(int width, int height) {
    FRAME_WIDTH = width;
    FRAME_HEIGHT = height;
    screen = (uint8_t *)malloc(FRAME_WIDTH * FRAME_HEIGHT);
    color_data = (uint8_t *)malloc(FRAME_WIDTH * FRAME_HEIGHT);
    background = (uint8_t *)malloc(FRAME_WIDTH * FRAME_HEIGHT);
    line_buffer = (char *)malloc((19 * FRAME_WIDTH) * FRAME_HEIGHT); // Buffer to hold the formatted line for rendering
    clear_terminal();
    hide_terminal_cursor();
    show_cursor();
    for (int _y = 0; _y < FRAME_HEIGHT; _y++) {
        for (int _x = 0; _x < FRAME_WIDTH; _x++) {
            int _pos = (_y) * FRAME_WIDTH + (_x);
            screen[_pos] = ' ';
            color_data[_pos] = 0x0f;
            background[_pos] = 0x00;
        }
    }
    for (int i = 0; i < 256; i++) {
        palette[i][0] = _palette[i][0];
        palette[i][1] = _palette[i][1];
        palette[i][2] = _palette[i][2];
    }
}

void restore_terminal() {
    write(STDOUT_FILENO, "\033[?1049l", 8); /* switch back to normal buffer */
    write(STDOUT_FILENO, "\033(B", 3);
    write(STDOUT_FILENO, "\033[0m", 4);   // színek törlése
    write(STDOUT_FILENO, "\033[?25h", 6); // kurzor látható
    write(STDOUT_FILENO, "\033[2J", 4);   // clear screen
}

void hide_terminal_cursor() {
    printf("\033[?25l"); /* hide cursor */
}

void show_terminal_cursor() {
    printf("\033[?25h"); /* show cursor */
}

void show_cursor() {
    cursor_enable = 1;
}

void hide_cursor() {
    cursor_enable = 0;
}

void set_cursor_position(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}

void get_lengtht_of_big_text(const char *text, int *width, int *height) {
    int len = strlen(text);
    *width = len * 8; // Each character is 8 pixels wide
    *height = 8;      // Each character is 8 pixels tall
}


Buffer *create_buffer(int width, int height, char transparent_char, char pixel_char) {
    Buffer *buffer = (Buffer *)malloc(sizeof(Buffer));
    buffer->width = width;
    buffer->height = height;
    buffer->transparent_char = transparent_char;
    buffer->pixel_char = pixel_char;
    buffer->data = (char *)malloc(width * height);
    buffer->color_data = (char *)malloc(width * height);
    buffer->background_data = (char *)malloc(width * height);
    memset(buffer->data, transparent_char, width * height);
    memset(buffer->color_data, 0x0f, width * height); // Default color
    memset(buffer->background_data, 0x00, width * height); // Default background
    return buffer;
}

void copy_img_to_buffer(Buffer *buffer, const char *img_data, char transparent_char, char pixel_char, uint8_t transparent_index, int img_width, int img_height) {
    for (int y = 0; y < img_height && y < buffer->height; y++) {
        for (int x = 0; x < img_width && x < buffer->width; x++) {
            int pos = y * img_width + x;
            int buffer_pos = y * buffer->width + x;
            if (img_data[pos] == transparent_index) {
                buffer->data[buffer_pos] = transparent_char;
                buffer->color_data[buffer_pos] = 0xFF; // Default color for transparent pixels
                continue;
            }
            buffer->data[buffer_pos] = pixel_char;
            buffer->color_data[buffer_pos] = img_data[pos];
        }
    }
}

void blit_to(Buffer *src, int x, int y, BufferType type) {
    switch (type) {
        case SCREEN:
            blit_to_screen(src, x, y);
            break;
        case BACKGROUND:
            blit_to_background(src, x, y);
            break;
        case COLOR:
            blit_to_color(src, x, y);
            break;
        case COLOR_BLEND_ADD:
            blit_add_to_screen(src, x, y);
            break;
        case COLOR_BLEND_SUB:
            blit_sub_to_screen(src, x, y);
            break;
        default:
            // Handle unknown buffer type if necessary
            break;
    }
}


void blit_to_screen(Buffer *buffer, int x, int y) {
    for (int by = 0; by < buffer->height; by++) {
        for (int bx = 0; bx < buffer->width; bx++) {
            int pos = by * buffer->width + bx;
            char pixel = buffer->data[pos];
            int px = x + bx;
            int py = y + by;
            if (px < 0 || py < 0 || px >= FRAME_WIDTH || py >= FRAME_HEIGHT) {
                continue; // Skip out-of-bounds pixels
            }
            if (pixel != buffer->transparent_char) {
                char_atc(px, py, buffer->color_data[pos], buffer->data[pos]);
            }
        }
    }
}

void copy_to_screen(Buffer *buffer, int x, int y) {
    for (int by = y; by < y + buffer->height; by++) {
        int address = by * FRAME_WIDTH + x;
        memcpy(&screen[address], &buffer->data[(by - y) * buffer->width], buffer->width);
        memcpy(&color_data[address], &buffer->color_data[(by - y) * buffer->width], buffer->width);
        memcpy(&background[address], &buffer->background_data[(by - y) * buffer->width], buffer->width);
    }
}

void blit_to_color(Buffer *buffer, int x, int y) {
    for (int by = 0; by < buffer->height; by++) {
        for (int bx = 0; bx < buffer->width; bx++) {
            int pos = by * buffer->width + bx;
            char pixel = buffer->data[pos];
            int px = x + bx;
            int py = y + by;
            if (px < 0 || py < 0 || px >= FRAME_WIDTH || py >= FRAME_HEIGHT) {
                continue; // Skip out-of-bounds pixels
            }
            if (pixel != buffer->transparent_char) {
                color_data[py * FRAME_WIDTH + px] = buffer->color_data[pos];
            }
        }
    }
}

void blit_to_background(Buffer *buffer, int x, int y) {
    for (int by = 0; by < buffer->height; by++) {
        for (int bx = 0; bx < buffer->width; bx++) {
            int pos = by * buffer->width + bx;
            char pixel = buffer->data[pos];
            int px = x + bx;
            int py = y + by;
            if (px < 0 || py < 0 || px >= FRAME_WIDTH || py >= FRAME_HEIGHT) {
                continue; // Skip out-of-bounds pixels
            }
            if (pixel != buffer->transparent_char) {
                background[py * FRAME_WIDTH + px] = buffer->color_data[pos];
            }
        }
    }
}

void char_buffer_atc(int x, int y, uint8_t color, char ch, Buffer *buffer)
{
    if (x >= buffer->width || y >= buffer->height)
        return;

    int pos = y * buffer->width + x;

    buffer->data[pos] = ch;
    buffer->color_data[pos] = color;
}

void init_big_characters(char pixel_char, char transparent_char, char color)
{
    for (int i = 0; i < 128; i++) {

        int width = 8;
        int height = 8;

        Buffer *b = &big_characters[i];

        b->width = width;
        b->height = height;
        b->pixel_char = pixel_char;
        b->transparent_char = transparent_char;

        b->data = malloc(width * height);
        b->color_data = malloc(width * height);

        memset(b->data, transparent_char, width * height);
        memset(b->color_data, 0, width * height);

        for (int y = 0; y < 8; y++) {
            uint8_t row = font8x8_basic[i][y];
            for (int x = 0; x < 8; x++) {

                char pixel = row & (1 << x)
                            ? pixel_char
                            : transparent_char;
                int pos = y * 8 + x;
                b->data[pos] = pixel;
                b->color_data[pos] = color;
            }
        }
    }
}


// Blit buffer to screen with additive color blending, only for non-transparent pixels, pixel is not affected
void blit_add_to_screen(Buffer *buffer, int x, int y) {
    for (int by = 0; by < buffer->height; by++) {
        for (int bx = 0; bx < buffer->width; bx++) {
            int pos = by * buffer->width + bx;
            int abspos = (y + by) * FRAME_WIDTH + (x + bx);
            char pixel = buffer->data[pos];
            int px = x + bx;
            int py = y + by;
            if (px < 0 || py < 0 || px >= FRAME_WIDTH || py >= FRAME_HEIGHT) {
                continue; // Skip out-of-bounds pixels
            }
            if (pixel != buffer->transparent_char) {
                char_atc(px, py, color_data[abspos] + buffer->color_data[pos], screen[abspos]);
            }
        }
    }
}

// Blit buffer to screen with subtractive color blending, only for non-transparent pixels, pixel is not affected
void blit_sub_to_screen(Buffer *buffer, int x, int y) {
    for (int by = 0; by < buffer->height; by++) {
        for (int bx = 0; bx < buffer->width; bx++) {
            int pos = by * buffer->width + bx;
            int abspos = (y + by) * FRAME_WIDTH + (x + bx);
            char pixel = buffer->data[pos];
            int px = x + bx;
            int py = y + by;
            if (px < 0 || py < 0 || px >= FRAME_WIDTH || py >= FRAME_HEIGHT) {
                continue; // Skip out-of-bounds pixels
            }
            if (pixel != buffer->transparent_char) {
                char_atc(px, py, color_data[abspos] - buffer->color_data[pos], screen[abspos]);
            }
        }
    }
}

void printf_big_atc(int x, int y, uint8_t color, BufferType type, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char text[512];
    vsnprintf(text, sizeof(text), fmt, args);

    va_end(args);

    int cursor_x = x;

    for (const char *p = text; *p; p++) {

        uint8_t ch = (uint8_t)*p;

        if (ch >= 128)
            continue;

        Buffer *glyph = &big_characters[ch];
        glyph->pixel_char = '#'; // Set pixel character for the glyph
        glyph->transparent_char = ' '; // Set transparent character for the glyph

        if (!glyph->data)
            continue;

        // opcionális: színezés
        memset(glyph->color_data, color,
               glyph->width * glyph->height);

        switch (type)
        {
        case SCREEN:
            blit_to_screen(glyph, cursor_x, y);
            break;
        case BACKGROUND:
            blit_to_background(glyph, cursor_x, y);
            break;
        case COLOR_BLEND_ADD:
            blit_add_to_screen(glyph, cursor_x, y);
            break;
        case COLOR_BLEND_SUB:
            blit_sub_to_screen(glyph, cursor_x, y);
            break;
        case COLOR:
            blit_to_color(glyph, cursor_x, y);
            break;
        default:
            break;
        }
        cursor_x += glyph->width;
    }
}

void printf_add_big_atc(int x, int y, uint8_t color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char text[512];
    vsnprintf(text, sizeof(text), fmt, args);

    va_end(args);

    int cursor_x = x;

    for (const char *p = text; *p; p++) {

        uint8_t ch = (uint8_t)*p;

        if (ch >= 128)
            continue;

        Buffer *glyph = &big_characters[ch];

        if (!glyph->data)
            continue;

        // opcionális: színezés
        memset(glyph->color_data, color,
               glyph->width * glyph->height);

        blit_add_to_screen(glyph, cursor_x, y);

        cursor_x += glyph->width;
    }
}

void printf_atc(int x, int y, uint8_t color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char text[512];
    vsnprintf(text, sizeof(text), fmt, args);

    va_end(args);

    int cursor_x = x;

    for (const char *p = text; *p; p++) {
        uint8_t ch = (uint8_t)*p;
        screen[(y) * FRAME_WIDTH + (cursor_x)] = ch;
        color_data[(y) * FRAME_WIDTH + (cursor_x)] = color;
        cursor_x++;
    }
}

void printf_sub_big_atc(int x, int y, uint8_t color, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char text[512];
    vsnprintf(text, sizeof(text), fmt, args);

    va_end(args);

    int cursor_x = x;

    for (const char *p = text; *p; p++) {

        uint8_t ch = (uint8_t)*p;

        if (ch >= 128)
            continue;

        Buffer *glyph = &big_characters[ch];

        if (!glyph->data)
            continue;

        // opcionális: színezés
        memset(glyph->color_data, color,
               glyph->width * glyph->height);

        blit_sub_to_screen(glyph, cursor_x, y);

        cursor_x += glyph->width;
    }
}


void scroll_buffer_left(Buffer *buffer, int lines) {
    if (lines <= 0 || lines >= buffer->width) {
        return; // Invalid number of lines to scroll
    }
    for (int y = 0; y < buffer->height; y++) {
        memmove(buffer->data + y * buffer->width, buffer->data + y * buffer->width + lines, buffer->width - lines);
        memset(buffer->data + y * buffer->width + (buffer->width - lines), buffer->transparent_char, lines);
        memmove(buffer->color_data + y * buffer->width, buffer->color_data + y * buffer->width + lines, buffer->width - lines);
        memset(buffer->color_data + y * buffer->width + (buffer->width - lines), 0x0f, lines); // Default color
    }
}

void scroll_buffer_right(Buffer *buffer, int lines) {
    if (lines <= 0 || lines >= buffer->width) {
        return; // Invalid number of lines to scroll
    }
    for (int y = 0; y < buffer->height; y++) {
        memmove(buffer->data + y * buffer->width + lines, buffer->data + y * buffer->width, buffer->width - lines);
        memset(buffer->data + y * buffer->width, buffer->transparent_char, lines);
        memmove(buffer->color_data + y * buffer->width + lines, buffer->color_data + y * buffer->width, buffer->width - lines);
        memset(buffer->color_data + y * buffer->width, 0x0f, lines); // Default color
    }
}

void scroll_buffer_up(Buffer *buffer, int lines) {
    if (lines <= 0 || lines >= buffer->height) {
        return; // Invalid number of lines to scroll
    }
    memmove(buffer->data, buffer->data + lines * buffer->width, (buffer->height - lines) * buffer->width);
    memset(buffer->data + (buffer->height - lines) * buffer->width, buffer->transparent_char, lines * buffer->width);
    memmove(buffer->color_data, buffer->color_data + lines * buffer->width, (buffer->height - lines) * buffer->width);
    memset(buffer->color_data + (buffer->height - lines) * buffer->width, 0x0f, lines * buffer->width); // Default color
}

void scroll_buffer_down(Buffer *buffer, int lines) {
    if (lines <= 0 || lines >= buffer->height) {
        return; // Invalid number of lines to scroll
    }
    memmove(buffer->data + lines * buffer->width, buffer->data, (buffer->height - lines) * buffer->width);
    memset(buffer->data, buffer->transparent_char, lines * buffer->width);
    memmove(buffer->color_data + lines * buffer->width, buffer->color_data, (buffer->height - lines) * buffer->width);
    memset(buffer->color_data, 0x0f, lines * buffer->width); // Default color
}

void fill_buffer_line(Buffer *buffer, int y, char ch) {
    if (y < 0 || y >= buffer->height) {
        return; // Invalid line number
    }
    memset(buffer->data + y * buffer->width, ch, buffer->width);
}

void set_palette(const uint8_t new_palette[][3], int src_len)
{
    if (src_len <= 0) return;
    for (int i = 0; i < 256; i++) {
        int j = i % src_len;
        palette[i][0] = new_palette[j][0];
        palette[i][1] = new_palette[j][1];
        palette[i][2] = new_palette[j][2];
    }
}

#endif // TERMINALNINJA_H