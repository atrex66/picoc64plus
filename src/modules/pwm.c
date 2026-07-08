#include <stdio.h>
#include <stdint.h>
#include "pwm.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "memmap.h"

int8_t pin_slice_map[31] = {[0 ... 30] = -1}; // Initialize all pins to -1 (not assigned)
uint16_t pwm_wrap[8];
uint16_t pwm_freq[8];
uint16_t pwm_lvl[8];

void ninja_pwm_init(uint8_t pin){
    gpio_set_function(pin, GPIO_FUNC_PWM); // Set PWM pin function
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pin_slice_map[pin] = slice_num; // Map the pin to its PWM slice
    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 1.0f);
    // Set the PWM frequency to 1kHz
    pwm_config_set_wrap(&config, 2048);
    pwm_init(slice_num, &config, false); // Initialize the PWM slice with the config
}

uint16_t pwm_calculate_wrap(uint32_t freq) {
    // Rendszer órajel lekérése (alapértelmezetten 125 MHz az RP2040 esetén)
    uint32_t sys_clock = CPU_CLOCK_KHZ * 1000;
    
    // Wrap kiszámítása fix 1.0 divider-rel
    uint32_t wrap = (uint32_t)(sys_clock/ freq);

    if (freq < 1908){
        wrap = 65535; // 65535 is the maximum wrap value for 16-bit PWM
    }
    return (uint16_t)wrap;
}

void ninja_pwm_set_frequency(uint8_t pin, uint32_t frequency){
    uint16_t wrap = pwm_calculate_wrap(frequency);
    pwm_wrap[pwm_gpio_to_slice_num(pin)] = wrap; // Store the wrap value for the slice
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_enabled(pin_slice_map[pin], false); // Disable PWM output before changing wrap
    pwm_set_wrap(slice, wrap); // Set the PWM wrap value
}

void ninja_pwm_set_duty(uint8_t pin, uint16_t duty){
    pwm_set_gpio_level(pin, duty ); // Set PWM value
}

void ninja_pwm_set_enabled(uint8_t pin, uint8_t enabled) {
    int8_t slice = pin_slice_map[pin];
    if (slice != -1) { // Check if the pin is mapped to a slice
        pwm_set_enabled(slice, enabled); // Enable or disable the PWM slice
    } 
}

void ninja_pwm_deinit(uint8_t pin){
    ninja_pwm_set_enabled(pin, false); // Disable PWM output
    gpio_set_function(pin, GPIO_FUNC_SIO); // Reset the pin function to default (SIO)
}
