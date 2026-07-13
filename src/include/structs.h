#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <stdbool.h>

#define MEMORY_SIZE 65536

typedef struct __attribute__((packed)){
    uint8_t port_a;  
    uint8_t port_b;   
    uint8_t port_a_ddr;  
    uint8_t port_b_ddr;  
    uint8_t timer_a_counterL;  
    uint8_t timer_a_counterH;  
    uint8_t timer_b_counterL;  
    uint8_t timer_b_counterH;  
    uint8_t tod_alarm0;  
    uint8_t tod_alarm1;  
    uint8_t tod_alarm2;  
    uint8_t serial_data;  
    uint8_t interrupt_control;  
    uint8_t timer_a_control;  
    uint8_t timer_b_control;  
} CIA_WriteState;


typedef struct {
    uint8_t accumulator;
    uint8_t x_register;
    uint8_t y_register;
    int16_t stack_pointer;
    uint16_t program_counter;
    uint16_t reset_vector;
    uint16_t irq_vector;
    uint16_t nmi_vector;
    
    uint32_t gpio_state;  
    uint32_t gpio_direction;  
    uint32_t gpio_pullup;    

    CIA_WriteState cia_write;  
    uint32_t mops;  
    bool halt_flag;
    uint8_t halt_reason;  
    uint16_t halt_address;  
    
    uint8_t pico_irq_source;  
    
    bool frame_ready_flag;  

    bool zero_flag;
    bool carry_flag;
    bool interrupt_disable;
    bool decimal_mode;
    bool overflow_flag;
    bool negative_flag;
    bool break_command;
    bool dirty_screen;  
    bool dirty_sprite;  
    bool IRQ_input;     
    bool NMI_input;     
    char disassembly[52];
} CPUState;

typedef struct {
    uint16_t address;  
    void (*hook_function)(CPUState *state);  
} KernalHook_t;




#endif 
