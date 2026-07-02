#include <stdio.h>
#include "6502_opcodes.h"
#include "c64-roms.h"
#include "basicext.h"
#include "cartstub.h"

extern volatile bool runstop_pressed;  // set by tud_cdc_rx_cb on CTRL+C / ESC

CPUState cpu_state;
uint8_t memory[MEMORY_SIZE];
uint8_t temp;
uint16_t temp_address;

#define basic_start_address     0xA000 // size:8192
#define kernal_start_address    0xE000 // size:8192
#define charrom_start_address   0xD000 // size:4096
#define cart_low_start_address  0x8000 // size:?
#define cart_high_start_address 0xE000 // size:?

uint8_t read_memory(CPUState *state, uint16_t address) {

    uint32_t kstart = kernal_start_address;
    uint32_t kend = kstart + 8192;
    uint32_t bstart = basic_start_address;
    uint32_t bend = bstart + 8192;

    #ifdef IO_START    // for later use, if we want to emulate the IO area under the ROM address space (C16, C+4)
    if (address >= IO_START && address <= IO_END) {
        // For now, just return the value from memory
        return memory[address];
    }
    #endif

    if (address == 0xD03f){
        return 0x80; // Return 0 for the VIC-II's raster register to prevent raster interrupts
    }
    // CIA1 Port B ($DC01) — keyboard matrix row output
    // The KERNAL scans columns by writing to $DC00 and reads rows from $DC01.
    // RUN/STOP is in column 7 (bit 7 of $DC00 = 0), row 7 (bit 7 of $DC01 = 0).
    if (address == 0xDC01) {
        uint8_t col_select = memory[0xDC00];  // active columns (0 = selected)
        uint8_t result = 0xFF;                // all rows high = no key pressed
        if ((col_select & 0x80) == 0) {       // column 7 selected
            if (runstop_pressed) result &= ~0x80;  // row 7 low = RUN/STOP held
        }
        return result;
    }

    // The basic extension is mapped to $C000-$CFFF, so we need to check if the address falls within that range and return the corresponding byte from BASIC_EXTENSION.
    if (address >= BASICEXT_PRG_START_ADDRESS && address < BASICEXT_PRG_START_ADDRESS + BASICEXT_PRG_SIZE) {
        return BASICEXT_PRG_DATA[address - BASICEXT_PRG_START_ADDRESS];
    }

    // Cartridge autostart stub at $8000: CBM80 signature + cold/warm vectors.
    // ColdStart does JSR $C000 (installs BASIC extension vectors) then RTS so
    // the KERNAL continues its normal init — BASIC extension is active on boot.
    //if (address >= CART_STUB_START_ADDRESS && address < CART_STUB_START_ADDRESS + CART_STUB_SIZE) {
    //    return CART_STUB[address - CART_STUB_START_ADDRESS];
    //}

    if (address >= bstart && address < bend)
    {
        return dump_c64_basic_bin[address - basic_start_address];
    }
    if (address >= kstart && address < kend)
    {
        return dump_c64_kernalv3_bin[address - kernal_start_address];
    }
    return memory[address];
}



void write_memory(CPUState *state, uint16_t address, uint8_t value) {
    
    if (address >= basic_start_address && address < basic_start_address + 8192) {
        return;
    }
    if (address >= kernal_start_address && address < kernal_start_address + 8192) {
        return;
    }

    // $D000-$DFFF: I/O area (VIC-II, SID, CIA1, CIA2) — writes go to read_memory(state,]
    // so that the KERNAL can configure these registers and read them back.
    memory[address] = value;

    // Mark screen dirty when default screen RAM ($0400-$07E7) is touched
    if (address >= 0x0400 && address < 0x07E8) {
        state->dirty_screen = true;
    }

}

void reset_cpu(CPUState *state) {
    state->accumulator = 0;
    state->x_register = 0;
    state->y_register = 0;
    state->stack_pointer = 0xFF; // Stack pointer starts at 0xFF
    memory[0x0001] = 0b00110111; // Set default value for processor port ($01)
    memory[0xD02F] = 0x00; // Set default palette for terminal screen (if not 0 switch to 256 color mode)
    state->reset_vector = read_memory(state, 0xFFFC) | (read_memory(state, 0xFFFD) << 8); // Read reset vector
    state->irq_vector = read_memory(state, 0xFFFE) | (read_memory(state, 0xFFFF) << 8); // Read IRQ vector
    state->nmi_vector = read_memory(state, 0xFFFA) | (read_memory(state, 0xFFFB) << 8); // Read NMI vector
    state->program_counter = state->reset_vector; // Set program counter to reset vector
    state->carry_flag = false; // Clear status flags
    state->zero_flag = false;
    state->interrupt_disable = true; // 6502 sets I on reset
    state->decimal_mode = false;
    state->overflow_flag = false;
    state->negative_flag = false;
    state->break_command = false;
    state->IRQ_input = false;
    state->NMI_input = false;
    // cartridge.enabled = false; // Disable cartridge on reset
    // cartridge.bank = 0; // Reset cartridge bank to 0
    // cartridge_init(); // Initialize cartridge
    #if debug
    printf("CPU Reset: PC=%04X SP=%02X\n", state->program_counter, state->stack_pointer);
    #endif
}


void BCD_ADD(CPUState *state, uint8_t value) {
    // 1. Az NMOS 6502-nél a Z, N és V flagek a BINÁRIS eredményből számítódnak!
    uint16_t binary_sum = state->accumulator + value + (state->carry_flag ? 1 : 0);
    state->zero_flag = ((binary_sum & 0xFF) == 0);
    state->negative_flag = ((binary_sum & 0x80) != 0);
    state->overflow_flag = (~(state->accumulator ^ value) & (state->accumulator ^ binary_sum) & 0x80) != 0;

    // 2. Decimális (BCD) korrekció az Akkumulátornak
    int low_nibble = (state->accumulator & 0x0F) + (value & 0x0F) + (state->carry_flag ? 1 : 0);
    if (low_nibble > 9) {
        low_nibble += 6;
    }
    
    int high_nibble = (state->accumulator >> 4) + (value >> 4) + (low_nibble > 15 ? 1 : 0);
    if (high_nibble > 9) {
        high_nibble += 6;
    }
    
    // A Carry flag-et a felső nibble túlcsordulása határozza meg decimális módban
    state->carry_flag = (high_nibble > 15);
    
    // Végeredmény beírása az akkumulátorba
    state->accumulator = ((high_nibble << 4) & 0xF0) | (low_nibble & 0x0F);
}

void BCD_SUB(CPUState *state, uint8_t value) {
    // 1. A Z, N és V flagek itt is a BINÁRIS kivonás alapján dőlnek el!
    uint16_t binary_diff = state->accumulator - value - (state->carry_flag ? 0 : 1);
    state->zero_flag = ((binary_diff & 0xFF) == 0);
    state->negative_flag = ((binary_diff & 0x80) != 0);
    state->overflow_flag = ((state->accumulator ^ value) & (state->accumulator ^ binary_diff) & 0x80) != 0;

    // 2. Decimális (BCD) korrekció kölcsönkérésekkel
    int low_nibble = (state->accumulator & 0x0F) - (value & 0x0F) - (state->carry_flag ? 0 : 1);
    if (low_nibble < 0) {
        low_nibble -= 6;
    }
    
    int high_nibble = (state->accumulator >> 4) - (value >> 4) + (low_nibble < 0 ? -1 : 0);
    if (high_nibble < 0) {
        high_nibble -= 6;
    }
    
    // 6502 Carry szabály kivonásnál: 1 ha nem volt borrow (eredmény >= 0), 0 ha volt borrow
    state->carry_flag = (binary_diff <= 0xFF); 
    
    // Végeredmény beírása az akkumulátorba
    state->accumulator = ((high_nibble << 4) & 0xF0) | (low_nibble & 0x0F);

}


void ADC_IMMEDIATE(CPUState *state) {  // eg: ADC #$01
    uint8_t value = read_memory(state, state->program_counter + 1); // Mindig olvassuk ki a bájtot

    if (state->decimal_mode) {
        BCD_ADD(state, value);
    } else {
        // 1. Kiszámoljuk a 16 bites összeget (A + érték + Carry)
        uint16_t sum = state->accumulator + value + (state->carry_flag ? 1 : 0);
        
        // 2. Beállítjuk a Carry flag-et (ha nagyobb, mint 255)
        state->carry_flag = (uint8_t)(sum > 0xFF);
        
        // 3. Beállítjuk az Overflow (V) flag-et (előjeles túlcsordulás)
        state->overflow_flag = (~(state->accumulator ^ value) & (state->accumulator ^ sum) & 0x80) != 0;
        
        // 4. Elmentjük a 8 bites végeredményt az akkumulátorba
        state->accumulator = (uint8_t)(sum & 0xFF);
        
        // 5. FRISSÍTJÜK A HIÁNYZÓ FLAGEKET a 8 bites eredmény alapján!
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    
    state->program_counter += 2; // Továbblépünk a memóriában
    
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ADC #$%02X", state->program_counter - 2, value);
    #endif
}


void ADC_ZEROPAGE(CPUState *state) {   // eg: ADC $00
    uint8_t zp_address = read_memory(state, state->program_counter + 1);
    uint8_t value = read_memory(state, zp_address);

    if (state->decimal_mode) {
        BCD_ADD(state, value);
    } else {
        uint16_t sum = state->accumulator + value + (state->carry_flag ? 1 : 0);
        state->carry_flag = (uint8_t)(sum > 0xFF);
        state->overflow_flag = (~(state->accumulator ^ value) & (state->accumulator ^ sum) & 0x80) != 0;
        state->accumulator = (uint8_t)(sum & 0xFF);
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ADC $%02X", state->program_counter - 2, zp_address);
    #endif
}

void ADC_ZEROPAGE_X(CPUState *state) {  // eg: ADC $00,X
    uint8_t zp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF;
    uint8_t value = read_memory(state, zp_address);

    if (state->decimal_mode) {
        BCD_ADD(state, value);
    } else {
        uint16_t sum = state->accumulator + value + (state->carry_flag ? 1 : 0);
        state->carry_flag = (uint8_t)(sum > 0xFF);
        state->overflow_flag = (~(state->accumulator ^ value) & (state->accumulator ^ sum) & 0x80) != 0;
        state->accumulator = (uint8_t)(sum & 0xFF);
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ADC $%02X,X", state->program_counter - 2, zp_address);
    #endif
}

void ADC_ABSOLUTE(CPUState *state) {    // eg: ADC $0000
    uint16_t abs_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8);
    uint8_t value = read_memory(state, abs_address);

    if (state->decimal_mode) {
        BCD_ADD(state, value);
    } else {
        uint16_t sum = state->accumulator + value + (state->carry_flag ? 1 : 0);
        state->carry_flag = (uint8_t)(sum > 0xFF);
        state->overflow_flag = (~(state->accumulator ^ value) & (state->accumulator ^ sum) & 0x80) != 0;
        state->accumulator = (uint8_t)(sum & 0xFF);
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 3;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ADC $%04X", state->program_counter - 3, abs_address);
    #endif  
}

void ADC_ABSOLUTE_X(CPUState *state) {  // eg: ADC $0000,X
    uint16_t abs_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register;
    uint8_t value = read_memory(state, abs_address);

    if (state->decimal_mode) {
        BCD_ADD(state, value);
    } else {
        uint16_t sum = state->accumulator + value + (state->carry_flag ? 1 : 0);
        state->carry_flag = (uint8_t)(sum > 0xFF);
        state->overflow_flag = (~(state->accumulator ^ value) & (state->accumulator ^ sum) & 0x80) != 0;
        state->accumulator = (uint8_t)(sum & 0xFF);
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 3;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ADC $%04X,X", state->program_counter - 3, abs_address);
    #endif
}

void ADC_ABSOLUTE_Y(CPUState *state) {  // eg: ADC $0000,Y
    uint16_t abs_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register;
    uint8_t value = read_memory(state, abs_address);

    if (state->decimal_mode) {
        BCD_ADD(state, value);
    } else {
        uint16_t sum = state->accumulator + value + (state->carry_flag ? 1 : 0);
        state->carry_flag = (uint8_t)(sum > 0xFF);
        state->overflow_flag = (~(state->accumulator ^ value) & (state->accumulator ^ sum) & 0x80) != 0;
        state->accumulator = (uint8_t)(sum & 0xFF);
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 3;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ADC $%04X,Y", state->program_counter - 3, abs_address);
    #endif
}

void ADC_INDIRECT_X(CPUState *state) {  // eg: ADC ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF;
    uint16_t ind_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8;
    uint8_t value = read_memory(state, ind_address);

    if (state->decimal_mode) {
        BCD_ADD(state, value);
    } else {
        uint16_t sum = state->accumulator + value + (state->carry_flag ? 1 : 0);
        state->carry_flag = (uint8_t)(sum > 0xFF);
        state->overflow_flag = (~(state->accumulator ^ value) & (state->accumulator ^ sum) & 0x80) != 0;
        state->accumulator = (uint8_t)(sum & 0xFF);
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ADC ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void ADC_INDIRECT_Y(CPUState *state) {  // eg: ADC ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1);
    uint16_t ind_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register;
    uint8_t value = read_memory(state, ind_address);

    if (state->decimal_mode) {
        BCD_ADD(state, value);
    } else {
        uint16_t sum = state->accumulator + value + (state->carry_flag ? 1 : 0);
        state->carry_flag = (uint8_t)(sum > 0xFF);
        state->overflow_flag = (~(state->accumulator ^ value) & (state->accumulator ^ sum) & 0x80) != 0;
        state->accumulator = (uint8_t)(sum & 0xFF);
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ADC ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void AND_IMMEDIATE(CPUState *state) {   // eg: AND #$01
    temp = read_memory(state, state->program_counter + 1); // Get immediate value
    state->accumulator &= temp; // AND immediate value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND #$%02X", state->program_counter - 2, temp);
    #endif
}

void AND_ZEROPAGE(CPUState *state) {   // eg: AND $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp = read_memory(state, temp_address); // Get value from zero page
    state->accumulator &= temp; // AND value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND $%02X", state->program_counter - 2, temp_address);
    #endif
}

void AND_ZEROPAGE_X(CPUState *state) {  // eg: AND $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp = read_memory(state, temp_address); // Get value from zero page
    state->accumulator &= temp; // AND value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void AND_ABSOLUTE(CPUState *state) {    // eg: AND $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->accumulator &= temp; // AND value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND $%04X", state->program_counter - 3, temp_address);
    #endif
}

void AND_ABSOLUTE_X(CPUState *state) {  // eg: AND $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->accumulator &= temp; // AND value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void AND_ABSOLUTE_Y(CPUState *state) {  // eg: AND $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->accumulator &= temp; // AND value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void AND_INDIRECT_X(CPUState *state) {  // eg: AND ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; // Get indirect address
    temp = read_memory(state, temp_address); // Get value from indirect address
    state->accumulator &= temp; // AND value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void AND_INDIRECT_Y(CPUState *state) {  // eg: AND ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; // Get indirect address with Y offset
    temp = read_memory(state, temp_address); // Get value from indirect address
    state->accumulator &= temp; // AND value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void ASL_IMMEDIATE(CPUState *state) {  // $0A = ASL Accumulator (1-byte)
    state->carry_flag = (state->accumulator & 0x80) != 0;
    state->accumulator <<= 1;
    state->zero_flag = (state->accumulator == 0);
    state->negative_flag = (state->accumulator & 0x80) != 0;
    state->program_counter += 1;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL A", state->program_counter - 1);
    #endif
}

void ASL_ZEROPAGE(CPUState *state) {  // eg: ASL $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp = read_memory(state, temp_address); // Get value from zero page
    state->carry_flag = (temp & 0x80) != 0; // Set carry flag if bit 7 is set
    temp <<= 1; // Shift left
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL $%02X", state->program_counter - 2, temp_address);
    #endif
}

void ASL_ZEROPAGE_X(CPUState *state) {  // eg: ASL $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp = read_memory(state, temp_address); // Get value from zero page
    state->carry_flag = (temp & 0x80) != 0; // Set carry flag if bit 7 is set
    temp <<= 1; // Shift left
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void ASL_ABSOLUTE(CPUState *state) {  // eg: ASL $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->carry_flag = (temp & 0x80) != 0; // Set carry flag if bit 7 is set
    temp <<= 1; // Shift left
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL $%04X", state->program_counter - 3, temp_address);
    #endif
}

void ASL_ABSOLUTE_X(CPUState *state) {  // eg: ASL $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->carry_flag = (temp & 0x80) != 0; // Set carry flag if bit 7 is set
    temp <<= 1; // Shift left
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void ASL_ABSOLUTE_Y(CPUState *state) {  // eg: ASL $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->carry_flag = (temp & 0x80) != 0; // Set carry flag if bit 7 is set
    temp <<= 1; // Shift left
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void ASL_INDIRECT_X(CPUState *state) {  // eg: ASL ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; // Get indirect address
    temp = read_memory(state, temp_address); // Get value from indirect address
    state->carry_flag = (temp & 0x80) != 0; // Set carry flag if bit 7 is set
    temp <<= 1; // Shift left
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void ASL_INDIRECT_Y(CPUState *state) {  // eg: ASL ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; // Get indirect address with Y offset
    temp = read_memory(state, temp_address); // Get value from indirect address
    state->carry_flag = (temp & 0x80) != 0; // Set carry flag if bit 7 is set
    temp <<= 1; // Shift left
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void BCC(CPUState *state) {  // eg: BCC $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); // Get signed offset
    if (!state->carry_flag) { // If carry flag is clear
        state->program_counter += 2 + offset; // Branch to new address
    } else {
        state->program_counter += 2; // Move to the next byte in memory
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BCC $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BCS(CPUState *state) {  // eg: BCS $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); // Get signed offset
    if (state->carry_flag) { // If carry flag is set
        state->program_counter += 2 + offset; // Branch to new address
    } else {
        state->program_counter += 2; // Move to the next byte in memory
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BCS $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BEQ(CPUState *state) {  // eg: BEQ $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); // Get signed offset
    if (state->zero_flag) { // If zero flag is set
        state->program_counter += 2 + offset; // Branch to new address
    } else {
        state->program_counter += 2; // Move to the next byte in memory
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BEQ $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BIT_ZEROPAGE(CPUState *state) {  // eg: BIT $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp = read_memory(state, temp_address); // Get value from zero page
    state->zero_flag = (state->accumulator & temp) == 0; // Set or clear zero flag based on AND with accumulator
    state->negative_flag = (temp & 0x80) != 0;  // N = bit 7 of memory value
    state->overflow_flag = (temp & 0x40) != 0; // Set or clear overflow flag based on bit 6
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BIT $%02X", state->program_counter - 2, temp_address);
    #endif
}

void BIT_ABSOLUTE(CPUState *state) {  // eg: BIT $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8);
    temp = read_memory(state, temp_address);
    state->zero_flag = (state->accumulator & temp) == 0;
    state->negative_flag = (temp & 0x80) != 0;  // N = bit 7
    state->overflow_flag = (temp & 0x40) != 0;  // V = bit 6
    state->program_counter += 3;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BIT $%04X", state->program_counter - 3, temp_address);
    #endif
}

void BMI(CPUState *state) {  // eg: BMI $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); // Get signed offset
    if (state->negative_flag) { // If negative flag is set  
        state->program_counter += 2 + offset; // Branch to new address
    } else {
        state->program_counter += 2; // Move to the next byte in memory
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BMI $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BNE(CPUState *state) {  // eg: BNE $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); // Get signed offset
    if (!state->zero_flag) { // If zero flag is clear
        state->program_counter += 2 + offset; // Branch to new address
    } else {
        state->program_counter += 2; // Move to the next byte in memory
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BNE $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BPL(CPUState *state) {  // eg: BPL $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); // Get signed offset
    if (!state->negative_flag) { // If negative flag is clear
        state->program_counter += 2 + offset; // Branch to new address
    } else {
        state->program_counter += 2; // Move to the next byte in memory
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BPL $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BRK(CPUState *state) {  // eg: BRK
    state->break_command = true;
    state->program_counter += 2;  // BRK is a 2-byte instruction (opcode + padding byte)
    // Push PCH
    write_memory(state, STACK_BASE + state->stack_pointer, (state->program_counter >> 8) & 0xFF);
    state->stack_pointer--;
    // Push PCL
    write_memory(state, STACK_BASE + state->stack_pointer, state->program_counter & 0xFF);
    state->stack_pointer--;
    // Push P with B=1 and unused=1
    uint8_t brk_status = ((uint8_t)state->negative_flag     << 7)
                       | ((uint8_t)state->overflow_flag     << 6)
                       | (1                                 << 5)
                       | (1                                 << 4)  // B = 1
                       | ((uint8_t)state->decimal_mode      << 3)
                       | ((uint8_t)state->interrupt_disable << 2)
                       | ((uint8_t)state->zero_flag         << 1)
                       |  (uint8_t)state->carry_flag;
    write_memory(state, STACK_BASE + state->stack_pointer, brk_status);
    state->stack_pointer--;
    state->interrupt_disable = 1;
    state->program_counter = read_memory(state, 0x316) | (read_memory(state, 0x317) << 8);  // Set PC to IRQ vector
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BRK", state->program_counter);
    #endif
}

void BVC(CPUState *state) {  // eg: BVC $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); // Get signed offset
    if (!state->overflow_flag) { // If overflow flag is clear
        state->program_counter += 2 + offset; // Branch to new address
    } else {
        state->program_counter += 2; // Move to the next byte in memory
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BVC $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BVS(CPUState *state) {  // eg: BVS $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); // Get signed offset
    if (state->overflow_flag) { // If overflow flag is set
        state->program_counter += 2 + offset; // Branch to new address
    } else {
        state->program_counter += 2; // Move to the next byte in memory
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BVS $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void CLC(CPUState *state) {  // eg: CLC
    state->carry_flag = false; // Clear carry flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CLC", state->program_counter - 1);
    #endif
}

void CLD(CPUState *state) {  // eg: CLD
    state->decimal_mode = false; // Clear decimal mode flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CLD", state->program_counter - 1);
    #endif
}

void CLI(CPUState *state) {  // eg: CLI
    state->interrupt_disable = false; // Clear interrupt disable flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CLI", state->program_counter - 1);
    #endif
}

void CLV(CPUState *state) {  // eg: CLV
    state->overflow_flag = false; // Clear overflow flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CLV", state->program_counter - 1);
    #endif
}

void CMP_IMMEDIATE(CPUState *state) {  // eg: CMP #$01
    temp = read_memory(state, state->program_counter + 1); // Get immediate value
    uint16_t result = state->accumulator - temp; // Subtract immediate value from accumulator
    state->carry_flag = (state->accumulator >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP #$%02X", state->program_counter - 2, temp);
    #endif
}

void CMP_ZEROPAGE(CPUState *state) {  // eg: CMP $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp = read_memory(state, temp_address); // Get value from zero page
    uint16_t result = state->accumulator - temp; // Subtract value from accumulator
    state->carry_flag = (state->accumulator >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP $%02X", state->program_counter - 2, temp_address);
    #endif
}

void CMP_ZEROPAGE_X(CPUState *state) {  // eg: CMP $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp = read_memory(state, temp_address); // Get value from zero page
    uint16_t result = state->accumulator - temp; // Subtract value from accumulator
    state->carry_flag = (state->accumulator >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void CMP_ABSOLUTE(CPUState *state) {  // eg: CMP $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    temp = read_memory(state, temp_address); // Get value from absolute address
    uint16_t result = state->accumulator - temp; // Subtract value from accumulator
    state->carry_flag = (state->accumulator >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP $%04X", state->program_counter - 3, temp_address);
    #endif
}

void CMP_ABSOLUTE_X(CPUState *state) {  // eg: CMP $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    uint16_t result = state->accumulator - temp; // Subtract value from accumulator
    state->carry_flag = (state->accumulator >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void CMP_ABSOLUTE_Y(CPUState *state) {  // eg: CMP $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    uint16_t result = state->accumulator - temp; // Subtract value from accumulator
    state->carry_flag = (state->accumulator >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void CMP_INDIRECT_X(CPUState *state) {  // eg: CMP ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; // Get indirect address
    temp = read_memory(state, temp_address); // Get value from indirect address
    uint16_t result = state->accumulator - temp; // Subtract value from accumulator
    state->carry_flag = (state->accumulator >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void CMP_INDIRECT_Y(CPUState *state) {  // eg: CMP ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; // Get indirect address with Y offset
    temp = read_memory(state, temp_address); // Get value from indirect address
    uint16_t result = state->accumulator - temp; // Subtract value from accumulator
    state->carry_flag = (state->accumulator >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void CPX_IMMEDIATE(CPUState *state) {  // eg: CPX #$01
    temp = read_memory(state, state->program_counter + 1); // Get immediate value
    uint16_t result = state->x_register - temp; // Subtract immediate value from X register
    state->carry_flag = (state->x_register >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPX #$%02X", state->program_counter - 2, temp);
    #endif
}

void CPX_ZEROPAGE(CPUState *state) {  // eg: CPX $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp = read_memory(state, temp_address); // Get value from zero page
    uint16_t result = state->x_register - temp; // Subtract value from X register
    state->carry_flag = (state->x_register >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPX $%02X", state->program_counter - 2, temp_address);
    #endif
}

void CPX_ABSOLUTE(CPUState *state) {  // eg: CPX $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    temp = read_memory(state, temp_address); // Get value from absolute address
    uint16_t result = state->x_register - temp; // Subtract value from X register
    state->carry_flag = (state->x_register >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPX $%04X", state->program_counter - 3, temp_address);
    #endif
}

void CPY_IMMEDIATE(CPUState *state) {  // eg: CPY #$01
    temp = read_memory(state, state->program_counter + 1); // Get immediate value
    uint16_t result = state->y_register - temp; // Subtract immediate value from Y register
    state->carry_flag = (state->y_register >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPY #$%02X", state->program_counter - 2, temp);
    #endif
}

void CPY_ZEROPAGE(CPUState *state) {  // eg: CPY $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp = read_memory(state, temp_address); // Get value from zero page
    uint16_t result = state->y_register - temp; // Subtract value from Y register
    state->carry_flag = (state->y_register >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPY $%02X", state->program_counter - 2, temp_address);
    #endif
}

void CPY_ABSOLUTE(CPUState *state) {  // eg: CPY $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    temp = read_memory(state, temp_address); // Get value from absolute address
    uint16_t result = state->y_register - temp; // Subtract value from Y register
    state->carry_flag = (state->y_register >= temp); // Set or clear carry flag
    state->zero_flag = (result == 0); // Set or clear zero flag
    state->negative_flag = (result & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPY $%04X", state->program_counter - 3, temp_address);
    #endif
}

void DEC_ZEROPAGE(CPUState *state) {  // eg: DEC $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp = read_memory(state, temp_address); // Get value from zero page
    temp--; // Decrement value
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEC $%02X", state->program_counter - 2, temp_address);
    #endif
}

void DEC_ZEROPAGE_X(CPUState *state) {  // eg: DEC $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp = read_memory(state, temp_address); // Get value from zero page
    temp--; // Decrement value
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEC $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void DEC_ABSOLUTE(CPUState *state) {  // eg: DEC $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    temp = read_memory(state, temp_address); // Get value from absolute address
    temp--; // Decrement value
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEC $%04X", state->program_counter - 3, temp_address);
    #endif
}

void DEC_ABSOLUTE_X(CPUState *state) {  // eg: DEC $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    temp--; // Decrement value
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEC $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void DEC_ABSOLUTE_Y(CPUState *state) {  // eg: DEC $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    temp--; // Decrement value
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEC $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void DEX(CPUState *state) {  // eg: DEX
    state->x_register--; // Decrement X register
    state->zero_flag = (state->x_register == 0); // Set or clear zero flag
    state->negative_flag = (state->x_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEX", state->program_counter - 1);
    #endif
}

void DEY(CPUState *state) {  // eg: DEY
    state->y_register--; // Decrement Y register
    state->zero_flag = (state->y_register == 0); // Set or clear zero flag
    state->negative_flag = (state->y_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEY", state->program_counter - 1);
    #endif
}

void EOR_IMMEDIATE(CPUState *state) {  // eg: EOR #$01
    temp = read_memory(state, state->program_counter + 1); // Get immediate value
    state->accumulator ^= temp; // XOR value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR #$%02X", state->program_counter - 2, temp);
    #endif
}

void EOR_ZEROPAGE(CPUState *state) {  // eg: EOR $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp = read_memory(state, temp_address); // Get value from zero page
    state->accumulator ^= temp; // XOR value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR $%02X", state->program_counter - 2, temp_address);
    #endif
}

void EOR_ZEROPAGE_X(CPUState *state) {  // eg: EOR $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp = read_memory(state, temp_address); // Get value from zero page
    state->accumulator ^= temp; // XOR value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void EOR_ABSOLUTE(CPUState *state) {  // eg: EOR $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->accumulator ^= temp; // XOR value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR $%04X", state->program_counter - 3, temp_address);
    #endif
}

void EOR_ABSOLUTE_X(CPUState *state) {  // eg: EOR $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->accumulator ^= temp; // XOR value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void EOR_ABSOLUTE_Y(CPUState *state) {  // eg: EOR $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->accumulator ^= temp; // XOR value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void EOR_INDIRECT_X(CPUState *state) {  // eg: EOR ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; // Get indirect address
    temp = read_memory(state, temp_address); // Get value from indirect address
    state->accumulator ^= temp; // XOR value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void EOR_INDIRECT_Y(CPUState *state) {  // eg: EOR ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; // Get indirect address with Y offset
    temp = read_memory(state, temp_address); // Get value from indirect address
    state->accumulator ^= temp; // XOR value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void INC_ZEROPAGE(CPUState *state) {  // eg: INC $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp = read_memory(state, temp_address); // Get value from zero page
    temp++; // Increment value
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INC $%02X", state->program_counter - 2, temp_address);
    #endif
}

void INC_ZEROPAGE_X(CPUState *state) {  // eg: INC $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp = read_memory(state, temp_address); // Get value from zero page
    temp++; // Increment value
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INC $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void INC_ABSOLUTE(CPUState *state) {  // eg: INC $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    temp = read_memory(state, temp_address); // Get value from absolute address
    temp++; // Increment value
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INC $%04X", state->program_counter - 3, temp_address);
    #endif
}

void INC_ABSOLUTE_X(CPUState *state) {  // eg: INC $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    temp++; // Increment value
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INC $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void INC_ABSOLUTE_Y(CPUState *state) {  // eg: INC $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    temp++; // Increment value
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INC $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void INX(CPUState *state) {  // eg: INX
    state->x_register++; // Increment X register
    state->zero_flag = (state->x_register == 0); // Set or clear zero flag
    state->negative_flag = (state->x_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INX", state->program_counter - 1);
    #endif
}

void INY(CPUState *state) {  // eg: INY
    state->y_register++; // Increment Y register
    state->zero_flag = (state->y_register == 0); // Set or clear zero flag
    state->negative_flag = (state->y_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INY", state->program_counter - 1);
    #endif
}

void JMP_ABSOLUTE(CPUState *state) {  // eg: JMP $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    state->program_counter = temp_address; // Jump to new address
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: JMP $%04X", state->program_counter - 3, temp_address);
    #endif
}

void JMP_INDIRECT(CPUState *state) {  // eg: JMP ($0000)
    uint16_t pointer_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get pointer address
    temp_address = read_memory(state, pointer_address) | (read_memory(state, (pointer_address + 1)) & 0xFFFF) << 8; // Get indirect address
    state->program_counter = temp_address; // Jump to new address
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: JMP ($%04X)", state->program_counter - 3, pointer_address);
    #endif
}

void JSR(CPUState *state) {  // eg: JSR $0000
    uint16_t return_address = state->program_counter + 2; // Calculate return address
    write_memory(state, STACK_BASE + state->stack_pointer, (return_address >> 8) & 0xFF); // Push high byte of return address to stack
    state->stack_pointer--; // Decrement stack pointer
    write_memory(state, STACK_BASE + state->stack_pointer, return_address & 0xFF); // Push low byte of return address to stack
    state->stack_pointer--; // Decrement stack pointer
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    state->program_counter = temp_address; // Jump to new address
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: JSR $%04X", state->program_counter - 3, temp_address);
    #endif
}

void LDA_IMMEDIATE(CPUState *state) {  // eg: LDA #$01
    state->accumulator = read_memory(state, state->program_counter + 1); // Load immediate value into accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA #$%02X", state->program_counter - 2, state->accumulator);
    #endif
}

void LDA_ZEROPAGE(CPUState *state) {  // eg: LDA $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    state->accumulator = read_memory(state, temp_address); // Load value from zero page into accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA $%02X", state->program_counter - 2, temp_address);
    #endif
}

void LDA_ZEROPAGE_X(CPUState *state) {  // eg: LDA $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    state->accumulator = read_memory(state, temp_address); // Load value from zero page into accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void LDA_ABSOLUTE(CPUState *state) {  // eg: LDA $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    state->accumulator = read_memory(state, temp_address); // Load value from absolute address into accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA $%04X", state->program_counter - 3, temp_address);
    #endif
}

void LDA_ABSOLUTE_X(CPUState *state) {  // eg: LDA $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    state->accumulator = read_memory(state, temp_address); // Load value from absolute address into accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void LDA_ABSOLUTE_Y(CPUState *state) {  // eg: LDA $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    state->accumulator = read_memory(state, temp_address); // Load value from absolute address into accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void LDA_INDIRECT_X(CPUState *state) {  // eg: LDA ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; // Get indirect address
    state->accumulator = read_memory(state, temp_address); // Load value from indirect address into accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void LDA_INDIRECT_Y(CPUState *state) {  // eg: LDA ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; // Get indirect address with Y offset
    state->accumulator = read_memory(state, temp_address); // Load value from indirect address into accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void LDX_IMMEDIATE(CPUState *state) {  // eg: LDX #$01
    state->x_register = read_memory(state, state->program_counter + 1); // Load immediate value into X register
    state->zero_flag = (state->x_register == 0); // Set or clear zero flag
    state->negative_flag = (state->x_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDX #$%02X", state->program_counter - 2, state->x_register);
    #endif
}

void LDX_ZEROPAGE(CPUState *state) {  // eg: LDX $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    state->x_register = read_memory(state, temp_address); // Load value from zero page into X register
    state->zero_flag = (state->x_register == 0); // Set or clear zero flag
    state->negative_flag = (state->x_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDX $%02X", state->program_counter - 2, temp_address);
    #endif
}

void LDX_ZEROPAGE_Y(CPUState *state) {  // eg: LDX $00,Y
    temp_address = (read_memory(state, state->program_counter + 1) + state->y_register) & 0xFF; // Get zero page address with Y offset
    state->x_register = read_memory(state, temp_address); // Load value from zero page into X register
    state->zero_flag = (state->x_register == 0); // Set or clear zero flag
    state->negative_flag = (state->x_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDX $%02X,Y", state->program_counter - 2, temp_address);
    #endif
}

void LDX_ABSOLUTE(CPUState *state) {  // eg: LDX $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    state->x_register = read_memory(state, temp_address); // Load value from absolute address into X register
    state->zero_flag = (state->x_register == 0); // Set or clear zero flag
    state->negative_flag = (state->x_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDX $%04X", state->program_counter - 3, temp_address);
    #endif
}

void LDX_ABSOLUTE_Y(CPUState *state) {  // eg: LDX $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    state->x_register = read_memory(state, temp_address); // Load value from absolute address into X register
    state->zero_flag = (state->x_register == 0); // Set or clear zero flag
    state->negative_flag = (state->x_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDX $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void LDY_IMMEDIATE(CPUState *state) {  // eg: LDY #$01
    state->y_register = read_memory(state, state->program_counter + 1); // Load immediate value into Y register
    state->zero_flag = (state->y_register == 0); // Set or clear zero flag
    state->negative_flag = (state->y_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY #$%02X", state->program_counter - 2, state->y_register);
    #endif
}

void LDY_ZEROPAGE(CPUState *state) {  // eg: LDY $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    state->y_register = read_memory(state, temp_address); // Load value from zero page into Y register
    state->zero_flag = (state->y_register == 0); // Set or clear zero flag
    state->negative_flag = (state->y_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY $%02X", state->program_counter - 2, temp_address);
    #endif
}

void LDY_ZEROPAGE_X(CPUState *state) {  // eg: LDY $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    state->y_register = read_memory(state, temp_address); // Load value from zero page into Y register
    state->zero_flag = (state->y_register == 0); // Set or clear zero flag
    state->negative_flag = (state->y_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void LDY_ABSOLUTE(CPUState *state) {  // eg: LDY $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    state->y_register = read_memory(state, temp_address); // Load value from absolute address into Y register
    state->zero_flag = (state->y_register == 0); // Set or clear zero flag
    state->negative_flag = (state->y_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY $%04X", state->program_counter - 3, temp_address);
    #endif
}

void LDY_ABSOLUTE_X(CPUState *state) {  // eg: LDY $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    state->y_register = read_memory(state, temp_address); // Load value from absolute address into Y register
    state->zero_flag = (state->y_register == 0); // Set or clear zero flag
    state->negative_flag = (state->y_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void LDY_INDIRECT_X(CPUState *state) {  // eg: LDY ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; // Get indirect address
    state->y_register = read_memory(state, temp_address); // Load value from indirect address into Y register
    state->zero_flag = (state->y_register == 0); // Set or clear zero flag
    state->negative_flag = (state->y_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void LDY_INDIRECT_Y(CPUState *state) {  // eg: LDY ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; // Get indirect address with Y offset
    state->y_register = read_memory(state, temp_address); // Load value from indirect address into Y register
    state->zero_flag = (state->y_register == 0); // Set or clear zero flag
    state->negative_flag = (state->y_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void NOP(CPUState *state) {  // eg: NOP
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: NOP", state->program_counter - 1);
    #endif
}

void LSR_IMMEDIATE(CPUState *state) {  // $4A = LSR Accumulator (1-byte)
    state->carry_flag = (state->accumulator & 0x01);
    state->accumulator >>= 1;
    state->zero_flag = (state->accumulator == 0);
    state->negative_flag = 0;  // bit 7 is always 0 after LSR
    state->program_counter += 1;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR A", state->program_counter - 1);
    #endif
}

void LSR_ZEROPAGE(CPUState *state) {  // eg: LSR $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp = read_memory(state, temp_address); // Get value from zero page
    state->carry_flag = (temp & 0x01); // Set carry flag to least significant bit of value
    temp >>= 1; // Shift value right by 1
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR $%02X", state->program_counter - 2, temp_address);
    #endif
}

void LSR_ZEROPAGE_X(CPUState *state) {  // eg: LSR $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp = read_memory(state, temp_address); // Get value from zero page
    state->carry_flag = (temp & 0x01); // Set carry flag to least significant bit of value
    temp >>= 1; // Shift value right by 1
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void LSR_ABSOLUTE(CPUState *state) {  // eg: LSR $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->carry_flag = (temp & 0x01); // Set carry flag to least significant bit of value
    temp >>= 1; // Shift value right by 1
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR $%04X", state->program_counter - 3, temp_address);
    #endif
}

void LSR_ABSOLUTE_X(CPUState *state) {  // eg: LSR $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->carry_flag = (temp & 0x01); // Set carry flag to least significant bit of value
    temp >>= 1; // Shift value right by 1
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void LSR_ABSOLUTE_Y(CPUState *state) {  // eg: LSR $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    temp = read_memory(state, temp_address); // Get value from absolute address
    state->carry_flag = (temp & 0x01); // Set carry flag to least significant bit of value
    temp >>= 1; // Shift value right by 1
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void LSR_INDIRECT_X(CPUState *state) {  // eg: LSR ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; // Get indirect address
    temp = read_memory(state, temp_address); // Get value from indirect address
    state->carry_flag = (temp & 0x01); // Set carry flag to least significant bit of value
    temp >>= 1; // Shift value right by 1
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void LSR_INDIRECT_Y(CPUState *state) {  // eg: LSR ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; // Get indirect address with Y offset
    temp = read_memory(state, temp_address); // Get value from indirect address
    state->carry_flag = (temp & 0x01); // Set carry flag to least significant bit of value
    temp >>= 1; // Shift value right by 1
    write_memory(state, temp_address, temp); // Store result back in memory
    state->zero_flag = (temp == 0); // Set or clear zero flag
    state->negative_flag = (temp & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void ORA_IMMEDIATE(CPUState *state) {  // eg: ORA #$01
    state->accumulator |= read_memory(state, state->program_counter + 1); // OR immediate value with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA #$%02X", state->program_counter - 2, read_memory(state, state->program_counter - 1));
    #endif
}

void ORA_ZEROPAGE(CPUState *state) {  // eg: ORA $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    state->accumulator |= read_memory(state, temp_address); // OR value from zero page with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA $%02X", state->program_counter - 2, temp_address);
    #endif
}

void ORA_ZEROPAGE_X(CPUState *state) {  // eg: ORA $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    state->accumulator |= read_memory(state, temp_address); // OR value from zero page with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void ORA_ABSOLUTE(CPUState *state) {  // eg: ORA $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    state->accumulator |= read_memory(state, temp_address); // OR value from absolute address with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA $%04X", state->program_counter - 3, temp_address);
    #endif
}

void ORA_ABSOLUTE_X(CPUState *state) {  // eg: ORA $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    state->accumulator |= read_memory(state, temp_address); // OR value from absolute address with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void ORA_ABSOLUTE_Y(CPUState *state) {  // eg: ORA $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    state->accumulator |= read_memory(state, temp_address); // OR value from absolute address with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void ORA_INDIRECT_X(CPUState *state) {  // eg: ORA ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; // Get indirect address
    state->accumulator |= read_memory(state, temp_address); // OR value from indirect address with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void ORA_INDIRECT_Y(CPUState *state) {  // eg: ORA ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; // Get indirect address with Y offset
    state->accumulator |= read_memory(state, temp_address); // OR value from indirect address with accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void PHA(CPUState *state) {  // eg: PHA
    write_memory(state, STACK_BASE + state->stack_pointer, state->accumulator); // Push accumulator onto stack
    state->stack_pointer--; // Decrement stack pointer
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: PHA", state->program_counter - 1);
    #endif
}

void PHP(CPUState *state) {  // eg: PHP
    // PHP always pushes B=1 (bit 4) regardless of break_command state
    uint8_t status = ((uint8_t)state->negative_flag     << 7)
                   | ((uint8_t)state->overflow_flag     << 6)
                   | (1                                 << 5)
                   | (1                                 << 4)  // B always 1 for PHP
                   | ((uint8_t)state->decimal_mode      << 3)
                   | ((uint8_t)state->interrupt_disable << 2)
                   | ((uint8_t)state->zero_flag         << 1)
                   |  (uint8_t)state->carry_flag;
    write_memory(state, STACK_BASE + state->stack_pointer, status);
    state->stack_pointer--;
    state->program_counter += 1;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: PHP", state->program_counter - 1);
    #endif
}

void PLA(CPUState *state) {  // eg: PLA
    state->stack_pointer++; // Increment stack pointer
    state->accumulator = read_memory(state, STACK_BASE + state->stack_pointer); // Pull value from stack into accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: PLA", state->program_counter - 1);
    #endif
}

void PLP(CPUState *state) {  // eg: PLP
    state->stack_pointer++; // Increment stack pointer
    uint8_t status = read_memory(state, STACK_BASE + state->stack_pointer); // Pull status from stack
    state->negative_flag = (status >> 7) & 1; // Set negative flag
    state->overflow_flag = (status >> 6) & 1; // Set overflow flag
    state->break_command = (status >> 4) & 1; // Set break command flag
    state->decimal_mode = (status >> 3) & 1; // Set decimal mode flag
    state->interrupt_disable = (status >> 2) & 1; // Set interrupt disable flag
    state->zero_flag = (status >> 1) & 1; // Set zero flag
    state->carry_flag = status & 1; // Set carry flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: PLP", state->program_counter - 1);
    #endif
}

void ROL_ACCUMULATOR(CPUState *state) {  // eg: ROL
    uint8_t carry_in = state->carry_flag; // Store current carry flag
    state->carry_flag = (state->accumulator & 0x80) != 0; // Set carry flag to most significant bit of accumulator
    state->accumulator = (state->accumulator << 1) | carry_in; // Shift accumulator left and add carry in
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROL A", state->program_counter - 1);
    #endif
}

void ROL_ZEROPAGE(CPUState *state) {  // eg: ROL $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    uint8_t carry_in = state->carry_flag; // Store current carry flag
    state->carry_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set carry flag to most significant bit of value
    write_memory(state, temp_address,   (read_memory(state, temp_address) << 1) | carry_in); // Shift value left and add carry in
    state->zero_flag = (read_memory(state, temp_address) == 0); // Set or clear zero flag
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROL $%02X", state->program_counter - 2, temp_address);
    #endif
}

void ROL_ZEROPAGE_X(CPUState *state) {  // eg: ROL $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    uint8_t carry_in = state->carry_flag; // Store current carry flag
    state->carry_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set carry flag to most significant bit of value
    write_memory(state, temp_address,   (read_memory(state, temp_address) << 1) | carry_in); // Shift value left and add carry in
    state->zero_flag = (read_memory(state, temp_address) == 0); // Set or clear zero flag
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROL $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void ROL_ABSOLUTE(CPUState *state) {  // eg: ROL $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    uint8_t carry_in = state->carry_flag; // Store current carry flag
    state->carry_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set carry flag to most significant bit of value
    write_memory(state, temp_address, (read_memory(state, temp_address) << 1) | carry_in); // Shift value left and add carry in
    state->zero_flag = (read_memory(state, temp_address) == 0); // Set or clear zero flag
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROL $%04X", state->program_counter - 3, temp_address);
    #endif
}

void ROL_ABSOLUTE_X(CPUState *state) {  // eg: ROL $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    uint8_t carry_in = state->carry_flag; // Store current carry flag
    state->carry_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set carry flag to most significant bit of value
    write_memory(state, temp_address, (read_memory(state, temp_address) << 1) | carry_in); // Shift value left and add carry in
    state->zero_flag = (read_memory(state, temp_address) == 0); // Set or clear zero flag
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROL $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void ROR_ACCUMULATOR(CPUState *state) {  // eg: ROR
    uint8_t carry_in = state->carry_flag; // Store current carry flag
    state->carry_flag = (state->accumulator & 0x01) != 0; // Set carry flag to least significant bit of accumulator
    state->accumulator = (state->accumulator >> 1) | (carry_in << 7); // Shift accumulator right and add carry in
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROR A", state->program_counter - 1);
    #endif
}

void ROR_ZEROPAGE(CPUState *state) {  // eg: ROR $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    uint8_t carry_in = state->carry_flag; // Store current carry flag
    state->carry_flag = (read_memory(state, temp_address) & 0x01) != 0; // Set carry flag to least significant bit of value
    write_memory(state, temp_address, (read_memory(state, temp_address) >> 1) | (carry_in << 7)); // Shift value right and add carry in
    state->zero_flag = (read_memory(state, temp_address) == 0); // Set or clear zero flag
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROR $%02X", state->program_counter - 2, temp_address);
    #endif
}

void ROR_ZEROPAGE_X(CPUState *state) {  // eg: ROR $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    uint8_t carry_in = state->carry_flag; // Store current carry flag
    state->carry_flag = (read_memory(state, temp_address) & 0x01) != 0; // Set carry flag to least significant bit of value
    write_memory(state, temp_address, (read_memory(state, temp_address) >> 1) | (carry_in << 7)); // Shift value right and add carry in
    state->zero_flag = (read_memory(state, temp_address) == 0); // Set or clear zero flag
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROR $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void ROR_ABSOLUTE(CPUState *state) {  // eg: ROR $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    uint8_t carry_in = state->carry_flag; // Store current carry flag
    state->carry_flag = (read_memory(state, temp_address) & 0x01) != 0; // Set carry flag to least significant bit of value
    write_memory(state, temp_address, (read_memory(state, temp_address) >> 1) | (carry_in << 7)); // Shift value right and add carry in
    state->zero_flag = (read_memory(state, temp_address) == 0); // Set or clear zero flag
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROR $%04X", state->program_counter - 3, temp_address);
    #endif
}

void ROR_ABSOLUTE_X(CPUState *state) {  // eg: ROR $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    uint8_t carry_in = state->carry_flag; // Store current carry flag
    state->carry_flag = (read_memory(state, temp_address) & 0x01) != 0; // Set carry flag to least significant bit of value
    write_memory(state, temp_address, (read_memory(state, temp_address) >> 1) | (carry_in << 7)); // Shift value right and add carry in
    state->zero_flag = (read_memory(state, temp_address) == 0); // Set or clear zero flag
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROR $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void RTI(CPUState *state) {  // eg: RTI
    state->stack_pointer++; // Increment stack pointer
    uint8_t status = read_memory(state, STACK_BASE + state->stack_pointer); // Pull status from stack
    state->negative_flag = (status >> 7) & 1; // Set negative flag
    state->overflow_flag = (status >> 6) & 1; // Set overflow flag
    state->break_command = (status >> 4) & 1; // Set break command flag
    state->decimal_mode = (status >> 3) & 1; // Set decimal mode flag
    state->interrupt_disable = (status >> 2) & 1; // Set interrupt disable flag
    state->zero_flag = (status >> 1) & 1; // Set zero flag
    state->carry_flag = status & 1; // Set carry flag

    state->stack_pointer++; // Increment stack pointer again to get return address
    uint8_t low_byte = read_memory(state, STACK_BASE + state->stack_pointer); // Get low byte of return address
    state->stack_pointer++; // Increment stack pointer to get high byte of return address
    uint8_t high_byte = read_memory(state, STACK_BASE + state->stack_pointer); // Get high byte of return address

    state->program_counter = (high_byte << 8) | low_byte; // Set program counter to return address
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: RTI", state->program_counter);
    #endif
}

void RTS(CPUState *state) {  // eg: RTS
    state->stack_pointer++; // Increment stack pointer
    uint8_t low_byte = read_memory(state, STACK_BASE + state->stack_pointer); // Get low byte of return address
    state->stack_pointer++; // Increment stack pointer to get high byte of return address
    uint8_t high_byte = read_memory(state, STACK_BASE + state->stack_pointer); // Get high byte of return address
    state->program_counter = ((high_byte << 8) | low_byte) + 1; // Set program counter to return address + 1
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: RTS", state->program_counter - 1);
    #endif
}

void SBC_IMMEDIATE(CPUState *state) {  // eg: SBC #$01
    uint8_t value = read_memory(state, state->program_counter + 1);
    if (state->decimal_mode) {
        BCD_SUB(state, value);
    } else {
        int result = state->accumulator - value - (1 - state->carry_flag);
        state->carry_flag = (result >= 0);
        uint8_t final_acc = (uint8_t)(result & 0xFF);
        state->overflow_flag = ((state->accumulator ^ value) & (state->accumulator ^ final_acc) & 0x80) != 0;
        state->accumulator = final_acc;
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SBC #$%02X", state->program_counter - 2, value);
    #endif
}

void SBC_ZEROPAGE(CPUState *state) {  // eg: SBC $00
    uint8_t zp_address = read_memory(state, state->program_counter + 1);
    uint8_t value = read_memory(state, zp_address);
    if (state->decimal_mode) {
        BCD_SUB(state, value);
    } else {
        int result = state->accumulator - value - (1 - state->carry_flag);
        state->carry_flag = (result >= 0);
        uint8_t final_acc = (uint8_t)(result & 0xFF);
        state->overflow_flag = ((state->accumulator ^ value) & (state->accumulator ^ final_acc) & 0x80) != 0;
        state->accumulator = final_acc;
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SBC $%02X", state->program_counter - 2, zp_address);
    #endif
}

void SBC_ZEROPAGE_X(CPUState *state) {  // eg: SBC $00,X
    uint8_t zp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF;
    uint8_t value = read_memory(state, zp_address);
    if (state->decimal_mode) {
        BCD_SUB(state, value);
    } else {
        int result = state->accumulator - value - (1 - state->carry_flag);
        state->carry_flag = (result >= 0);
        uint8_t final_acc = (uint8_t)(result & 0xFF);
        state->overflow_flag = ((state->accumulator ^ value) & (state->accumulator ^ final_acc) & 0x80) != 0;
        state->accumulator = final_acc;
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SBC $%02X,X", state->program_counter - 2, zp_address);
    #endif
}

void SBC_ABSOLUTE(CPUState *state) {  // eg: SBC $0000
    uint16_t abs_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8);
    uint8_t value = read_memory(state, abs_address);
    if (state->decimal_mode) {
        BCD_SUB(state, value);
    } else {
        int result = state->accumulator - value - (1 - state->carry_flag);
        state->carry_flag = (result >= 0);
        uint8_t final_acc = (uint8_t)(result & 0xFF);
        state->overflow_flag = ((state->accumulator ^ value) & (state->accumulator ^ final_acc) & 0x80) != 0;
        state->accumulator = final_acc;
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 3;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SBC $%04X", state->program_counter - 3, abs_address);
    #endif
}

void SBC_ABSOLUTE_X(CPUState *state) {  // eg: SBC $0000,X
    uint16_t abs_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register;
    uint8_t value = read_memory(state, abs_address);
    if (state->decimal_mode) {
        BCD_SUB(state, value);
    } else {
        int result = state->accumulator - value - (1 - state->carry_flag);
        state->carry_flag = (result >= 0);
        uint8_t final_acc = (uint8_t)(result & 0xFF);
        state->overflow_flag = ((state->accumulator ^ value) & (state->accumulator ^ final_acc) & 0x80) != 0;
        state->accumulator = final_acc;
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 3;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SBC $%04X,X", state->program_counter - 3, abs_address);
    #endif
}

void SBC_ABSOLUTE_Y(CPUState *state) {  // eg: SBC $0000,Y
    uint16_t abs_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register;
    uint8_t value = read_memory(state, abs_address);
    if (state->decimal_mode) {
        BCD_SUB(state, value);
    } else {
        int result = state->accumulator - value - (1 - state->carry_flag);
        state->carry_flag = (result >= 0);
        uint8_t final_acc = (uint8_t)(result & 0xFF);
        state->overflow_flag = ((state->accumulator ^ value) & (state->accumulator ^ final_acc) & 0x80) != 0;
        state->accumulator = final_acc;
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 3;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SBC $%04X,Y", state->program_counter - 3, abs_address);
    #endif
}

void SBC_INDIRECT_X(CPUState *state) {  // eg: SBC ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF;
    uint16_t ind_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8;
    uint8_t value = read_memory(state, ind_address);
    if (state->decimal_mode) {
        BCD_SUB(state, value);
    } else {
        int result = state->accumulator - value - (1 - state->carry_flag);
        state->carry_flag = (result >= 0);
        uint8_t final_acc = (uint8_t)(result & 0xFF);
        state->overflow_flag = ((state->accumulator ^ value) & (state->accumulator ^ final_acc) & 0x80) != 0;
        state->accumulator = final_acc;
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SBC ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void SBC_INDIRECT_Y(CPUState *state) {  // eg: SBC ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1);
    uint16_t ind_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register;
    uint8_t value = read_memory(state, ind_address);
    if (state->decimal_mode) {
        BCD_SUB(state, value);
    } else {
        int result = state->accumulator - value - (1 - state->carry_flag);
        state->carry_flag = (result >= 0);
        uint8_t final_acc = (uint8_t)(result & 0xFF);
        state->overflow_flag = ((state->accumulator ^ value) & (state->accumulator ^ final_acc) & 0x80) != 0;
        state->accumulator = final_acc;
        state->zero_flag = (state->accumulator == 0);
        state->negative_flag = (state->accumulator & 0x80) != 0;
    }
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SBC ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}


void STA_ZEROPAGE(CPUState *state) {  // eg: STA $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    write_memory(state, temp_address, state->accumulator); // Store accumulator in zero page
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA $%02X", state->program_counter - 2, temp_address);
    #endif
}

void STA_ZEROPAGE_X(CPUState *state) {  // eg: STA $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    write_memory(state, temp_address, state->accumulator); // Store accumulator in zero page
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void STA_ABSOLUTE(CPUState *state) {  // eg: STA $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    write_memory(state, temp_address, state->accumulator); // Store accumulator in absolute address
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA $%04X", state->program_counter - 3, temp_address);
    #endif
}

void STA_ABSOLUTE_X(CPUState *state) {  // eg: STA $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; // Get absolute address with X offset
    write_memory(state, temp_address, state->accumulator); // Store accumulator in absolute address
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void STA_ABSOLUTE_Y(CPUState *state) {  // eg: STA $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; // Get absolute address with Y offset
    write_memory(state, temp_address, state->accumulator); // Store accumulator in absolute address
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void STA_INDIRECT_X(CPUState *state) {  // eg: STA ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; // Get indirect address
    write_memory(state, temp_address, state->accumulator); // Store accumulator in indirect address
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void STA_INDIRECT_Y(CPUState *state) {  // eg: STA ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); // Get zero page address
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8)     + state->y_register; // Get indirect address with Y offset
    write_memory(state, temp_address, state->accumulator); // Store accumulator in indirect address
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void STX_ZEROPAGE(CPUState *state) {  // eg: STX $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    write_memory(state, temp_address, state->x_register); // Store X register in zero page
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STX $%02X", state->program_counter - 2, temp_address);
    #endif
}

void STX_ZEROPAGE_Y(CPUState *state) {  // eg: STX $00,Y
    temp_address = (read_memory(state, state->program_counter + 1) + state->y_register) & 0xFF; // Get zero page address with Y offset
    write_memory(state, temp_address, state->x_register); // Store X register in zero page
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STX $%02X,Y", state->program_counter - 2, temp_address);
    #endif
}

void STX_ABSOLUTE(CPUState *state) {  // eg: STX $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    write_memory(state, temp_address, state->x_register); // Store X register in absolute address
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STX $%04X", state->program_counter - 3, temp_address);
    #endif
}

void STY_ZEROPAGE(CPUState *state) {  // eg: STY $00
    temp_address = read_memory(state, state->program_counter + 1); // Get zero page address
    write_memory(state, temp_address, state->y_register); // Store Y register in zero page
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STY $%02X", state->program_counter - 2, temp_address);
    #endif
}

void STY_ZEROPAGE_X(CPUState *state) {  // eg: STY $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; // Get zero page address with X offset
    write_memory(state, temp_address, state->y_register); // Store Y register in zero page
    state->program_counter += 2; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STY $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void STY_ABSOLUTE(CPUState *state) {  // eg: STY $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); // Get absolute address
    write_memory(state, temp_address, state->y_register); // Store Y register in absolute address
    state->program_counter += 3; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STY $%04X", state->program_counter - 3, temp_address);
    #endif
}

void TXS(CPUState *state) {  // eg: TXS
    state->stack_pointer = state->x_register; // Transfer X register to stack pointer
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TXS", state->program_counter - 1);
    #endif
}

void TSX(CPUState *state) {  // eg: TSX
    state->x_register = state->stack_pointer; // Transfer stack pointer to X register
    state->zero_flag = (state->x_register == 0); // Set or clear zero flag
    state->negative_flag = (state->x_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TSX", state->program_counter - 1);
    #endif
}

void TXA(CPUState *state) {  // eg: TXA
    state->accumulator = state->x_register; // Transfer X register to accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TXA", state->program_counter - 1);
    #endif
}

void TYA(CPUState *state) {  // eg: TYA
    state->accumulator = state->y_register; // Transfer Y register to accumulator
    state->zero_flag = (state->accumulator == 0); // Set or clear zero flag
    state->negative_flag = (state->accumulator & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TYA", state->program_counter - 1);
    #endif
}

void TAY(CPUState *state) {  // eg: TAY
    state->y_register = state->accumulator; // Transfer accumulator to Y register
    state->zero_flag = (state->y_register == 0); // Set or clear zero flag
    state->negative_flag = (state->y_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TAY", state->program_counter - 1);
    #endif
}

void TAX(CPUState *state) {  // eg: TAX
    state->x_register = state->accumulator; // Transfer accumulator to X register
    state->zero_flag = (state->x_register == 0); // Set or clear zero flag
    state->negative_flag = (state->x_register & 0x80) != 0; // Set or clear negative flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TAX", state->program_counter - 1);
    #endif
}

void SEC(CPUState *state) {  // eg: SEC
    state->carry_flag = 1; // Set carry flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SEC", state->program_counter - 1);
    #endif
}

void SED(CPUState *state) {  // eg: SED
    state->decimal_mode = 1; // Set decimal mode flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SED", state->program_counter - 1);
    #endif
}

void SEI(CPUState *state) {  // eg: SEI
    state->interrupt_disable = 1; // Set interrupt disable flag
    state->program_counter += 1; // Move to the next byte in memory
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SEI", state->program_counter - 1);
    #endif
}

// Fire a maskable IRQ into the CPU.
// Sets CIA1 ICR ($DC0D) = $81 so the KERNAL timer ISR sees a Timer-A event
// and runs cursor blink + keyboard scan as it would on real hardware.
void trigger_irq(CPUState *state, uint16_t irq_source) {

    state->IRQ_input = false; // Clear the IRQ input line

    // Push PCH, PCL, P (B=0 for hardware IRQ)
    uint8_t status = ((uint8_t)state->negative_flag     << 7)
                   | ((uint8_t)state->overflow_flag     << 6)
                   | (1                                 << 5)  // always 1
                   | (0                                 << 4)  // B = 0 for IRQ
                   | ((uint8_t)state->decimal_mode      << 3)
                   | ((uint8_t)state->interrupt_disable << 2)
                   | ((uint8_t)state->zero_flag         << 1)
                   |  (uint8_t)state->carry_flag;

    write_memory(state, STACK_BASE + state->stack_pointer, (state->program_counter >> 8) & 0xFF);
    state->stack_pointer--;
    write_memory(state, STACK_BASE + state->stack_pointer, state->program_counter & 0xFF);
    state->stack_pointer--;
    write_memory(state, STACK_BASE + state->stack_pointer, status);
    state->stack_pointer--;
    state->program_counter = irq_source;
}

void (*opcode_functions[256])(CPUState *state) = {
    [0x00] = BRK,
    [0x01] = ORA_INDIRECT_X,
    [0x02] = ASL_INDIRECT_X,
    [0x05] = ORA_ZEROPAGE,
    [0x06] = ASL_ZEROPAGE,
    [0x08] = PHP,
    [0x09] = ORA_IMMEDIATE,
    [0x0A] = ASL_IMMEDIATE,
    [0x0D] = ORA_ABSOLUTE,
    [0x0E] = ASL_ABSOLUTE,
    [0x10] = BPL,
    [0x11] = ORA_INDIRECT_Y,
    [0x12] = ASL_INDIRECT_Y,
    [0x15] = ORA_ZEROPAGE_X,
    [0x16] = ASL_ZEROPAGE_X,
    [0x18] = CLC,
    [0x19] = ORA_ABSOLUTE_Y,
    [0x1A] = ASL_ABSOLUTE_Y,
    [0x1D] = ORA_ABSOLUTE_X,
    [0x1E] = ASL_ABSOLUTE_X,
    [0x20] = JSR,
    [0x21] = AND_INDIRECT_X,
    [0x24] = BIT_ZEROPAGE,
    [0x2C] = BIT_ABSOLUTE,
    [0x25] = AND_ZEROPAGE,
    [0x26] = ROL_ZEROPAGE,
    [0x28] = PLP,
    [0x29] = AND_IMMEDIATE,
    [0x2A] = ROL_ACCUMULATOR,
    [0x2D] = AND_ABSOLUTE,
    [0x2E] = ROL_ABSOLUTE,
    [0x30] = BMI,
    [0x31] = AND_INDIRECT_Y,
    [0x35] = AND_ZEROPAGE_X,
    [0x36] = ROL_ZEROPAGE_X,
    [0x38] = SEC,
    [0x39] = AND_ABSOLUTE_Y,
    [0x3D] = AND_ABSOLUTE_X,
    [0x3E] = ROL_ABSOLUTE_X,
    [0x40] = RTI,
    [0x41] = EOR_INDIRECT_X,
    [0x42] = LSR_INDIRECT_X,
    [0x45] = EOR_ZEROPAGE,
    [0x46] = LSR_ZEROPAGE,
    [0x48] = PHA,
    [0x49] = EOR_IMMEDIATE,
    [0x4A] = LSR_IMMEDIATE,
    [0x4C] = JMP_ABSOLUTE,
    [0x4D] = EOR_ABSOLUTE,
    [0x4E] = LSR_ABSOLUTE,
    [0x50] = BVC,
    [0x51] = EOR_INDIRECT_Y,
    [0x52] = LSR_INDIRECT_Y,
    [0x55] = EOR_ZEROPAGE_X,
    [0x56] = LSR_ZEROPAGE_X,
    [0x58] = CLI,
    [0x59] = EOR_ABSOLUTE_Y,
    [0x5A] = LSR_ABSOLUTE_Y,
    [0x5D] = EOR_ABSOLUTE_X,
    [0x5E] = LSR_ABSOLUTE_X,
    [0x60] = RTS,
    [0x61] = ADC_INDIRECT_X,
    [0x65] = ADC_ZEROPAGE,
    [0x66] = ROR_ZEROPAGE,
    [0x68] = PLA,
    [0x69] = ADC_IMMEDIATE,
    [0x6A] = ROR_ACCUMULATOR,
    [0x6C] = JMP_INDIRECT,
    [0x6D] = ADC_ABSOLUTE,
    [0x6E] = ROR_ABSOLUTE,
    [0x70] = BVS,
    [0x71] = ADC_INDIRECT_Y,
    [0x75] = ADC_ZEROPAGE_X,
    [0x76] = ROR_ZEROPAGE_X,
    [0x78] = SEI,
    [0x79] = ADC_ABSOLUTE_Y,
    [0x7D] = ADC_ABSOLUTE_X,
    [0x7E] = ROR_ABSOLUTE_X,
    [0x81] = STA_INDIRECT_X,
    [0x84] = STY_ZEROPAGE,
    [0x85] = STA_ZEROPAGE,
    [0x86] = STX_ZEROPAGE,
    [0x88] = DEY,
    [0x8A] = TXA,
    [0x8C] = STY_ABSOLUTE,
    [0x8D] = STA_ABSOLUTE,
    [0x8E] = STX_ABSOLUTE,
    [0x90] = BCC,
    [0x91] = STA_INDIRECT_Y,
    [0x94] = STY_ZEROPAGE_X,
    [0x95] = STA_ZEROPAGE_X,
    [0x96] = STX_ZEROPAGE_Y,
    [0x98] = TYA,
    [0x99] = STA_ABSOLUTE_Y,
    [0x9A] = TXS,
    [0x9D] = STA_ABSOLUTE_X,
    [0xA0] = LDY_IMMEDIATE,
    [0xA1] = LDA_INDIRECT_X,
    [0xA2] = LDX_IMMEDIATE,
    [0xA4] = LDY_ZEROPAGE,
    [0xA5] = LDA_ZEROPAGE,
    [0xA6] = LDX_ZEROPAGE,
    [0xA8] = TAY,
    [0xA9] = LDA_IMMEDIATE,
    [0xAA] = TAX,
    [0xAC] = LDY_ABSOLUTE,
    [0xAD] = LDA_ABSOLUTE,
    [0xAE] = LDX_ABSOLUTE,
    [0xB0] = BCS,
    [0xB1] = LDA_INDIRECT_Y,
    [0xB4] = LDY_ZEROPAGE_X,
    [0xB5] = LDA_ZEROPAGE_X,
    [0xB6] = LDX_ZEROPAGE_Y,
    [0xB8] = CLV,
    [0xB9] = LDA_ABSOLUTE_Y,
    [0xBA] = TSX,
    [0xBC] = LDY_ABSOLUTE_X,
    [0xBD] = LDA_ABSOLUTE_X,
    [0xBE] = LDX_ABSOLUTE_Y,
    [0xC0] = CPY_IMMEDIATE,
    [0xC1] = CMP_INDIRECT_X,
    [0xC4] = CPY_ZEROPAGE,
    [0xC5] = CMP_ZEROPAGE,
    [0xC6] = DEC_ZEROPAGE,
    [0xC8] = INY,
    [0xC9] = CMP_IMMEDIATE,
    [0xCA] = DEX,
    [0xCC] = CPY_ABSOLUTE,
    [0xCD] = CMP_ABSOLUTE,
    [0xCE] = DEC_ABSOLUTE,
    [0xD0] = BNE,
    [0xD1] = CMP_INDIRECT_Y,
    [0xD5] = CMP_ZEROPAGE_X,
    [0xD6] = DEC_ZEROPAGE_X,
    [0xD8] = CLD,
    [0xD9] = CMP_ABSOLUTE_Y,
    [0xDD] = CMP_ABSOLUTE_X,
    [0xDE] = DEC_ABSOLUTE_X,
    [0xE0] = CPX_IMMEDIATE,
    [0xE1] = SBC_INDIRECT_X,
    [0xE4] = CPX_ZEROPAGE,
    [0xE5] = SBC_ZEROPAGE,
    [0xE6] = INC_ZEROPAGE,
    [0xE8] = INX,
    [0xE9] = SBC_IMMEDIATE,
    [0xEA] = NOP,
    [0xEC] = CPX_ABSOLUTE,
    [0xED] = SBC_ABSOLUTE,
    [0xEE] = INC_ABSOLUTE,
    [0xF0] = BEQ,
    [0xF1] = SBC_INDIRECT_Y,
    [0xF5] = SBC_ZEROPAGE_X,
    [0xF6] = INC_ZEROPAGE_X,
    [0xF8] = SED,
    [0xF9] = SBC_ABSOLUTE_Y,
    [0xFD] = SBC_ABSOLUTE_X,
    [0xFE] = INC_ABSOLUTE_X
};


void execute_opcode(CPUState *state, uint8_t opcode) {

    if (state->NMI_input) {
        // Trigger NMI if the NMI line is high
        state->NMI_input = 0; // Clear the NMI input after triggering
        #if debug
        snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: NMI", state->program_counter);
        #endif
        trigger_irq(state, state->nmi_vector);
        return;
    }

    if (state->IRQ_input) {
        state->IRQ_input = false; // Clear the IRQ input after triggering
        #if debug
        snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: IRQ", state->program_counter);
        #endif
        trigger_irq(state, state->irq_vector);
        return;
    }
    if (opcode_functions[opcode]) {
        opcode_functions[opcode](state);
    } else {
        snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ??? (Unknown opcode 0x%02X)", state->program_counter, opcode);
    }
}