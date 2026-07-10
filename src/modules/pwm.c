#include <stdio.h>
#include <stdint.h>
#include "pwm.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "memmap.h"

int8_t pin_slice_map[31] = {[0 ... 30] = -1}; 
uint16_t pwm_wrap[8];
uint16_t pwm_freq[8];
uint16_t pwm_lvl[8];

void ninja_pwm_init(uint8_t pin){
    gpio_set_function(pin, GPIO_FUNC_PWM); 
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pin_slice_map[pin] = slice_num; 
    
    
    pwm_config config = pwm_get_default_config();
    
    pwm_config_set_clkdiv(&config, 1.0f);
    
    pwm_config_set_wrap(&config, 2048);
    pwm_init(slice_num, &config, false); 
}

uint16_t pwm_calculate_wrap(uint32_t freq) {
    
    uint32_t sys_clock = CPU_CLOCK_KHZ * 1000;
    
    
    uint32_t wrap = (uint32_t)(sys_clock/ freq);

    if (freq < 1908){
        wrap = 65535; 
    }
    return (uint16_t)wrap;
}

void ninja_pwm_set_frequency(uint8_t pin, uint32_t frequency){
    uint16_t wrap = pwm_calculate_wrap(frequency);
    pwm_wrap[pwm_gpio_to_slice_num(pin)] = wrap; 
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_enabled(pin_slice_map[pin], false); 
    pwm_set_wrap(slice, wrap); 
}

void ninja_pwm_set_duty(uint8_t pin, uint16_t duty){
    pwm_set_gpio_level(pin, duty ); 
}

void ninja_pwm_set_enabled(uint8_t pin, uint8_t enabled) {
    int8_t slice = pin_slice_map[pin];
    if (slice != -1) { 
        pwm_set_enabled(slice, enabled); 
    } 
}

void ninja_pwm_deinit(uint8_t pin){
    ninja_pwm_set_enabled(pin, false); 
    gpio_set_function(pin, GPIO_FUNC_SIO); 
}
