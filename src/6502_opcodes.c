#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "pico/time.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "6502_opcodes.h"
#include "syscalls.h"
#include "c64-roms.h"
#include "basicext.h"
#include "memmap.h"
#include "enums.h"
#include "structs.h"
#include "syscalls.h"

uint8_t memory[MEMORY_SIZE];

#define basic_start_address     0xA000 
#define basic_end_address       0xBFFF 

#define kernal_start_address    0xE000 
#define kernal_end_address      0xFFFF 

#define charrom_start_address   0xD000 
#define cart_low_start_address  0x8000 
#define cart_high_start_address 0xE000 

uint8_t read_memory(CPUState *state, uint16_t address) {

    if (address >= BASIC_EXT_START_ADDRESS && address < BASIC_EXT_START_ADDRESS + BASIC_EXT_SIZE) {
        return BASIC_EXTENSION[address - BASIC_EXT_START_ADDRESS];
    }

    if (address >= basic_start_address && address <= basic_end_address)
    {
        return dump_c64_basic_bin[address - basic_start_address];
    }
    if (address >= kernal_start_address && address <= kernal_end_address)
    {
        return dump_c64_kernalv3_bin[address - kernal_start_address];
    }
    return memory[address];
}

void write_memory(CPUState *state, uint16_t address, uint8_t value) {
    
    memory[address] = value;

}

void reset_cpu(CPUState *state) {
    memset(memory, 0, sizeof(memory)); 
    state->accumulator = 0;
    state->x_register = 0;
    state->y_register = 0;
    state->stack_pointer = 0xFF; 
    memory[0x0001] = 0b00110111; 
    memory[0xD02F] = 0x00; 
    state->reset_vector = read_memory(state, 0xFFFC) | (read_memory(state, 0xFFFD) << 8); 
    state->irq_vector = read_memory(state, 0xFFFE) | (read_memory(state, 0xFFFF) << 8); 
    state->nmi_vector = read_memory(state, 0xFFFA) | (read_memory(state, 0xFFFB) << 8); 
    state->program_counter = state->reset_vector; 
    state->carry_flag = false; 
    state->zero_flag = false;
    state->interrupt_disable = true; 
    state->decimal_mode = false;
    state->overflow_flag = false;
    state->negative_flag = false;
    state->break_command = false;
    state->IRQ_input = false;
    state->NMI_input = false;
    state->dirty_screen = false;
    state->dirty_sprite = false;
    state->halt_flag = false;
    state->pico_irq_source = 0;
    // deinitialize GPIO pins 0-31 to SIO function
    for (int i = 0; i < 32;i++){
        gpio_set_function(i, GPIO_FUNC_SIO);
    }
    #if debug
    printf("CPU Reset: PC=%04X SP=%02X\n", state->program_counter, state->stack_pointer);
    #endif
}

void push_stack(CPUState *state, uint8_t value) {
    write_memory(state, STACK_BASE + state->stack_pointer, value);
    state->stack_pointer--;
}

__attribute__((always_inline)) inline uint8_t pop_stack(CPUState *state) {
    state->stack_pointer++;
    return read_memory(state, STACK_BASE + state->stack_pointer);
}

void push_stack_16(CPUState *state, uint16_t value) {
    push_stack(state, (value >> 8) & 0xFF); 
    push_stack(state, value & 0xFF);        
}

__attribute__((always_inline)) inline uint16_t pop_stack_16(CPUState *state) {
    uint8_t low_byte = pop_stack(state);
    uint8_t high_byte = pop_stack(state);
    return (high_byte << 8) | low_byte;
}

void SYSCALL(CPUState *state) {
    uint8_t opcode = read_memory(state, state->program_counter+1);
    if (arm_opcode_functions[opcode]) {
        arm_opcode_functions[opcode](state);
    } else {
        snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SYSCALL $%04X", state->program_counter - 3, opcode);
        state->halt_reason = SYSCALL_REASON;
        state->halt_flag = true;
    }
    state->program_counter = state->program_counter + 2;
}

void BCD_ADD(CPUState *state, uint8_t value) {
    uint16_t binary_sum = state->accumulator + value + (state->carry_flag ? 1 : 0);
    state->zero_flag = ((binary_sum & 0xFF) == 0);
    state->negative_flag = ((binary_sum & 0x80) != 0);
    state->overflow_flag = (~(state->accumulator ^ value) & (state->accumulator ^ binary_sum) & 0x80) != 0;

    int low_nibble = (state->accumulator & 0x0F) + (value & 0x0F) + (state->carry_flag ? 1 : 0);
    if (low_nibble > 9) {
        low_nibble += 6;
    }
    
    int high_nibble = (state->accumulator >> 4) + (value >> 4) + (low_nibble > 15 ? 1 : 0);
    if (high_nibble > 9) {
        high_nibble += 6;
    }
    
    state->carry_flag = (high_nibble > 15);
    state->accumulator = ((high_nibble << 4) & 0xF0) | (low_nibble & 0x0F);
}

void BCD_SUB(CPUState *state, uint8_t value) {
    uint16_t binary_diff = state->accumulator - value - (state->carry_flag ? 0 : 1);
    state->zero_flag = ((binary_diff & 0xFF) == 0);
    state->negative_flag = ((binary_diff & 0x80) != 0);
    state->overflow_flag = ((state->accumulator ^ value) & (state->accumulator ^ binary_diff) & 0x80) != 0;

    int low_nibble = (state->accumulator & 0x0F) - (value & 0x0F) - (state->carry_flag ? 0 : 1);
    if (low_nibble < 0) {
        low_nibble -= 6;
    }
    
    int high_nibble = (state->accumulator >> 4) - (value >> 4) + (low_nibble < 0 ? -1 : 0);
    if (high_nibble < 0) {
        high_nibble -= 6;
    }
    
    state->carry_flag = (binary_diff <= 0xFF); 
    state->accumulator = ((high_nibble << 4) & 0xF0) | (low_nibble & 0x0F);

}


void ADC_IMMEDIATE(CPUState *state) {  // eg: ADC #$01
    uint8_t value = read_memory(state, state->program_counter + 1);

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
    temp = read_memory(state, state->program_counter + 1);
    state->accumulator &= temp; 
    state->zero_flag = (state->accumulator == 0);
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND #$%02X", state->program_counter - 2, temp);
    #endif
}

void AND_ZEROPAGE(CPUState *state) {   // eg: AND $00
    temp_address = read_memory(state, state->program_counter + 1);
    temp = read_memory(state, temp_address);
    state->accumulator &= temp;
    state->zero_flag = (state->accumulator == 0);
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND $%02X", state->program_counter - 2, temp_address);
    #endif
}

void AND_ZEROPAGE_X(CPUState *state) {  // eg: AND $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF;
    temp = read_memory(state, temp_address);
    state->accumulator &= temp;
    state->zero_flag = (state->accumulator == 0);
    state->program_counter += 2;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void AND_ABSOLUTE(CPUState *state) {    // eg: AND $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8);
    temp = read_memory(state, temp_address);
    state->accumulator &= temp;
    state->zero_flag = (state->accumulator == 0);
    state->program_counter += 3;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND $%04X", state->program_counter - 3, temp_address);
    #endif
}

void AND_ABSOLUTE_X(CPUState *state) {  // eg: AND $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register;
    temp = read_memory(state, temp_address);
    state->accumulator &= temp;
    state->zero_flag = (state->accumulator == 0);
    state->program_counter += 3;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void AND_ABSOLUTE_Y(CPUState *state) {  // eg: AND $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    temp = read_memory(state, temp_address); 
    state->accumulator &= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void AND_INDIRECT_X(CPUState *state) {  // eg: AND ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; 
    temp = read_memory(state, temp_address); 
    state->accumulator &= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: AND ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void AND_INDIRECT_Y(CPUState *state) {  // eg: AND ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); 
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; 
    temp = read_memory(state, temp_address); 
    state->accumulator &= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->program_counter += 2; 
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
    temp_address = read_memory(state, state->program_counter + 1); 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x80) != 0; 
    temp <<= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL $%02X", state->program_counter - 2, temp_address);
    #endif
}

void ASL_ZEROPAGE_X(CPUState *state) {  // eg: ASL $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x80) != 0; 
    temp <<= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void ASL_ABSOLUTE(CPUState *state) {  // eg: ASL $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x80) != 0; 
    temp <<= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL $%04X", state->program_counter - 3, temp_address);
    #endif
}

void ASL_ABSOLUTE_X(CPUState *state) {  // eg: ASL $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x80) != 0; 
    temp <<= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void ASL_ABSOLUTE_Y(CPUState *state) {  // eg: ASL $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x80) != 0; 
    temp <<= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void ASL_INDIRECT_X(CPUState *state) {  // eg: ASL ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x80) != 0; 
    temp <<= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void ASL_INDIRECT_Y(CPUState *state) {  // eg: ASL ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); 
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x80) != 0; 
    temp <<= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ASL ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void BCC(CPUState *state) {  // eg: BCC $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); 
    if (!state->carry_flag) { 
        state->program_counter += 2 + offset; 
    } else {
        state->program_counter += 2; 
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BCC $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BCS(CPUState *state) {  // eg: BCS $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); 
    if (state->carry_flag) { 
        state->program_counter += 2 + offset; 
    } else {
        state->program_counter += 2; 
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BCS $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BEQ(CPUState *state) {  // eg: BEQ $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); 
    if (state->zero_flag) { 
        state->program_counter += 2 + offset; 
    } else {
        state->program_counter += 2; 
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BEQ $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BIT_ZEROPAGE(CPUState *state) {  // eg: BIT $00
    temp_address = read_memory(state, state->program_counter + 1); 
    temp = read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator & temp) == 0; 
    state->negative_flag = (temp & 0x80) != 0;  
    state->overflow_flag = (temp & 0x40) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BIT $%02X", state->program_counter - 2, temp_address);
    #endif
}

void BIT_ABSOLUTE(CPUState *state) {  // eg: BIT $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8);
    temp = read_memory(state, temp_address);
    state->zero_flag = (state->accumulator & temp) == 0;
    state->negative_flag = (temp & 0x80) != 0;  
    state->overflow_flag = (temp & 0x40) != 0;  
    state->program_counter += 3;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BIT $%04X", state->program_counter - 3, temp_address);
    #endif
}

void BMI(CPUState *state) {  // eg: BMI $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); 
    if (state->negative_flag) { 
        state->program_counter += 2 + offset; 
    } else {
        state->program_counter += 2; 
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BMI $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BNE(CPUState *state) {  // eg: BNE $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); 
    if (!state->zero_flag) { 
        state->program_counter += 2 + offset; 
    } else {
        state->program_counter += 2; 
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BNE $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BPL(CPUState *state) {  // eg: BPL $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); 
    if (!state->negative_flag) { 
        state->program_counter += 2 + offset; 
    } else {
        state->program_counter += 2; 
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BPL $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BRK(CPUState *state) {  // eg: BRK
    state->break_command = true;
    state->program_counter += 2;  
    push_stack(state, (state->program_counter >> 8) & 0xFF);
    push_stack(state, state->program_counter & 0xFF);
    uint8_t brk_status = ((uint8_t)state->negative_flag     << 7)
                       | ((uint8_t)state->overflow_flag     << 6)
                       | (1                                 << 5)
                       | (1                                 << 4)  
                       | ((uint8_t)state->decimal_mode      << 3)
                       | ((uint8_t)state->interrupt_disable << 2)
                       | ((uint8_t)state->zero_flag         << 1)
                       |  (uint8_t)state->carry_flag;
    push_stack(state, brk_status);
    state->interrupt_disable = 1;
    state->program_counter = read_memory(state, 0x316) | (read_memory(state, 0x317) << 8);  
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BRK", state->program_counter);
    #endif
}

void BVC(CPUState *state) {  // eg: BVC $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); 
    if (!state->overflow_flag) { 
        state->program_counter += 2 + offset; 
    } else {
        state->program_counter += 2; 
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BVC $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void BVS(CPUState *state) {  // eg: BVS $00
    int8_t offset;
    offset = (int8_t)read_memory(state, state->program_counter + 1); 
    if (state->overflow_flag) { 
        state->program_counter += 2 + offset; 
    } else {
        state->program_counter += 2; 
    }
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: BVS $%02X", state->program_counter - 2, (uint8_t)offset);
    #endif
}

void CLC(CPUState *state) {  // eg: CLC
    state->carry_flag = false; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CLC", state->program_counter - 1);
    #endif
}

void CLD(CPUState *state) {  // eg: CLD
    state->decimal_mode = false; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CLD", state->program_counter - 1);
    #endif
}

void CLI(CPUState *state) {  // eg: CLI
    state->interrupt_disable = false; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CLI", state->program_counter - 1);
    #endif
}

void CLV(CPUState *state) {  // eg: CLV
    state->overflow_flag = false; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CLV", state->program_counter - 1);
    #endif
}

void CMP_IMMEDIATE(CPUState *state) {  // eg: CMP #$01
    temp = read_memory(state, state->program_counter + 1); 
    uint16_t result = state->accumulator - temp; 
    state->carry_flag = (state->accumulator >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP #$%02X", state->program_counter - 2, temp);
    #endif
}

void CMP_ZEROPAGE(CPUState *state) {  // eg: CMP $00
    temp_address = read_memory(state, state->program_counter + 1); 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->accumulator - temp; 
    state->carry_flag = (state->accumulator >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP $%02X", state->program_counter - 2, temp_address);
    #endif
}

void CMP_ZEROPAGE_X(CPUState *state) {  // eg: CMP $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->accumulator - temp; 
    state->carry_flag = (state->accumulator >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void CMP_ABSOLUTE(CPUState *state) {  // eg: CMP $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->accumulator - temp; 
    state->carry_flag = (state->accumulator >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP $%04X", state->program_counter - 3, temp_address);
    #endif
}

void CMP_ABSOLUTE_X(CPUState *state) {  // eg: CMP $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->accumulator - temp; 
    state->carry_flag = (state->accumulator >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void CMP_ABSOLUTE_Y(CPUState *state) {  // eg: CMP $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->accumulator - temp; 
    state->carry_flag = (state->accumulator >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void CMP_INDIRECT_X(CPUState *state) {  // eg: CMP ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->accumulator - temp; 
    state->carry_flag = (state->accumulator >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void CMP_INDIRECT_Y(CPUState *state) {  // eg: CMP ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); 
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->accumulator - temp; 
    state->carry_flag = (state->accumulator >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CMP ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void CPX_IMMEDIATE(CPUState *state) {  // eg: CPX #$01
    temp = read_memory(state, state->program_counter + 1); 
    uint16_t result = state->x_register - temp; 
    state->carry_flag = (state->x_register >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPX #$%02X", state->program_counter - 2, temp);
    #endif
}

void CPX_ZEROPAGE(CPUState *state) {  // eg: CPX $00
    temp_address = read_memory(state, state->program_counter + 1); 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->x_register - temp; 
    state->carry_flag = (state->x_register >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPX $%02X", state->program_counter - 2, temp_address);
    #endif
}

void CPX_ABSOLUTE(CPUState *state) {  // eg: CPX $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->x_register - temp; 
    state->carry_flag = (state->x_register >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPX $%04X", state->program_counter - 3, temp_address);
    #endif
}

void CPY_IMMEDIATE(CPUState *state) {  // eg: CPY #$01
    temp = read_memory(state, state->program_counter + 1); 
    uint16_t result = state->y_register - temp; 
    state->carry_flag = (state->y_register >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPY #$%02X", state->program_counter - 2, temp);
    #endif
}

void CPY_ZEROPAGE(CPUState *state) {  // eg: CPY $00
    temp_address = read_memory(state, state->program_counter + 1); 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->y_register - temp; 
    state->carry_flag = (state->y_register >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPY $%02X", state->program_counter - 2, temp_address);
    #endif
}

void CPY_ABSOLUTE(CPUState *state) {  // eg: CPY $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    temp = read_memory(state, temp_address); 
    uint16_t result = state->y_register - temp; 
    state->carry_flag = (state->y_register >= temp); 
    state->zero_flag = (result == 0); 
    state->negative_flag = (result & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: CPY $%04X", state->program_counter - 3, temp_address);
    #endif
}

void DEC_ZEROPAGE(CPUState *state) {  // eg: DEC $00
    temp_address = read_memory(state, state->program_counter + 1); 
    temp = read_memory(state, temp_address); 
    temp--; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEC $%02X", state->program_counter - 2, temp_address);
    #endif
}

void DEC_ZEROPAGE_X(CPUState *state) {  // eg: DEC $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp = read_memory(state, temp_address); 
    temp--; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEC $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void DEC_ABSOLUTE(CPUState *state) {  // eg: DEC $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    temp = read_memory(state, temp_address); 
    temp--; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEC $%04X", state->program_counter - 3, temp_address);
    #endif
}

void DEC_ABSOLUTE_X(CPUState *state) {  // eg: DEC $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    temp = read_memory(state, temp_address); 
    temp--; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEC $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void DEC_ABSOLUTE_Y(CPUState *state) {  // eg: DEC $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    temp = read_memory(state, temp_address); 
    temp--; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEC $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void DEX(CPUState *state) {  // eg: DEX
    state->x_register--; 
    state->zero_flag = (state->x_register == 0); 
    state->negative_flag = (state->x_register & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEX", state->program_counter - 1);
    #endif
}

void DEY(CPUState *state) {  // eg: DEY
    state->y_register--; 
    state->zero_flag = (state->y_register == 0); 
    state->negative_flag = (state->y_register & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: DEY", state->program_counter - 1);
    #endif
}

void EOR_IMMEDIATE(CPUState *state) {  // eg: EOR #$01
    temp = read_memory(state, state->program_counter + 1); 
    state->accumulator ^= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR #$%02X", state->program_counter - 2, temp);
    #endif
}

void EOR_ZEROPAGE(CPUState *state) {  // eg: EOR $00
    temp_address = read_memory(state, state->program_counter + 1); 
    temp = read_memory(state, temp_address); 
    state->accumulator ^= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR $%02X", state->program_counter - 2, temp_address);
    #endif
}

void EOR_ZEROPAGE_X(CPUState *state) {  // eg: EOR $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp = read_memory(state, temp_address); 
    state->accumulator ^= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void EOR_ABSOLUTE(CPUState *state) {  // eg: EOR $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    temp = read_memory(state, temp_address); 
    state->accumulator ^= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR $%04X", state->program_counter - 3, temp_address);
    #endif
}

void EOR_ABSOLUTE_X(CPUState *state) {  // eg: EOR $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    temp = read_memory(state, temp_address); 
    state->accumulator ^= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void EOR_ABSOLUTE_Y(CPUState *state) {  // eg: EOR $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    temp = read_memory(state, temp_address); 
    state->accumulator ^= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void EOR_INDIRECT_X(CPUState *state) {  // eg: EOR ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; 
    temp = read_memory(state, temp_address); 
    state->accumulator ^= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void EOR_INDIRECT_Y(CPUState *state) {  // eg: EOR ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); 
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; 
    temp = read_memory(state, temp_address); 
    state->accumulator ^= temp; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: EOR ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void INC_ZEROPAGE(CPUState *state) {  // eg: INC $00
    temp_address = read_memory(state, state->program_counter + 1); 
    temp = read_memory(state, temp_address); 
    temp++; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INC $%02X", state->program_counter - 2, temp_address);
    #endif
}

void INC_ZEROPAGE_X(CPUState *state) {  // eg: INC $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp = read_memory(state, temp_address); 
    temp++; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INC $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void INC_ABSOLUTE(CPUState *state) {  // eg: INC $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    temp = read_memory(state, temp_address); 
    temp++; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INC $%04X", state->program_counter - 3, temp_address);
    #endif
}

void INC_ABSOLUTE_X(CPUState *state) {  // eg: INC $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    temp = read_memory(state, temp_address); 
    temp++; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INC $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void INC_ABSOLUTE_Y(CPUState *state) {  // eg: INC $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    temp = read_memory(state, temp_address); 
    temp++; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INC $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void INX(CPUState *state) {  // eg: INX
    state->x_register++; 
    state->zero_flag = (state->x_register == 0); 
    state->negative_flag = (state->x_register & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INX", state->program_counter - 1);
    #endif
}

void INY(CPUState *state) {  // eg: INY
    state->y_register++; 
    state->zero_flag = (state->y_register == 0); 
    state->negative_flag = (state->y_register & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: INY", state->program_counter - 1);
    #endif
}

void JMP_ABSOLUTE(CPUState *state) {  // eg: JMP $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    state->program_counter = temp_address; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: JMP $%04X", state->program_counter - 3, temp_address);
    #endif
}

void JMP_INDIRECT(CPUState *state) {  // eg: JMP ($0000)
    uint16_t pointer_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    temp_address = read_memory(state, pointer_address) | (read_memory(state, (pointer_address + 1)) & 0xFFFF) << 8; 
    state->program_counter = temp_address; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: JMP ($%04X)", state->program_counter - 3, pointer_address);
    #endif
}

void JSR(CPUState *state) {  // eg: JSR $0000
    uint16_t return_address = state->program_counter + 2; 
    push_stack(state, (return_address >> 8) & 0xFF); 
    push_stack(state, return_address & 0xFF);
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    state->program_counter = temp_address; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: JSR $%04X", state->program_counter - 3, temp_address);
    #endif
}

void LDA_IMMEDIATE(CPUState *state) {  // eg: LDA #$01
    state->accumulator = read_memory(state, state->program_counter + 1); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA #$%02X", state->program_counter - 2, state->accumulator);
    #endif
}

void LDA_ZEROPAGE(CPUState *state) {  // eg: LDA $00
    temp_address = read_memory(state, state->program_counter + 1); 
    state->accumulator = read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA $%02X", state->program_counter - 2, temp_address);
    #endif
}

void LDA_ZEROPAGE_X(CPUState *state) {  // eg: LDA $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    state->accumulator = read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void LDA_ABSOLUTE(CPUState *state) {  // eg: LDA $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    state->accumulator = read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA $%04X", state->program_counter - 3, temp_address);
    #endif
}

void LDA_ABSOLUTE_X(CPUState *state) {  // eg: LDA $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    state->accumulator = read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void LDA_ABSOLUTE_Y(CPUState *state) {  // eg: LDA $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    state->accumulator = read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void LDA_INDIRECT_X(CPUState *state) {  // eg: LDA ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; 
    state->accumulator = read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void LDA_INDIRECT_Y(CPUState *state) {  // eg: LDA ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); 
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; 
    state->accumulator = read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDA ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void LDX_IMMEDIATE(CPUState *state) {  // eg: LDX #$01
    state->x_register = read_memory(state, state->program_counter + 1); 
    state->zero_flag = (state->x_register == 0); 
    state->negative_flag = (state->x_register & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDX #$%02X", state->program_counter - 2, state->x_register);
    #endif
}

void LDX_ZEROPAGE(CPUState *state) {  // eg: LDX $00
    temp_address = read_memory(state, state->program_counter + 1); 
    state->x_register = read_memory(state, temp_address); 
    state->zero_flag = (state->x_register == 0); 
    state->negative_flag = (state->x_register & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDX $%02X", state->program_counter - 2, temp_address);
    #endif
}

void LDX_ZEROPAGE_Y(CPUState *state) {  // eg: LDX $00,Y
    temp_address = (read_memory(state, state->program_counter + 1) + state->y_register) & 0xFF; 
    state->x_register = read_memory(state, temp_address); 
    state->zero_flag = (state->x_register == 0); 
    state->negative_flag = (state->x_register & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDX $%02X,Y", state->program_counter - 2, temp_address);
    #endif
}

void LDX_ABSOLUTE(CPUState *state) {  // eg: LDX $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    state->x_register = read_memory(state, temp_address); 
    state->zero_flag = (state->x_register == 0); 
    state->negative_flag = (state->x_register & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDX $%04X", state->program_counter - 3, temp_address);
    #endif
}

void LDX_ABSOLUTE_Y(CPUState *state) {  // eg: LDX $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    state->x_register = read_memory(state, temp_address); 
    state->zero_flag = (state->x_register == 0); 
    state->negative_flag = (state->x_register & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDX $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void LDY_IMMEDIATE(CPUState *state) {  // eg: LDY #$01
    state->y_register = read_memory(state, state->program_counter + 1); 
    state->zero_flag = (state->y_register == 0); 
    state->negative_flag = (state->y_register & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY #$%02X", state->program_counter - 2, state->y_register);
    #endif
}

void LDY_ZEROPAGE(CPUState *state) {  // eg: LDY $00
    temp_address = read_memory(state, state->program_counter + 1); 
    state->y_register = read_memory(state, temp_address); 
    state->zero_flag = (state->y_register == 0); 
    state->negative_flag = (state->y_register & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY $%02X", state->program_counter - 2, temp_address);
    #endif
}

void LDY_ZEROPAGE_X(CPUState *state) {  // eg: LDY $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    state->y_register = read_memory(state, temp_address); 
    state->zero_flag = (state->y_register == 0); 
    state->negative_flag = (state->y_register & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void LDY_ABSOLUTE(CPUState *state) {  // eg: LDY $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    state->y_register = read_memory(state, temp_address); 
    state->zero_flag = (state->y_register == 0); 
    state->negative_flag = (state->y_register & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY $%04X", state->program_counter - 3, temp_address);
    #endif
}

void LDY_ABSOLUTE_X(CPUState *state) {  // eg: LDY $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    state->y_register = read_memory(state, temp_address); 
    state->zero_flag = (state->y_register == 0); 
    state->negative_flag = (state->y_register & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void LDY_INDIRECT_X(CPUState *state) {  // eg: LDY ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; 
    state->y_register = read_memory(state, temp_address); 
    state->zero_flag = (state->y_register == 0); 
    state->negative_flag = (state->y_register & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void LDY_INDIRECT_Y(CPUState *state) {  // eg: LDY ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); 
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; 
    state->y_register = read_memory(state, temp_address); 
    state->zero_flag = (state->y_register == 0); 
    state->negative_flag = (state->y_register & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LDY ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void NOP(CPUState *state) {  // eg: NOP
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: NOP", state->program_counter - 1);
    #endif
}

void LSR_IMMEDIATE(CPUState *state) {  // $4A = LSR Accumulator (1-byte)
    state->carry_flag = (state->accumulator & 0x01);
    state->accumulator >>= 1;
    state->zero_flag = (state->accumulator == 0);
    state->negative_flag = 0;  
    state->program_counter += 1;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR A", state->program_counter - 1);
    #endif
}

void LSR_ZEROPAGE(CPUState *state) {  // eg: LSR $00
    temp_address = read_memory(state, state->program_counter + 1); 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x01); 
    temp >>= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR $%02X", state->program_counter - 2, temp_address);
    #endif
}

void LSR_ZEROPAGE_X(CPUState *state) {  // eg: LSR $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x01); 
    temp >>= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void LSR_ABSOLUTE(CPUState *state) {  // eg: LSR $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x01); 
    temp >>= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR $%04X", state->program_counter - 3, temp_address);
    #endif
}

void LSR_ABSOLUTE_X(CPUState *state) {  // eg: LSR $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x01); 
    temp >>= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void LSR_ABSOLUTE_Y(CPUState *state) {  // eg: LSR $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x01); 
    temp >>= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void LSR_INDIRECT_X(CPUState *state) {  // eg: LSR ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x01); 
    temp >>= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void LSR_INDIRECT_Y(CPUState *state) {  // eg: LSR ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); 
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; 
    temp = read_memory(state, temp_address); 
    state->carry_flag = (temp & 0x01); 
    temp >>= 1; 
    write_memory(state, temp_address, temp); 
    state->zero_flag = (temp == 0); 
    state->negative_flag = (temp & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: LSR ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void ORA_IMMEDIATE(CPUState *state) {  // eg: ORA #$01
    state->accumulator |= read_memory(state, state->program_counter + 1); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA #$%02X", state->program_counter - 2, read_memory(state, state->program_counter - 1));
    #endif
}

void ORA_ZEROPAGE(CPUState *state) {  // eg: ORA $00
    temp_address = read_memory(state, state->program_counter + 1); 
    state->accumulator |= read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA $%02X", state->program_counter - 2, temp_address);
    #endif
}

void ORA_ZEROPAGE_X(CPUState *state) {  // eg: ORA $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    state->accumulator |= read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void ORA_ABSOLUTE(CPUState *state) {  // eg: ORA $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    state->accumulator |= read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA $%04X", state->program_counter - 3, temp_address);
    #endif
}

void ORA_ABSOLUTE_X(CPUState *state) {  // eg: ORA $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    state->accumulator |= read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void ORA_ABSOLUTE_Y(CPUState *state) {  // eg: ORA $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    state->accumulator |= read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void ORA_INDIRECT_X(CPUState *state) {  // eg: ORA ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; 
    state->accumulator |= read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void ORA_INDIRECT_Y(CPUState *state) {  // eg: ORA ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); 
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8) + state->y_register; 
    state->accumulator |= read_memory(state, temp_address); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ORA ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void PHA(CPUState *state) {  // eg: PHA
    push_stack(state, state->accumulator);
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: PHA", state->program_counter - 1);
    #endif
}

void PHP(CPUState *state) {  // eg: PHP
    uint8_t status = ((uint8_t)state->negative_flag     << 7)
                   | ((uint8_t)state->overflow_flag     << 6)
                   | (1                                 << 5)
                   | (1                                 << 4)  
                   | ((uint8_t)state->decimal_mode      << 3)
                   | ((uint8_t)state->interrupt_disable << 2)
                   | ((uint8_t)state->zero_flag         << 1)
                   |  (uint8_t)state->carry_flag;
    push_stack(state, status);
    state->program_counter += 1;
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: PHP", state->program_counter - 1);
    #endif
}

void PLA(CPUState *state) {  // eg: PLA
    state->accumulator = pop_stack(state);
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: PLA", state->program_counter - 1);
    #endif
}

void PLP(CPUState *state) {  // eg: PLP
    uint8_t status = pop_stack(state);
    state->negative_flag = (status >> 7) & 1; 
    state->overflow_flag = (status >> 6) & 1; 
    state->break_command = (status >> 4) & 1; 
    state->decimal_mode = (status >> 3) & 1; 
    state->interrupt_disable = (status >> 2) & 1; 
    state->zero_flag = (status >> 1) & 1; 
    state->carry_flag = status & 1; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: PLP", state->program_counter - 1);
    #endif
}

void ROL_ACCUMULATOR(CPUState *state) {  // eg: ROL
    uint8_t carry_in = state->carry_flag; 
    state->carry_flag = (state->accumulator & 0x80) != 0; 
    state->accumulator = (state->accumulator << 1) | carry_in; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROL A", state->program_counter - 1);
    #endif
}

void ROL_ZEROPAGE(CPUState *state) {  // eg: ROL $00
    temp_address = read_memory(state, state->program_counter + 1); 
    uint8_t carry_in = state->carry_flag; 
    state->carry_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    write_memory(state, temp_address,   (read_memory(state, temp_address) << 1) | carry_in); 
    state->zero_flag = (read_memory(state, temp_address) == 0); 
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROL $%02X", state->program_counter - 2, temp_address);
    #endif
}

void ROL_ZEROPAGE_X(CPUState *state) {  // eg: ROL $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    uint8_t carry_in = state->carry_flag; 
    state->carry_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    write_memory(state, temp_address,   (read_memory(state, temp_address) << 1) | carry_in); 
    state->zero_flag = (read_memory(state, temp_address) == 0); 
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROL $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void ROL_ABSOLUTE(CPUState *state) {  // eg: ROL $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    uint8_t carry_in = state->carry_flag; 
    state->carry_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    write_memory(state, temp_address, (read_memory(state, temp_address) << 1) | carry_in); 
    state->zero_flag = (read_memory(state, temp_address) == 0); 
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROL $%04X", state->program_counter - 3, temp_address);
    #endif
}

void ROL_ABSOLUTE_X(CPUState *state) {  // eg: ROL $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    uint8_t carry_in = state->carry_flag; 
    state->carry_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    write_memory(state, temp_address, (read_memory(state, temp_address) << 1) | carry_in); 
    state->zero_flag = (read_memory(state, temp_address) == 0); 
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROL $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void ROR_ACCUMULATOR(CPUState *state) {  // eg: ROR
    uint8_t carry_in = state->carry_flag; 
    state->carry_flag = (state->accumulator & 0x01) != 0; 
    state->accumulator = (state->accumulator >> 1) | (carry_in << 7); 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROR A", state->program_counter - 1);
    #endif
}

void ROR_ZEROPAGE(CPUState *state) {  // eg: ROR $00
    temp_address = read_memory(state, state->program_counter + 1); 
    uint8_t carry_in = state->carry_flag; 
    state->carry_flag = (read_memory(state, temp_address) & 0x01) != 0; 
    write_memory(state, temp_address, (read_memory(state, temp_address) >> 1) | (carry_in << 7)); 
    state->zero_flag = (read_memory(state, temp_address) == 0); 
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROR $%02X", state->program_counter - 2, temp_address);
    #endif
}

void ROR_ZEROPAGE_X(CPUState *state) {  // eg: ROR $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    uint8_t carry_in = state->carry_flag; 
    state->carry_flag = (read_memory(state, temp_address) & 0x01) != 0; 
    write_memory(state, temp_address, (read_memory(state, temp_address) >> 1) | (carry_in << 7)); 
    state->zero_flag = (read_memory(state, temp_address) == 0); 
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROR $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void ROR_ABSOLUTE(CPUState *state) {  // eg: ROR $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    uint8_t carry_in = state->carry_flag; 
    state->carry_flag = (read_memory(state, temp_address) & 0x01) != 0; 
    write_memory(state, temp_address, (read_memory(state, temp_address) >> 1) | (carry_in << 7)); 
    state->zero_flag = (read_memory(state, temp_address) == 0); 
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROR $%04X", state->program_counter - 3, temp_address);
    #endif
}

void ROR_ABSOLUTE_X(CPUState *state) {  // eg: ROR $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    uint8_t carry_in = state->carry_flag; 
    state->carry_flag = (read_memory(state, temp_address) & 0x01) != 0; 
    write_memory(state, temp_address, (read_memory(state, temp_address) >> 1) | (carry_in << 7)); 
    state->zero_flag = (read_memory(state, temp_address) == 0); 
    state->negative_flag = (read_memory(state, temp_address) & 0x80) != 0; 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ROR $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void RTI(CPUState *state) {  // eg: RTI
    uint8_t status = pop_stack(state);
    state->negative_flag = (status >> 7) & 1; 
    state->overflow_flag = (status >> 6) & 1; 
    state->break_command = (status >> 4) & 1; 
    state->decimal_mode = (status >> 3) & 1; 
    state->interrupt_disable = (status >> 2) & 1; 
    state->zero_flag = (status >> 1) & 1; 
    state->carry_flag = status & 1; 
    uint8_t low_byte = pop_stack(state); 
    uint8_t high_byte = pop_stack(state);
    state->program_counter = (high_byte << 8) | low_byte; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: RTI", state->program_counter);
    #endif
}

void RTS(CPUState *state) {  // eg: RTS
    state->program_counter = pop_stack_16(state) + 1;
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
    temp_address = read_memory(state, state->program_counter + 1); 
    write_memory(state, temp_address, state->accumulator); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA $%02X", state->program_counter - 2, temp_address);
    #endif
}

void STA_ZEROPAGE_X(CPUState *state) {  // eg: STA $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    write_memory(state, temp_address, state->accumulator); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void STA_ABSOLUTE(CPUState *state) {  // eg: STA $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    write_memory(state, temp_address, state->accumulator); 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA $%04X", state->program_counter - 3, temp_address);
    #endif
}

void STA_ABSOLUTE_X(CPUState *state) {  // eg: STA $0000,X
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->x_register; 
    write_memory(state, temp_address, state->accumulator); 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA $%04X,X", state->program_counter - 3, temp_address);
    #endif
}

void STA_ABSOLUTE_Y(CPUState *state) {  // eg: STA $0000,Y
    temp_address = (read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8)) + state->y_register; 
    write_memory(state, temp_address, state->accumulator); 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA $%04X,Y", state->program_counter - 3, temp_address);
    #endif
}

void STA_INDIRECT_X(CPUState *state) {  // eg: STA ($00,X)
    uint8_t ZEROPAGE_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    temp_address = read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8; 
    write_memory(state, temp_address, state->accumulator); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA ($%02X,X)", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void STA_INDIRECT_Y(CPUState *state) {  // eg: STA ($00),Y
    uint8_t ZEROPAGE_address = read_memory(state, state->program_counter + 1); 
    temp_address = (read_memory(state, ZEROPAGE_address) | (read_memory(state, (ZEROPAGE_address + 1)) & 0xFF) << 8)     + state->y_register; 
    write_memory(state, temp_address, state->accumulator); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STA ($%02X),Y", state->program_counter - 2, ZEROPAGE_address);
    #endif
}

void STX_ZEROPAGE(CPUState *state) {  // eg: STX $00
    temp_address = read_memory(state, state->program_counter + 1); 
    write_memory(state, temp_address, state->x_register); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STX $%02X", state->program_counter - 2, temp_address);
    #endif
}

void STX_ZEROPAGE_Y(CPUState *state) {  // eg: STX $00,Y
    temp_address = (read_memory(state, state->program_counter + 1) + state->y_register) & 0xFF; 
    write_memory(state, temp_address, state->x_register); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STX $%02X,Y", state->program_counter - 2, temp_address);
    #endif
}

void STX_ABSOLUTE(CPUState *state) {  // eg: STX $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    write_memory(state, temp_address, state->x_register); 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STX $%04X", state->program_counter - 3, temp_address);
    #endif
}

void STY_ZEROPAGE(CPUState *state) {  // eg: STY $00
    temp_address = read_memory(state, state->program_counter + 1); 
    write_memory(state, temp_address, state->y_register); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STY $%02X", state->program_counter - 2, temp_address);
    #endif
}

void STY_ZEROPAGE_X(CPUState *state) {  // eg: STY $00,X
    temp_address = (read_memory(state, state->program_counter + 1) + state->x_register) & 0xFF; 
    write_memory(state, temp_address, state->y_register); 
    state->program_counter += 2; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STY $%02X,X", state->program_counter - 2, temp_address);
    #endif
}

void STY_ABSOLUTE(CPUState *state) {  // eg: STY $0000
    temp_address = read_memory(state, state->program_counter + 1) | (read_memory(state, state->program_counter + 2) << 8); 
    write_memory(state, temp_address, state->y_register); 
    state->program_counter += 3; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: STY $%04X", state->program_counter - 3, temp_address);
    #endif
}

void TXS(CPUState *state) {  // eg: TXS
    state->stack_pointer = state->x_register; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TXS", state->program_counter - 1);
    #endif
}

void TSX(CPUState *state) {  // eg: TSX
    state->x_register = state->stack_pointer; 
    state->zero_flag = (state->x_register == 0); 
    state->negative_flag = (state->x_register & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TSX", state->program_counter - 1);
    #endif
}

void TXA(CPUState *state) {  // eg: TXA
    state->accumulator = state->x_register; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TXA", state->program_counter - 1);
    #endif
}

void TYA(CPUState *state) {  // eg: TYA
    state->accumulator = state->y_register; 
    state->zero_flag = (state->accumulator == 0); 
    state->negative_flag = (state->accumulator & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TYA", state->program_counter - 1);
    #endif
}

void TAY(CPUState *state) {  // eg: TAY
    state->y_register = state->accumulator; 
    state->zero_flag = (state->y_register == 0); 
    state->negative_flag = (state->y_register & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TAY", state->program_counter - 1);
    #endif
}

void TAX(CPUState *state) {  // eg: TAX
    state->x_register = state->accumulator; 
    state->zero_flag = (state->x_register == 0); 
    state->negative_flag = (state->x_register & 0x80) != 0; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: TAX", state->program_counter - 1);
    #endif
}

void SEC(CPUState *state) {  // eg: SEC
    state->carry_flag = 1; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SEC", state->program_counter - 1);
    #endif
}

void SED(CPUState *state) {  // eg: SED
    state->decimal_mode = 1; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SED", state->program_counter - 1);
    #endif
}

void SEI(CPUState *state) {  // eg: SEI
    state->interrupt_disable = 1; 
    state->program_counter += 1; 
    #if debug
    snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: SEI", state->program_counter - 1);
    #endif
}

void trigger_irq(CPUState *state, uint16_t irq_source) {

    state->IRQ_input = false; 

    uint8_t status = ((uint8_t)state->negative_flag     << 7)
                   | ((uint8_t)state->overflow_flag     << 6)
                   | (1                                 << 5)  
                   | (0                                 << 4)  
                   | ((uint8_t)state->decimal_mode      << 3)
                   | ((uint8_t)state->interrupt_disable << 2)
                   | ((uint8_t)state->zero_flag         << 1)
                   |  (uint8_t)state->carry_flag;

    push_stack(state, (state->program_counter >> 8) & 0xFF);
    push_stack(state, state->program_counter & 0xFF);
    push_stack(state, status);
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
    [0x19] = SYSCALL,               // you can call c routine from here
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
        state->NMI_input = 0; 
        #if debug
        snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: NMI", state->nmi_vector);
        #endif
        trigger_irq(state, state->nmi_vector);
        return;
    }

    if (state->IRQ_input) {
        state->IRQ_input = false; 
        #if debug
        snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: IRQ", state->irq_vector);
        #endif
        trigger_irq(state, state->irq_vector);
        return;
    }

    if (opcode_functions[opcode]) {
        opcode_functions[opcode](state);
    } else {
        snprintf(state->disassembly, sizeof(state->disassembly), "$%04X: ??? (Unknown opcode #$%02X)", state->program_counter, opcode);
        state->halt_reason = ILLEGAL_OPCODE_REASON;
        state->halt_flag = true;
    }
}