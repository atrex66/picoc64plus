#ifndef _6502_DISASM_H_
#define _6502_DISASM_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char *format;
    uint8_t length;
    bool adressing_mode;  // false for absolute, true for relative
} Instruction_t;

typedef struct {
    uint16_t address;
    char *comment;
} RoutineComment_t;

char *decode_instruction(uint16_t address);
void hex_view(uint16_t start_address, uint16_t end_address);

#endif