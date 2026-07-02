#include <stdint.h>
#include "sintable.h"

#define COLOR_RAM ((volatile uint8_t*)0xD800)
#define SCREEN ((volatile uint8_t*)0x0400)
#define COLOR_MODE ((volatile uint8_t*)0xD02f)
#define FRAME_READY ((volatile uint8_t*)0x02A7)


int main(void)
{
    uint8_t x, y;
    uint8_t t = 0;

    *COLOR_MODE = 1; // set to 16 color mode    

    // init screen
    for (y = 0; y < 25; y++)
    {
        for (x = 0; x < 40; x++)
        {
            SCREEN[x + y * 40] = 0x48;
        }
    }

    while (1)
    {
        for (y = 5; y < 21; y++)
        {
            for (x = 10; x < 31; x++)
            {
                uint8_t v =
                    sintab[(x * 6 + t) & 255] +
                    sintab[(y * 9 + t * 2) & 255];

                COLOR_RAM[x + y * 40] = colortab[v];
            }
        }

        t++;
    }
    return 0;
}