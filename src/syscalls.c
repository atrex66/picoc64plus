#include "enums.h"
#include "structs.h"
#include "syscalls.h"
#include "pico/stdlib.h"
#include  <stdio.h>
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "pwm.h"

extern uint16_t pwm_wrap[8];
extern uint16_t pwm_freq[8];
extern uint16_t pwm_lvl[8];

extern CPUState state;  // Declare the CPU state as extern
extern uint8_t memory[MEMORY_SIZE];  // Declare the memory array as extern

#define DMA_CHANNEL 11  // Define the DMA channel to use (0-11 for RP2040)
dma_channel_config dma_config = {

};
uint8_t *dma_src_addr;
uint8_t *dma_dst_addr;
uint16_t dmasrc;
uint16_t dmadst;
uint16_t dma_transfer_count;
uint8_t dma_incr;
uint8_t dma_size;


void (*arm_opcode_functions[256])(CPUState *state) = {
    [0x00] = HELLO_FROM_ARM,
    [0x01] = GET_IRQ_SOURCE,
    [0x02] = CLEAR_IRQ_SOURCE,
    [0x03] = SET_GPIO_IRQ,
    [0x04] = DISABLE_GPIO_IRQ,

    [0x80] = PWM_SETUP,  // Setup a pin for PWM output A=PIN NUMBER
    [0x81] = PWM_SET_FREQUENCY,  // Set the frequency of a PWM pin  A = pin, X = Freq low, Y = Freq high
    [0x82] = PWM_SET_DUTY,  // Set the duty cycle of a PWM pin  A = pin, X = Duty Low, Y = Duty High
    [0x83] = PWM_CONTROL, // Control PWM output (enable/disable) A=Pin, X=Control (0=disable, 1=enable)
    [0x84] = PWM_DEINIT, // Deinitialize PWM for a pin A=Pin

    [0x90] = GPIO_SET_PINMODE,  // Set GPIO pin mode (input/output) A=Pin, X=Mode (0=input, 1=output)
    [0x91] = GPIO_SET_PINOUT,  // Set GPIO pin output value A=Pin, X=Value (0=low, 1=high)
    [0x92] = GPIO_SET_PINPULL,  // Set GPIO pin pull-up/pull-down A=Pin, X=Pull (0=no pull, 1=pull-up, 2=pull-down)
    [0x93] = GPIO_READ_PIN,  // Read GPIO pin value A=Pin, returns value in accumulator (0=low, 1=high)

    [0xA0] = ADC_READ, // Read ADC value A=ADC channel (0-3), returns value in X/Y registers (X=low byte, Y=high byte)
    [0xA1] = ADC_INIT, // Initialize the ADC hardware (all channels)

    [0xB0] = DMA_INIT,
    [0xB1] = DMA_SET_SIZE,
    [0xB2] = DMA_INCREMENT,
    [0xB3] = DMA_SET_TRANSFER_COUNT,
    [0xB4] = DMA_SET_SRC,
    [0xB5] = DMA_SET_DST,
    [0xB6] = DMA_START,
    [0xB7] = DMA_WAIT_FOR_COMPLETE,
    [0xB8] = DMA_ABORT,
    [0xB9] = DMA_GET_STATUS,

    [0xC0] = RENDER_WAIT_FRAME
};

void gpio_irq_callback(uint gpio, uint32_t events) {
    // Set the IRQ source in the CPU state based on the GPIO pin that triggered the interrupt
    state.pico_irq_source |= (1 << 0);  // Set bit 0 for GPIO IRQ
    state.IRQ_input = true;  // Set the IRQ input flag
}

void HELLO_FROM_ARM(CPUState *state) 
{
    state->halt_reason = HELLO_REASON;
    state->halt_flag = true;
}

void GET_IRQ_SOURCE(CPUState *state) 
{
    state->accumulator = state->pico_irq_source;  // Return the current IRQ source in the accumulator
}

void CLEAR_IRQ_SOURCE(CPUState *state) 
{
    state->pico_irq_source = 0;  // Clear the IRQ source
}

void SET_GPIO_IRQ(CPUState *state) 
{
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    uint8_t edge = state->x_register;  // Get the edge type from the X register (0=falling, 1=rising)
    if (edge == 0) {
        gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_callback);
    } else if (edge == 1) {
        gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_RISE, true, &gpio_irq_callback);
    }
}

void DISABLE_GPIO_IRQ(CPUState *state) 
{
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);  // Disable both edges
}

// ---------------- start of PWM functions ----------------
void PWM_SETUP(CPUState *state) {
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    ninja_pwm_init(pin);  // Initialize the PWM for the specified pin
}

void PWM_SET_FREQUENCY(CPUState *state) {
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    uint8_t freq_low = state->x_register;  // Get the low byte of the frequency from the accumulator
    uint8_t freq_high = state->y_register;  // Get the high byte of the frequency from the Y register
    uint32_t frequency = (freq_high << 8) | freq_low;  // Combine to form the full frequency value
    ninja_pwm_set_frequency(pin, frequency);  // Set the frequency for the specified pin
    pwm_freq[pwm_gpio_to_slice_num(pin)] = frequency; // Store the frequency for the slice
}

void PWM_SET_DUTY(CPUState *state) {
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    uint8_t duty_low = state->x_register;  // Get the low byte of the duty cycle from the X register
    uint8_t duty_high = state->y_register;  // Get the high byte of the duty cycle from the Y register
    uint16_t duty = (duty_high << 8) | duty_low;  // Combine to form the full duty cycle value
    ninja_pwm_set_duty(pin, duty);  // Set the duty cycle for the specified pin
    pwm_lvl[pwm_gpio_to_slice_num(pin)] = duty; // Store the duty cycle for the slice
}

void PWM_CONTROL(CPUState *state) {
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    uint8_t control = state->x_register;  // Get the control value from the X register (0=disable, 1=enable)
    if (control == 0) {
        ninja_pwm_set_enabled(pin, false);  // Disable PWM output for the specified pin
    } else if (control == 1) {
        ninja_pwm_set_enabled(pin, true);  // Enable PWM output for the specified pin
    }
}

void PWM_DEINIT(CPUState *state) {
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    ninja_pwm_deinit(pin);  // Deinitialize the PWM for the specified pin
}

// ---------------- start of gpio functions ----------------

void GPIO_SET_PINMODE(CPUState *state) {
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    uint8_t mode = state->x_register;   // Get the mode from the X register (0=input, 1=output)
    gpio_init(pin);  // Initialize the GPIO pin
    gpio_set_dir(pin, mode == 1);  // Set pin as output if mode is 1, otherwise input
}

void GPIO_SET_PINOUT(CPUState *state) {
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    uint8_t value = state->x_register;  // Get the output value from the X register (0=low, 1=high)
    gpio_put(pin, value);  // Set the pin output value
}

void GPIO_SET_PINPULL(CPUState *state) {
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    uint8_t pull = state->x_register;   // Get the pull configuration from the X register (0=no pull, 1=pull-up, 2=pull-down)
    if (pull == 0) {
        gpio_disable_pulls(pin);  // Disable pull-up/pull-down
    } else if (pull == 1) {
        gpio_pull_up(pin);         // Enable pull-up
    } else if (pull == 2) {
        gpio_pull_down(pin);       // Enable pull-down
    }
}

void GPIO_READ_PIN(CPUState *state) {
    uint8_t pin = state->accumulator;  // Get the pin number from the accumulator
    state->accumulator = gpio_get(pin);  // Read the pin value and store it in the accumulator
}


// ---------------- start of ADC functions ----------------
void ADC_INIT(CPUState *state) {
    adc_init();  // Initialize the ADC hardware
}

#define TEMPERATURE_UNITS 'C'

/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */
int16_t read_onboard_temperature(const char unit) {
    
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = (27.0f - (adc - 0.706f) / 0.001721f) * 10.0F;

    if (unit == 'C') {
        return (int16_t)(tempC);
    } else if (unit == 'F') {
        return (int16_t)(tempC * 9 / 5 + 32);
    }

    return (int16_t)(-100);
}

void ADC_READ(CPUState *state) {
    uint8_t channel = state->accumulator;  // Get the ADC channel from the accumulator
    // Select ADC input 0 (GPIO26)
    uint8_t pin = 26 + channel;  // Calculate the GPIO pin corresponding to the ADC channel
    if ((adc_hw->cs & 0x01) != ADC_CS_EN_BITS)
    {
        adc_init();  // Initialize the ADC hardware
        adc_run(true);  // Start the ADC
        adc_set_temp_sensor_enabled(true);
    }
    adc_gpio_init(pin);  // Initialize the GPIO pin for ADC input
    adc_select_input(channel);
    if (channel < NUM_ADC_CHANNELS) {  // Ensure the channel is valid (0-8)
        int16_t adc_value = adc_read();  // Read the ADC value for the specified channel
        state->x_register = adc_value & 0xFF;  // Store the low byte in the X register
        state->y_register = (adc_value >> 8) & 0xFF;  // Store the high byte in the Y register
        state->accumulator = 0;  // Indicate success in the accumulator
    } else {
        adc_select_input(4);  // Select the temperature sensor input
        int16_t adc_value = read_onboard_temperature(TEMPERATURE_UNITS);  // Read the ADC value for the specified channel
        state->x_register = adc_value & 0xFF;  // Store the low byte in the X register
        state->y_register = (adc_value >> 8) & 0xFF;  // Store the high byte in the Y register
        state->accumulator = 0;  // Indicate success in the accumulator
    }
}


void DMA_INIT(CPUState *state) {
    // Initialize DMA controller
    if (dma_channel_is_claimed(DMA_CHANNEL)) {
        dma_channel_unclaim(DMA_CHANNEL);  // Unclaim the channel if already claimed
    } 
    dma_channel_claim(DMA_CHANNEL);  // Claim the DMA channel if not already claimed
    dma_config = dma_channel_get_default_config(DMA_CHANNEL);  // Get default configuration for channel 0
}



void DMA_SET_SIZE(CPUState *state) {
    // Set the transfer size (8, 16, or 32 bits)
    uint8_t size = state->accumulator;  // Get the size from the accumulator
    dma_size = size;  // Store the size for later use
    switch (size) {
        case 0:  // 8 bits
            channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_8);
            break;
        case 1:  // 16 bits
            channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_16);
            break;
        case 2:  // 32 bits
            channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
            break;
        default:
            state->accumulator = 1;  // Invalid size, set error in accumulator
            return;
    }
    state->accumulator = 0;  // Indicate success in the accumulator
}

void DMA_INCREMENT(CPUState *state) {
    // Set whether to increment source and destination addresses
    uint8_t src_increment = state->x_register;  // Get the increment setting from the accumulator
    uint8_t dst_increment = state->y_register;  // Get the destination increment setting from the X register
    dma_incr = (src_increment & 0x01) | ((dst_increment & 0x01) << 1);  // Store increment settings for later use
}

void DMA_SET_TRANSFER_COUNT(CPUState *state) {
    // Set the number of transfers for the DMA operation
    dma_transfer_count = (state->y_register << 8) | state->x_register;  // Combine X and Y registers for transfer count
}

void DMA_SET_SRC(CPUState *state) {
    // Set the source address for the DMA transfer
    dmasrc = (state->y_register << 8) | state->x_register;  // Combine X and Y registers for source address
    dma_src_addr = &memory[dmasrc];  // Use the combined source address
}

void DMA_SET_DST(CPUState *state) {
    // Set the destination address for the DMA transfer
    dmadst = (state->y_register << 8) | state->x_register;  // Combine X and Y registers for destination address
    dma_dst_addr = &memory[dmadst];  // Use the combined destination address
}

void DMA_START(CPUState *state) {
    // 6502 címek -> memory[] pointerek
    uint8_t *src_ptr = &memory[dmasrc];
    uint8_t *dst_ptr = &memory[dmadst];

    channel_config_set_read_increment(&dma_config, dma_incr & 0x01);  // Set source increment
    channel_config_set_write_increment(&dma_config, (dma_incr >> 1) & 0x01);  // Set destination increment

    // channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_8);
    channel_config_set_dreq(&dma_config, DREQ_FORCE);
    channel_config_set_irq_quiet(&dma_config, true);

    dma_channel_configure(
        DMA_CHANNEL,
        &dma_config,
        dst_ptr,
        src_ptr,
        dma_transfer_count,
        true
    );
    
    state->accumulator = 0;  // Indicate success in the accumulator
}

void DMA_WAIT_FOR_COMPLETE(CPUState *state) {
    // Wait for the DMA transfer to complete
    dma_channel_wait_for_finish_blocking(DMA_CHANNEL);  // Block until the DMA transfer is complete
}

void DMA_ABORT(CPUState *state) {
    // Abort the DMA transfer
    dma_channel_abort(DMA_CHANNEL);  // Abort the DMA channel
    dma_src_addr = 0;  // Clear the stored source address
    dma_dst_addr = 0;  // Clear the stored destination address
    dma_transfer_count = 0;  // Clear the stored transfer count
}

void DMA_GET_STATUS(CPUState *state) {
    // Get the status of the DMA transfer
    uint32_t status = dma_channel_is_busy(DMA_CHANNEL) ? 1 : 0;  // Check if the DMA channel is busy
    state->accumulator = status;  // Store the status in the accumulator (0=idle, 1=busy)
}

void RENDER_WAIT_FRAME(CPUState *state) {
    // Wait for the next frame to be ready
    memory[0xd013] = 0;  // Clear the frame ready flag in memory
    while (memory[0xd013] == 0) {
        tight_loop_contents();  // Wait until the frame_ready_flag is set
    }
}