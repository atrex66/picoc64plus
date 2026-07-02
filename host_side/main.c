/*
 * c64terminal — host-side SDL2 renderer for the C64+ Pico emulator
 *
 * Connects to the Pico over USB-CDC serial, reads C64ScreenPacket frames,
 * renders a pixel-accurate C64 screen, and sends keystrokes back as PETSCII
 * bytes over the same serial port.
 *
 * Build:   make
 * Usage:   ./c64terminal /dev/ttyACM0 [scale]   (scale default = 2)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include "protocol.h"
#include "font.h"        // static const uint8_t CHARROM[256][8]

// ── Display constants ────────────────────────────────────────────────────────

#define CHAR_W      8
#define CHAR_H      8

// Window size: full 52×29 char frame, 8×8 pixels per char
#define NATIVE_W    (TOTAL_COLS * CHAR_W)   // 416 px
#define NATIVE_H    (TOTAL_ROWS * CHAR_H)   // 232 px

// ── SDL→PETSCII translation ──────────────────────────────────────────────────
//
// The Pico's tud_cdc_rx_cb already handles ASCII→PETSCII for most keys.
// We send raw bytes; the Pico's inject_key() puts them in the keyboard buffer.
// Special keys are sent as PETSCII control codes directly.

static uint8_t sdl_key_to_petscii(SDL_Keycode sym, SDL_Keymod mod) {
    bool shift = (mod & (KMOD_LSHIFT | KMOD_RSHIFT)) != 0;

    switch (sym) {
        // Printable ASCII — C64 PETSCII is mostly identical for 0x20-0x5F
        // Letters must be uppercased (C64 default charset is uppercase)
        case SDLK_SPACE:        return 0x20;
        case SDLK_RETURN:       return 0x0D;  // RETURN
        case SDLK_BACKSPACE:    return 0x14;  // DEL (PETSCII)
        case SDLK_DELETE:       return 0x14;
        case SDLK_INSERT:       return 0x94;  // INSERT (PETSCII)
        case SDLK_HOME:         return 0x13;  // HOME
        case SDLK_END:          return 0x93;  // CLR
        case SDLK_UP:           return 0x91;  // CURSOR UP
        case SDLK_DOWN:         return 0x11;  // CURSOR DOWN
        case SDLK_LEFT:         return 0x9D;  // CURSOR LEFT
        case SDLK_RIGHT:        return 0x1D;  // CURSOR RIGHT
        case SDLK_F1:           return shift ? 0x86 : 0x85;
        case SDLK_F2:           return shift ? 0x86 : 0x85;
        case SDLK_F3:           return shift ? 0x88 : 0x87;
        case SDLK_F4:           return shift ? 0x88 : 0x87;
        case SDLK_F5:           return shift ? 0x8A : 0x89;
        case SDLK_F6:           return shift ? 0x8A : 0x89;
        case SDLK_F7:           return shift ? 0x8C : 0x8B;
        case SDLK_F8:           return shift ? 0x8C : 0x8B;
        case SDLK_ESCAPE:       return 0;   // handled separately (quit)
        default: break;
    }

    // Letters: C64 PETSCII uppercase = ASCII value (A-Z = 0x41-0x5A)
    if (sym >= SDLK_a && sym <= SDLK_z) {
        uint8_t ch = (uint8_t)(sym - SDLK_a + 'A');  // always uppercase
        return ch;
    }

    // Digits and symbols: pass through if in printable ASCII range
    if (sym >= 0x20 && sym < 0x60) {
        return (uint8_t)sym;
    }

    return 0;  // unhandled
}

// ── Serial helpers ───────────────────────────────────────────────────────────

static int serial_open(const char *dev) {
    int fd = open(dev, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) { perror("open serial"); return -1; }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) { perror("tcgetattr"); close(fd); return -1; }

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag  = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
    tty.c_iflag  = IGNBRK;
    tty.c_lflag  = 0;
    tty.c_oflag  = 0;
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) { perror("tcsetattr"); close(fd); return -1; }
    return fd;
}

static bool serial_read_exact(int fd, void *buf, size_t n) {
    uint8_t *p = buf;
    size_t got = 0;
    while (got < n) {
        ssize_t r = read(fd, p + got, n - got);
        if (r <= 0) return false;
        got += (size_t)r;
    }
    return true;
}

// Scan the stream until the 4-byte frame magic is found
static bool sync_frame(int fd) {
    uint8_t buf[4] = {0};
    while (true) {
        uint8_t b;
        if (read(fd, &b, 1) != 1) return false;
        buf[0] = buf[1]; buf[1] = buf[2]; buf[2] = buf[3]; buf[3] = b;
        if (buf[0] == FRAME_MAGIC_0 && buf[1] == FRAME_MAGIC_1 &&
            buf[2] == FRAME_MAGIC_2 && buf[3] == FRAME_MAGIC_3)
            return true;
    }
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[]) {
    const char *dev = (argc >= 2) ? argv[1] : "/dev/ttyACM0";
    int scale = 2;
    if (argc >= 3) scale = atoi(argv[2]);
    if (scale < 1 || scale > 4) scale = 2;

    int win_w = NATIVE_W * scale;
    int win_h = NATIVE_H * scale;

    // ── Serial ───────────────────────────────────────────────────────────────
    int fd = serial_open(dev);
    if (fd < 0) return 1;
    printf("Connected to %s  (scale %d×)\n", dev, scale);

    // ── SDL ──────────────────────────────────────────────────────────────────
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1;
    }

    SDL_Window *win = SDL_CreateWindow("C64+ Terminal",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return 1; }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); return 1; }

    SDL_RenderSetLogicalSize(ren, win_w, win_h);

    SDL_Texture *tex = SDL_CreateTexture(ren,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, win_w, win_h);
    if (!tex) { fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError()); return 1; }

    uint32_t *pixels = calloc((size_t)(win_w * win_h), sizeof(uint32_t));
    if (!pixels) { fputs("OOM\n", stderr); return 1; }

    C64ScreenPacket pkt;
    memset(&pkt, 0, sizeof(pkt));

    bool cursor_blink = false;
    uint32_t last_blink = SDL_GetTicks();

    // ── Main loop ─────────────────────────────────────────────────────────────
    bool running = true;
    while (running) {

        // ── Events & keyboard sendback ───────────────────────────────────────
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running = false; break; }

            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) { running = false; break; }

                uint8_t petscii = sdl_key_to_petscii(
                    ev.key.keysym.sym, (SDL_Keymod)ev.key.keysym.mod);
                if (petscii != 0) {
                    // Send the PETSCII byte to the Pico over serial.
                    // The Pico's tud_cdc_rx_cb / inject_key handles it.
                    if (write(fd, &petscii, 1) != 1)
                        fputs("key send failed\n", stderr);
                }
            }
        }

        // ── Cursor blink (500 ms, independent of frame rate) ─────────────────
        uint32_t now = SDL_GetTicks();
        if (now - last_blink >= 500) { cursor_blink = !cursor_blink; last_blink = now; }

        // ── Receive next frame ────────────────────────────────────────────────
        if (!sync_frame(fd)) { fputs("serial sync lost\n", stderr); break; }

        memcpy(pkt.magic,
               (uint8_t[]){FRAME_MAGIC_0, FRAME_MAGIC_1, FRAME_MAGIC_2, FRAME_MAGIC_3}, 4);
        if (!serial_read_exact(fd, ((uint8_t *)&pkt) + 4, PACKET_SIZE - 4)) {
            fputs("serial read error\n", stderr); break;
        }

        // ── Render full 52×29 frame ───────────────────────────────────────────
        for (int row = 0; row < TOTAL_ROWS; row++) {
            for (int col = 0; col < TOTAL_COLS; col++) {
                int idx      = row * TOTAL_COLS + col;
                uint8_t code = pkt.screen[idx];
                const uint8_t *fgc = C64_PALETTE[pkt.color[idx]      & 0xF];
                const uint8_t *bgc = C64_PALETTE[pkt.background[idx]  & 0xF];
                uint32_t fg32 = (0xFFu<<24)|((uint32_t)fgc[0]<<16)|((uint32_t)fgc[1]<<8)|fgc[2];
                uint32_t bg32 = (0xFFu<<24)|((uint32_t)bgc[0]<<16)|((uint32_t)bgc[1]<<8)|bgc[2];

                int px = col * CHAR_W * scale;
                int py = row * CHAR_H * scale;

                const uint8_t *glyph = CHARROM[code];
                for (int r = 0; r < CHAR_H; r++) {
                    uint8_t bits = glyph[r];
                    for (int c = 0; c < CHAR_W; c++) {
                        uint32_t colour = (bits & (0x80 >> c)) ? fg32 : bg32;
                        for (int sy = 0; sy < scale; sy++)
                            for (int sx = 0; sx < scale; sx++)
                                pixels[(py+r*scale+sy)*win_w + (px+c*scale+sx)] = colour;
                    }
                }
            }
        }

        // ── Upload pixel buffer ───────────────────────────────────────────────
        SDL_UpdateTexture(tex, NULL, pixels, win_w * (int)sizeof(uint32_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);

        // ── Software cursor (SDL rect drawn on top, blinks independently) ─────
        if (pkt.cursor_on == 0 && cursor_blink) {
            // cursor_x/y are in full-frame coords (already include border offset)
            int cx = pkt.cursor_x * CHAR_W * scale;
            int cy = pkt.cursor_y * CHAR_H * scale;
            SDL_Rect cur = { cx, cy, CHAR_W * scale, CHAR_H * scale };
            int cidx = pkt.cursor_y * TOTAL_COLS + pkt.cursor_x;
            const uint8_t *cc = C64_PALETTE[pkt.color[cidx] & 0xF];
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(ren, cc[0], cc[1], cc[2], 255);
            SDL_RenderFillRect(ren, &cur);
        }

        SDL_RenderPresent(ren);
    }

    free(pixels);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    close(fd);
    return 0;
}

