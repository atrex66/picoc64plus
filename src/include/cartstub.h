#ifndef CARTSTUB_H
#define CARTSTUB_H
#include <stdint.h>

// Cartridge autostart stub — maps at $8000 in the 6502 address space.
//
// The C64 KERNAL cold-start routine reads bytes $8004-$8008 looking for
// the CBM80 signature ($C3 $C2 $CD $38 $30). When found it JSRs through
// the cold vector at $8000/$8001. We install the BASIC extension vectors
// at $C000 and then RTS so the KERNAL continues its normal init sequence.
//
// IMPORTANT: the KERNAL JSRs to the cold vector — it expects an RTS back.
// A JMP $E394 would skip the rest of KERNAL init and hang the machine.
//
// Full layout (13 bytes, $8000-$800C):
//
//   Offset  Address  Bytes        Meaning
//   ------  -------  -----------  -------
//    0- 1   $8000    09 80        Cold vector lo/hi -> ColdStart $8009
//    2- 3   $8002    0C 80        Warm vector lo/hi -> WarmStart $800C
//    4- 8   $8004    C3 C2 CD 38 30  CBM80 signature
//    9-11   $8009    20 00 C0     ColdStart: JSR $C000
//      12   $800C    60           ColdStart: RTS  (KERNAL continues init)
//   13-15   $800D    20 00 C0     WarmStart: JSR $C000
//      16   $8010    60           WarmStart: RTS

#define CART_STUB_START_ADDRESS 0x8000
#define CART_STUB_SIZE          17

static const uint8_t CART_STUB[CART_STUB_SIZE] = {
    /* $8000 */ 0x09, 0x80,              // cold vector lo/hi -> ColdStart $8009
    /* $8002 */ 0x0D, 0x80,              // warm vector lo/hi -> WarmStart $800D
    /* $8004 */ 0xC3, 0xC2, 0xCD, 0x38, 0x30,  // CBM80 signature
    /* $8009 */ 0x20, 0x00, 0xC0,        // ColdStart: JSR $C000
    /* $800C */ 0x60,                    // ColdStart: RTS  -> KERNAL finishes init
    /* $800D */ 0x20, 0x00, 0xC0,        // WarmStart: JSR $C000
    /* $8010 */ 0x60,                    // WarmStart: RTS
};

#endif // CARTSTUB_H
