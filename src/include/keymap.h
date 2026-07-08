#ifndef TERMINAL_KEYS_H
#define TERMINAL_KEYS_H

typedef struct {
    const char *seq;
    const char c64_keycode;
} terminal_key_t;

static const terminal_key_t terminal_keys[] = {
    { "\x1b[A",  0x91 },  // cursor up
    { "\x1b[B",  0x11 },  // cursor down
    { "\x1b[C",  0x1d },  // cursor right
    { "\x1b[D",  0x9d },  // cursor left

    { "\x1b[H",  0x13 },  // home
    { "\x1b[F",  0x03 },  // end

    { "\x1b[2~", 0x94 }, // insert
    // { "\x1b[3~", 0x14 }, // delete
    { "\x1b[5~", 0x18 }, // page up (RVS ON)
    { "\x1b[6~", 0x92 }, // page down (RVS OFF)

    { "\x1bOP",  0x85 }, // F1
    { "\x1bOQ",  0x86 }, // F2
    { "\x1bOR",  0x87 }, // F3
    { "\x1bOS",  0x88 }, // F4

    { "\x1b[15~", 0x85 }, // F5
    { "\x1b[17~", 0x86 }, // F6
    { "\x1b[18~", 0x87 }, // F7
    { "\x1b[19~", 0x88 }, // F8
    { "\x1b[20~", 0x0E }, // lo/hi charset (F9)
    { "\x1b[21~", 0x8E }, // up/gfx charset (F10)
    { "\x1b[23~", 0x8B }, 
    { "\x1b[24~", 0x8C },
    { "{UP}",  0x91 },  // cursor up
    { "{DOWN}",  0x11 },  // cursor down
    { "{LEFT}",  0x9d },  // cursor left
    { "{RIGHT}",  0x1d },
    { "{CURSOR LEFT}",  0x9d },  // cursor left
    { "{CURSOR RIGHT}",  0x1d },  // cursor right
    { "{HOME}",  0x13 },  // home
    { "{END}",  0x03 },  // end

    { "{INS}", 0x94 }, // insert
    { "{DEL}", 0x14 }, // delete
    { "{CLS}", 0x93 }, // clear screen
    { "{CLR}", 0x93 }, // clear screen
    { "{HOME}", 0x13 }, // home
    { "{END}", 0x03 }, // end
    { "{INS}", 0x94 }, // insert
    { "{DEL}", 0x14 }, // delete
    { "{RVS ON}", 0x18 }, // page up (RVS ON)
    { "{RVS OFF}", 0x92 }, // page down (RVS OFF)
    { "{BLACK}", 0x90 }, // black color
    { "{BROWN}", 0x95},
    { "{PINK}", 0x96},
    { "{DARKGREY}", 0x97},
    { "{GREY}", 0x98},
    { "{LIGHTGREEN}", 0x99},
    { "{LIGHTBLUE}", 0x9A},
    { "{LIGHTGREY}", 0x9B},
    { "{GRAY1}", 0x9B},
    { "{PURPLE}", 0x9C},
    { "{YELLOW}", 0x9E},
    { "{CYAN}", 0x9F},
    { "{RED}", 0x1C},
    { "{GREEN}", 0x1E},
    { "{BLUE}", 0x1F},
    { "{ORANGE}", 0x81},
    { "{WHITE}", 0x05},
    { "{UP}",  0x91 },  // cursor up
    { "{DOWN}",  0x11 },  // cursor down
    { "{RIGHT}",  0x1d },  // cursor right
    { "{LEFT}",  0x9d },  // cursor left

    { NULL, 0 }
};

#endif