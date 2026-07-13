#include "6502_disasm.h"
#include "structs.h"

extern CPUState state;
extern uint8_t read_memory(CPUState *state, uint16_t address);


RoutineComment_t routine_comments[] = {
    {0xFFFC, "// Reset Vector"},
    {0xFFFE, "// IRQ Vector"},
    {0xFFFA, "// NMI Vector"},
    // Kernal routines
    {0xFF81, "// SCINIT. VIC I/O CLS IRQ TIMER"},
    {0xFF84, "// IOINIT. CIA,SID,MEM,IRQ"},
    {0xFF87, "// RAMTAS. ram test"},
    {0xFF8A, "// RESTOR. $0314-$0333 defaults"},
    {0xFF8D, "// VECTOR. init"},
    {0xFF90, "// SETMSG"},
    {0xFF93, "// LSTNSA"},
    {0xFF96, "// TALKSA"},
    {0xFF99, "// MEMTOP"},
    {0xFF9C, "// MEMBOT"},
    {0xFF9F, "// SCNKEY"},
    {0xFFA2, "// SETTMO"},
    {0xFFA5, "// IECIN"},
    {0xFFA8, "// IECOUT"},
    {0xFFAB, "// UNTALK"},
    {0xFFAE, "// UNLIST"},
    {0xFFB1, "// LISTEN"},
    {0xFFB4, "// TALK"},
    {0xFFB7, "// READST"},
    {0xFFBA, "// SETLFS"},
    {0xFFBD, "// SETNAM"},
    {0xFFC0, "// OPEN"},
    {0xFFC3, "// CLOSE"},
    {0xFFC6, "// CHKIN"},
    {0xFFC9, "// CHKOUT"},
    {0xFFCC, "// CLRCHN"},
    {0xFFCF, "// CHRIN"},
    {0xFFD2, "// CHROUT"},
    {0xFFD5, "// LOAD"},
    {0xFFD8, "// SAVE"},
    {0xFFDB, "// SETTIM"},
    {0xFFDE, "// RDTIM"},
    {0xFFE1, "// STOP"},
    {0xFFE4, "// GETIN"},
    {0xFFE7, "// CLALL"},
    {0xFFEA, "// UDTIM"},
    {0xFFED, "// SCREEN"},
    {0xFFF0, "// PLOT"},
    {0xFFF3, "// IOBASE"},
};


Instruction_t instructions[256] = {
    [0x00] = { "BRK", 1 , false},
    [0x01] = { "ORA ($%02X,X)", 2 , false},
    [0x05] = { "ORA $%02X", 2 , false},
    [0x06] = { "ASL $%02X", 2 , false},
    [0x08] = { "PHP", 1 , false},
    [0x09] = { "ORA #$%02X", 2 , false},
    [0x0A] = { "ASL A", 1 , false},
    [0x0D] = { "ORA $%04X", 3 , false},
    [0x0E] = { "ASL $%04X", 3 , false},
    [0x10] = { "BPL $%02X", 2 , true},
    [0x11] = { "ORA ($%02X),Y", 2 , false},
    [0x15] = { "ORA $%02X,X", 2 , false},
    [0x16] = { "ASL $%02X,X", 2 , false},
    [0x18] = { "CLC", 1 , false},
    [0x19] = { "ORA $%04X,Y", 3 , false},
    [0x1D] = { "ORA $%04X,X", 3 , false},
    [0x1E] = { "ASL $%04X,X", 3 , false},
    [0x20] = { "JSR $%04X", 3 , false},    
    [0x21] = { "AND ($%02X,X)", 2 , false},
    [0x24] = { "BIT $%02X", 2 , false},
    [0x25] = { "AND $%02X", 2 , false},
    [0x26] = { "ROL $%02X", 2 , false},
    [0x28] = { "PLP", 1 , false},
    [0x29] = { "AND #$%02X", 2 , false},
    [0x2A] = { "ROL A", 1 , false},
    [0x2C] = { "BIT $%04X", 3 , false},
    [0x2D] = { "AND $%04X", 3 , false},
    [0x2E] = { "ROL $%04X", 3 , false},
    [0x30] = { "BMI $%02X", 2 , true},
    [0x31] = { "AND ($%02X),Y", 2 , false},
    [0x35] = { "AND $%02X,X", 2 , false},
    [0x36] = { "ROL $%02X,X", 2 , false},
    [0x38] = { "SEC", 1 , false},
    [0x39] = { "AND $%04X,Y", 3 , false},
    [0x3D] = { "AND $%04X,X", 3 , false},
    [0x3E] = { "ROL $%04X,X", 3 , false},
    [0x40] = { "RTI", 1 , false},
    [0x41] = { "EOR ($%02X,X)", 2 , false},
    [0x45] = { "EOR $%02X", 2 , false},
    [0x46] = { "LSR $%02X", 2 , false},
    [0x48] = { "PHA", 1 , false},
    [0x49] = { "EOR #$%02X", 2 , false},
    [0x4A] = { "LSR A", 1 , false},
    [0x4C] = { "JMP $%04X", 3 , false},    
    [0x4D] = { "EOR $%04X", 3 , false},
    [0x4E] = { "LSR $%04X", 3 , false},
    [0x50] = { "BVC $%02X", 2 , true},
    [0x51] = { "EOR ($%02X),Y", 2 , false},
    [0x55] = { "EOR $%02X,X", 2 , false},
    [0x56] = { "LSR $%02X,X", 2 , false},
    [0x58] = { "CLI", 1 , false},
    [0x59] = { "EOR $%04X,Y", 3 , false},
    [0x5D] = { "EOR $%04X,X", 3 , false},
    [0x5E] = { "LSR $%04X,X", 3 , false},
    [0x60] = { "RTS", 1 , false},
    [0x61] = { "ADC ($%02X,X)", 2 , false},
    [0x65] = { "ADC $%02X", 2 , false},
    [0x66] = { "ROR $%02X", 2 , false},
    [0x68] = { "PLA", 1 , false},
    [0x69] = { "ADC #$%02X", 2 , false},
    [0x6A] = { "ROR A", 1 , false},
    [0x6C] = { "JMP ($%04X)", 3 , false},
    [0x6D] = { "ADC $%04X", 3 , false},
    [0x6E] = { "ROR $%04X", 3 , false},
    [0x70] = { "BVS $%02X", 2 , true},
    [0x71] = { "ADC ($%02X),Y", 2 , false},
    [0x75] = { "ADC $%02X,X", 2 , false},
    [0x76] = { "ROR $%02X,X", 2 , false},
    [0x78] = { "SEI", 1 , false},
    [0x79] = { "ADC $%04X,Y", 3 , false},
    [0x7D] = { "ADC $%04X,X", 3 , false},
    [0x7E] = { "ROR $%04X,X", 3 , false},
    [0x81] = { "STA ($%02X,X)", 2 , false},
    [0x84] = { "STY $%02X", 2 , false},
    [0x85] = { "STA $%02X", 2 , false},
    [0x86] = { "STX $%02X", 2 , false},
    [0x88] = { "DEY", 1 , false},
    [0x8A] = { "TXA", 1 , false},
    [0x8C] = { "STY $%04X", 3 , false},
    [0x8D] = { "STA $%04X", 3 , false},
    [0x8E] = { "STX $%04X", 3 , false},
    [0x90] = { "BCC $%02X", 2 , true},
    [0x91] = { "STA ($%02X),Y", 2 , false},
    [0x94] = { "STY $%02X,X", 2 , false},
    [0x95] = { "STA $%02X,X", 2 , false},
    [0x96] = { "STX $%02X,Y", 2 , false},
    [0x98] = { "TYA", 1 , false},
    [0x99] = { "STA $%04X,Y", 3 , false},
    [0x9A] = { "TXS", 1 , false},
    [0x9D] = { "STA $%04X,X", 3 , false},
    [0xA0] = { "LDY #$%02X", 2 , false},
    [0xA1] = { "LDA ($%02X,X)", 2 , false},
    [0xA2] = { "LDX #$%02X", 2 , false},
    [0xA4] = { "LDY $%02X", 2 , false},
    [0xA5] = { "LDA $%02X", 2 , false},
    [0xA6] = { "LDX $%02X", 2 , false},
    [0xA8] = { "TAY", 1 , false},
    [0xA9] = { "LDA #$%02X", 2 , false},
    [0xAA] = { "TAX", 1 , false},
    [0xAC] = { "LDY $%04X", 3 , false},
    [0xAD] = { "LDA $%04X", 3 , false},
    [0xAE] = { "LDX $%04X", 3 , false},
    [0xB0] = { "BCS $%02X", 2 , true},
    [0xB1] = { "LDA ($%02X),Y", 2 , false},
    [0xB4] = { "LDY $%02X,X", 2 , false},
    [0xB5] = { "LDA $%02X,X", 2 , false},
    [0xB6] = { "LDX $%02X,Y", 2 , false},
    [0xB8] = { "CLV", 1 , false},
    [0xB9] = { "LDA $%04X,Y", 3 , false},
    [0xBA] = { "TSX", 1 , false},
    [0xBC] = { "LDY $%04X,X", 3 , false},
    [0xBD] = { "LDA $%04X,X", 3 , false},
    [0xBE] = { "LDX $%04X,Y", 3 , false},
    [0xC0] = { "CPY #$%02X", 2 , false},
    [0xC1] = { "CMP ($%02X,X)", 2 , false},
    [0xC4] = { "CPY $%02X", 2 , false},
    [0xC5] = { "CMP $%02X", 2 , false},
    [0xC6] = { "DEC $%02X", 2 , false},
    [0xC8] = { "INY", 1 , false},
    [0xC9] = { "CMP #$%02X", 2 , false},
    [0xCA] = { "DEX", 1 , false},  
    [0xCC] = { "CPY $%04X", 3 , false},
    [0xCD] = { "CMP $%04X", 3 , false},
    [0xCE] = { "DEC $%04X", 3 , false},
    [0xD0] = { "BNE $%02X", 2 , true},
    [0xD1] = { "CMP ($%02X),Y", 2 , false},
    [0xD5] = { "CMP $%02X,X", 2 , false},
    [0xD6] = { "DEC $%02X,X", 2 , false},
    [0xD8] = { "CLD", 1 , false},
    [0xD9] = { "CMP $%04X,Y", 3 , false},
    [0xDD] = { "CMP $%04X,X", 3 , false},
    [0xDE] = { "DEC $%04X,X", 3 , false},
    [0xE0] = { "CPX #$%02X", 2 , false},
    [0xE1] = { "SBC ($%02X,X)", 2 , false},
    [0xE4] = { "CPX $%02X", 2 , false},
    [0xE5] = { "SBC $%02X", 2 , false},
    [0xE6] = { "INC $%02X", 2 , false},
    [0xE8] = { "INX", 1 , false},
    [0xE9] = { "SBC #$%02X", 2 , false},
    [0xEA] = { "NOP", 1 , false},
    [0xEC] = { "CPX $%04X", 3 , false},
    [0xED] = { "SBC $%04X", 3 , false},
    [0xEE] = { "INC $%04X", 3 , false},
    [0xF0] = { "BEQ $%02X", 2 , true},
    [0xF1] = { "SBC ($%02X),Y", 2 , false},
    [0xF5] = { "SBC $%02X,X", 2 , false},
    [0xF6] = { "INC $%02X,X", 2 , false},
    [0xF8] = { "SED", 1 , false},
    [0xF9] = { "SBC $%04X,Y", 3 , false},
    [0xFD] = { "SBC $%04X,X", 3 , false},
    [0xFE] = { "INC $%04X,X", 3 , false},
};

char *decode_instruction(uint16_t address) {
    uint8_t opcode = read_memory(&state, address);
    Instruction_t instr = instructions[opcode];
    
    if (instr.format == NULL) {
        return "???";
    }

    uint16_t target_address = 0;
    static char buffer[40];

    if (instr.length == 1) {
        snprintf(buffer, sizeof(buffer), "%s", instr.format);
    } else if (instr.length == 2) {
        if (instr.adressing_mode) {
            int8_t offset = (int8_t)read_memory(&state, address + 1);
            target_address = address + 2 + offset;
            snprintf(buffer, sizeof(buffer), instr.format, target_address);
        } else {
            uint8_t operand = read_memory(&state, address + 1);
            snprintf(buffer, sizeof(buffer), instr.format, operand);
        }
    } else if (instr.length == 3) {
        uint16_t operand = read_memory(&state, address + 1) | (read_memory(&state, address + 2) << 8);
        target_address = operand;
        snprintf(buffer, sizeof(buffer), instr.format, operand);
    }
    return buffer;
}

void hex_view(uint16_t start_address, uint16_t end_address) {
    for (uint16_t addr = start_address; addr <= end_address; addr += 16) {
        printf("$%04X: ", addr);
        for (int i = 0; i < 16; i++) {
            if (addr + i <= end_address) {
                printf("%02X ", read_memory(&state, addr + i));
            } else {
                printf("   ");
            }
        }
        printf(" |");
        for (int i = 0; i < 16; i++) {
            if (addr + i <= end_address) {
                char c = read_memory(&state, addr + i);
                if (c >= 32 && c <= 126) {  
                    putchar(c);
                } else {
                    putchar('.');
                }
            }
        }
        printf("|\n");
    }
}