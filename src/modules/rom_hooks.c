#include "rom_hooks.h"
#include "structs.h"


void keymatrix_scan(CPUState *state){
    state->halt_flag = true;  // Set the halt flag to indicate that the CPU is halted
    state->halt_reason = KEYBOARD_BUFFER_OVERFLOW_REASON;
}
