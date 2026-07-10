#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <stdbool.h>

#define MEMORY_SIZE 65536

typedef struct __attribute__((packed)){
    uint8_t port_a;  // Control register for GPIO direction and pull-up configuration
    uint8_t port_b;   // Status register for GPIO input values
    uint8_t port_a_ddr;  // Data Direction Register for Port A
    uint8_t port_b_ddr;  // Data Direction Register for Port B
    uint8_t timer_a_counterL;  // Timer A low byte
    uint8_t timer_a_counterH;  // Timer A high byte
    uint8_t timer_b_counterL;  // Timer B low byte
    uint8_t timer_b_counterH;  // Timer B high byte
    uint8_t tod_alarm0;  // Time-of-Day register 0
    uint8_t tod_alarm1;  // Time-of-Day register 1
    uint8_t tod_alarm2;  // Time-of-Day register 2
    uint8_t serial_data;  // Serial data register
    uint8_t interrupt_control;  // Interrupt control register
    uint8_t timer_a_control;  // Timer A control register
    uint8_t timer_b_control;  // Timer B control register
} CIA_WriteState;

// maybe a better idea to use status register 8 bit but now it is more readable
typedef struct {
    uint8_t accumulator;
    uint8_t x_register;
    uint8_t y_register;
    uint8_t stack_pointer;
    uint16_t program_counter;
    uint16_t reset_vector;
    uint16_t irq_vector;
    uint16_t nmi_vector;
    
    uint32_t gpio_state;  // Store the state of GPIO pins (0-7) in a single uint32_t variable
    uint32_t gpio_direction;  // Store the direction of GPIO pins (0-7) in a single uint32_t variable
    uint32_t gpio_pullup;    // Store the pull-up state of GPIO pins (0-7) in a single uint32_t variable

    CIA_WriteState cia_write;  // Store the write state of the CIA chip
    uint32_t mops;  // Store the number of million operations per second (MOPS)
    bool halt_flag;
    uint8_t halt_reason;  // Reason for halting the CPU (0: None, 1: BRK, 2: HALT instruction, etc.)
    uint16_t halt_address;  // Address where the CPU halted
    
    uint8_t pico_irq_source;  // Store the source of the Pico's IRQ (0: None, 1: Timer, 2: GPIO, etc.)
    // bit 0: GPIO
    // bit 1: Timer
    // bit 2: DMA
    // bit 3: I2C
    // bit 4: PWM
    // bit 5: UART
    // bit 6: ADC
    // bit 7: Other

    bool frame_ready_flag;  // Flag to indicate if a new frame is ready

    bool zero_flag;
    bool carry_flag;
    bool interrupt_disable;
    bool decimal_mode;
    bool overflow_flag;
    bool negative_flag;
    bool break_command;
    bool dirty_screen;  // set when screen RAM ($0400-$07E7) is written
    bool dirty_sprite;  // set when sprite RAM ($07F8-$07FF) is written
    bool IRQ_input;     // set when IRQ line is active
    bool NMI_input;     // set when NMI line is active
    char disassembly[52];
} CPUState;

typedef struct {
    uint16_t address;  // The address to hook
    void (*hook_function)(CPUState *state);  // Pointer to the hook function
} KernalHook_t;

#endif // STRUCTS_H
