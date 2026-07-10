#ifndef ROM_HOOKS_H
#define ROM_HOOKS_H

#include "structs.h"
#include <stdint.h>
#include "enums.h"

extern KernalHook_t hooked_address[32];
void keymatrix_scan(CPUState *state);  // Function prototype for the keymatrix scan hook

#endif // ROM_HOOKS_H
