#include <stdint.h>
#include <stdio.h>
#include "sintable.h"

#define COLOR_RAM ((volatile uint8_t*)0x9000)
#define FRAME_READY ((volatile uint8_t*)0xd013)
#define SCREEN ((volatile uint8_t*)0x0400)
#define COLOR_MODE ((volatile uint8_t*)0xD02f)
#define SPRITE_WIDTH ((volatile uint8_t*)0xa000)
#define SPRITE_HEIGHT ((volatile uint8_t*)0xa001)
#define SPRITE_ENABLED ((volatile uint8_t*)0xa002)
#define SPRITE_X ((volatile int16_t*)0xa003)
#define SPRITE_Y ((volatile int16_t*)0xa005)
#define SPRITE_TRANSPARENCY_COLOR ((volatile uint8_t*)0xa007)
#define SPRITE_PIXEL_DATA ((volatile uint16_t*)0xa008)

int main(void)
{
    uint8_t x, y;
    uint8_t t = 0;
    uint8_t w = 52;
    uint8_t h = 58;

    *SPRITE_WIDTH = w; // set sprite width
    *SPRITE_HEIGHT = h; // set sprite height

    *SPRITE_X = (int16_t)0; // set sprite X position
    *SPRITE_Y = (int16_t)0; // set sprite Y position
    *SPRITE_ENABLED = 1; // enable sprite
    *SPRITE_TRANSPARENCY_COLOR = 0; // set transparency color to 0
    *SPRITE_PIXEL_DATA = (uint16_t)0x9000; // set sprite pixel data to color RAM

    printf("PLEASE WAIT FOR THE PLASMA EFFECT TO LOAD...\n");

    // init screen
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            COLOR_RAM[x + y * w] = 0x00;
        }
    }

    *FRAME_READY = 0; // clear frame ready flag
    while (*FRAME_READY == 0) {} // wait for frame ready flag

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            uint8_t v =
                sintab[(x * 6 + t) & 255] +
                sintab[(y * 9 + t * 2) & 255];

            COLOR_RAM[x + y * w] = colortab[v];
        }
    }

    return 0;
}