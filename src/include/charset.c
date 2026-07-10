#include <stdio.h>
#include <stdint.h>
#include "c64basic_emu.h"   /* extern const char *c64_to_utf8[256]; */
#define _XOPEN_SOURCE_EXTENDED 1
#include <locale.h>
#include <ncursesw/ncurses.h>

int main(void)
{
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();

    mvprintw(0, 0, "     ");
    for (int c = 0; c < 16; ++c)
        mvprintw(0, 5 + c * 5, "%X", c);

    for (int r = 0; r < 16; ++r) {
        mvprintw(r + 1, 0, "%02X:", r << 4);

        for (int c = 0; c < 16; ++c) {
            int code = (r << 4) | c;
            mvaddstr(r + 1, 5 + c * 5, c64_to_utf8[code]);
        }
    }

    mvprintw(18, 0, "Nyomj meg egy billentyut...");
    refresh();
    getch();
    endwin();

    return 0;
}