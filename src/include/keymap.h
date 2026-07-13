#ifndef TERMINAL_KEYS_H
#define TERMINAL_KEYS_H

typedef struct {
    const char *seq;
    const char c64_keycode;
} terminal_key_t;

static const terminal_key_t terminal_keys[] = {
    { "\x1b[A",  0x91 },  
    { "\x1b[B",  0x11 },  
    { "\x1b[C",  0x1d },  
    { "\x1b[D",  0x9d },  

    { "\x1b[H",  0x13 },  
    { "\x1b[F",  0x03 },  

    { "\x1b[2~", 0x94 }, 
    { "\x1b[3~", 0x14 }, 
    { "\x1b[5~", 0x18 }, 
    { "\x1b[6~", 0x92 }, 

    { "\x1bOP",  0x85 }, 
    { "\x1bOQ",  0xff }, 
    { "\x1bOR",  0x87 }, 
    { "\x1bOS",  0x88 }, 

    { "\x1b[15~", 0x85 }, 
    { "\x1b[16~", 0x86 },
    { "\x1b[17~", 0x87 }, 
    { "\x1b[18~", 0x88 }, 
    { "\x1b[19~", 0xfe }, 
    { "\x1b[20~", 0xff }, 
    { "\x1b[21~", 0x8a }, 
    { "\x1b[23~", 0x8B }, 
    { "\x1b[24~", 0x8C },
    { "{UP}",  0x91 },  
    { "{DOWN}",  0x11 },  
    { "{LEFT}",  0x9d },  
    { "{RIGHT}",  0x1d },
    { "{CURSOR LEFT}",  0x9d },  
    { "{CURSOR RIGHT}",  0x1d },  
    { "{HOME}",  0x13 },  
    { "{END}",  0x03 },  

    { "{INS}", 0x94 }, 
    { "{DEL}", 0x14 }, 
    { "{CLS}", 0x93 }, 
    { "{CLR}", 0x93 }, 
    { "{HOME}", 0x13 }, 
    { "{END}", 0x03 }, 
    { "{INS}", 0x94 }, 
    { "{DEL}", 0x14 }, 
    { "{RVS ON}", 0x18 }, 
    { "{RVS OFF}", 0x92 }, 
    { "{BLACK}", 0x90 }, 
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
    { "{UP}",  0x91 },  
    { "{DOWN}",  0x11 },  
    { "{RIGHT}",  0x1d },  
    { "{LEFT}",  0x9d },  

    { NULL, 0 }
};

#endif