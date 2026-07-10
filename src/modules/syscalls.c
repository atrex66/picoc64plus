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

extern CPUState state;  
extern uint8_t memory[MEMORY_SIZE];  

extern void reset_cpu(CPUState *state);

#define DMA_CHANNEL 11  
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
    [0x05] = RESET_CPU_SYS,
    
    [0x80] = PWM_SETUP,  
    [0x81] = PWM_SET_FREQUENCY,  
    [0x82] = PWM_SET_DUTY,  
    [0x83] = PWM_CONTROL, 
    [0x84] = PWM_DEINIT, 

    [0x90] = GPIO_SET_PINMODE,  
    [0x91] = GPIO_SET_PINOUT,  
    [0x92] = GPIO_SET_PINPULL,  
    [0x93] = GPIO_READ_PIN,  

    [0xA0] = ADC_READ, 
    [0xA1] = ADC_INIT, 

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


void RESET_CPU_SYS(CPUState *state) {
    state->halt_flag = true;
    reset_cpu(state);
    state->halt_flag = false;
}

void gpio_irq_callback(uint gpio, uint32_t events) {
    
    state.pico_irq_source |= (1 << 0);  
    state.IRQ_input = true;  
}

void HELLO_FROM_ARM(CPUState *state) 
{
    state->halt_reason = HELLO_REASON;
    state->halt_flag = true;
}

void GET_IRQ_SOURCE(CPUState *state) 
{
    state->accumulator = state->pico_irq_source;  
}

void CLEAR_IRQ_SOURCE(CPUState *state) 
{
    state->pico_irq_source = 0;  
}

void SET_GPIO_IRQ(CPUState *state) 
{
    uint8_t pin = state->accumulator;  
    uint8_t edge = state->x_register;  
    if (edge == 0) {
        gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_callback);
    } else if (edge == 1) {
        gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_RISE, true, &gpio_irq_callback);
    }
}

void DISABLE_GPIO_IRQ(CPUState *state) 
{
    uint8_t pin = state->accumulator;  
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);  
}


void PWM_SETUP(CPUState *state) {
    uint8_t pin = state->accumulator;  
    ninja_pwm_init(pin);  
}

void PWM_SET_FREQUENCY(CPUState *state) {
    uint8_t pin = state->accumulator;  
    uint8_t freq_low = state->x_register;  
    uint8_t freq_high = state->y_register;  
    uint32_t frequency = (freq_high << 8) | freq_low;  
    ninja_pwm_set_frequency(pin, frequency);  
    pwm_freq[pwm_gpio_to_slice_num(pin)] = frequency; 
}

void PWM_SET_DUTY(CPUState *state) {
    uint8_t pin = state->accumulator;  
    uint8_t duty_low = state->x_register;  
    uint8_t duty_high = state->y_register;  
    uint16_t duty = (duty_high << 8) | duty_low;  
    ninja_pwm_set_duty(pin, duty);  
    pwm_lvl[pwm_gpio_to_slice_num(pin)] = duty; 
}

void PWM_CONTROL(CPUState *state) {
    uint8_t pin = state->accumulator;  
    uint8_t control = state->x_register;  
    if (control == 0) {
        ninja_pwm_set_enabled(pin, false);  
    } else if (control == 1) {
        ninja_pwm_set_enabled(pin, true);  
    }
}

void PWM_DEINIT(CPUState *state) {
    uint8_t pin = state->accumulator;  
    ninja_pwm_deinit(pin);  
}



void GPIO_SET_PINMODE(CPUState *state) {
    uint8_t pin = state->accumulator;  
    uint8_t mode = state->x_register;   
    gpio_init(pin);  
    gpio_set_dir(pin, mode == 1);  
}

void GPIO_SET_PINOUT(CPUState *state) {
    uint8_t pin = state->accumulator;  
    uint8_t value = state->x_register;  
    gpio_put(pin, value);  
}

void GPIO_SET_PINPULL(CPUState *state) {
    uint8_t pin = state->accumulator;  
    uint8_t pull = state->x_register;   
    if (pull == 0) {
        gpio_disable_pulls(pin);  
    } else if (pull == 1) {
        gpio_pull_up(pin);         
    } else if (pull == 2) {
        gpio_pull_down(pin);       
    }
}

void GPIO_READ_PIN(CPUState *state) {
    uint8_t pin = state->accumulator;  
    state->accumulator = gpio_get(pin);  
}



void ADC_INIT(CPUState *state) {
    adc_init();  
}

#define TEMPERATURE_UNITS 'C'


int16_t read_onboard_temperature(const char unit) {
    
    
    const float conversionFactor = 3.3f / (1 << 12);
    adc_select_input(4);
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
    uint8_t channel = state->accumulator;  
    
    uint8_t pin = 26 + channel;  
    if ((adc_hw->cs & 0x01) != ADC_CS_EN_BITS)
    {
        adc_init();  
        adc_run(true);  
        adc_set_temp_sensor_enabled(true);
    }
    adc_gpio_init(pin);  
    adc_select_input(channel);
    if (channel < NUM_ADC_CHANNELS) {  
        int16_t adc_value = adc_read();  
        state->x_register = adc_value & 0xFF;  
        state->y_register = (adc_value >> 8) & 0xFF;  
        state->accumulator = 0;  
    } else {
        adc_select_input(4);  
        int16_t adc_value = read_onboard_temperature(TEMPERATURE_UNITS);  
        state->x_register = adc_value & 0xFF;  
        state->y_register = (adc_value >> 8) & 0xFF;  
        state->accumulator = 0;  
    }
}


void DMA_INIT(CPUState *state) {
    
    if (dma_channel_is_claimed(DMA_CHANNEL)) {
        dma_channel_unclaim(DMA_CHANNEL);  
    } 
    dma_channel_claim(DMA_CHANNEL);  
    dma_config = dma_channel_get_default_config(DMA_CHANNEL);  
}



void DMA_SET_SIZE(CPUState *state) {
    
    uint8_t size = state->accumulator;  
    dma_size = size;  
    switch (size) {
        case 0:  
            channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_8);
            break;
        case 1:  
            channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_16);
            break;
        case 2:  
            channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
            break;
        default:
            state->accumulator = 1;  
            return;
    }
    state->accumulator = 0;  
}

void DMA_INCREMENT(CPUState *state) {
    
    uint8_t src_increment = state->x_register;  
    uint8_t dst_increment = state->y_register;  
    dma_incr = (src_increment & 0x01) | ((dst_increment & 0x01) << 1);  
}

void DMA_SET_TRANSFER_COUNT(CPUState *state) {
    
    dma_transfer_count = (state->y_register << 8) | state->x_register;  
}

void DMA_SET_SRC(CPUState *state) {
    
    dmasrc = (state->y_register << 8) | state->x_register;  
    dma_src_addr = &memory[dmasrc];  
}

void DMA_SET_DST(CPUState *state) {
    
    dmadst = (state->y_register << 8) | state->x_register;  
    dma_dst_addr = &memory[dmadst];  
}

void DMA_START(CPUState *state) {
    
    uint8_t *src_ptr = &memory[dmasrc];
    uint8_t *dst_ptr = &memory[dmadst];

    channel_config_set_read_increment(&dma_config, dma_incr & 0x01);  
    channel_config_set_write_increment(&dma_config, (dma_incr >> 1) & 0x01);  

    
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
    
    state->accumulator = 0;  
}

void DMA_WAIT_FOR_COMPLETE(CPUState *state) {
    
    dma_channel_wait_for_finish_blocking(DMA_CHANNEL);  
}

void DMA_ABORT(CPUState *state) {
    
    dma_channel_abort(DMA_CHANNEL);  
    dma_src_addr = 0;  
    dma_dst_addr = 0;  
    dma_transfer_count = 0;  
}

void DMA_GET_STATUS(CPUState *state) {
    
    uint32_t status = dma_channel_is_busy(DMA_CHANNEL) ? 1 : 0;  
    state->accumulator = status;  
}

void RENDER_WAIT_FRAME(CPUState *state) {
    
    memory[0xd013] = 0;  
    while (memory[0xd013] == 0) {
        tight_loop_contents();  
    }
}