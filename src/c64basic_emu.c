#include "enums.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <pico/mutex.h>
#include "hardware/timer.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "tusb.h"        
#include "c64basic_emu.h"
#include "6502_opcodes.h"
#include "palette256.h"
#include "terminalninja.h"
#include "keymap.h"
#include "syscalls.h"
#include "pwm.h"
#include "ringbuffer.h"

CPUState state = {0};

extern uint16_t pwm_wrap[8];
extern uint16_t pwm_freq[8];
extern uint16_t pwm_lvl[8];
extern uint16_t dmasrc;
extern uint16_t dmadst;
extern uint16_t dma_transfer_count;
extern uint8_t dma_incr;
extern uint8_t dma_size;

#define RESET_PIN 23  

void core1_entry();

static uint8_t to_bcd(uint8_t value) {
    return ((value / 10) << 4) | (value % 10);
}

static void inject_key(uint8_t petscii) {

    memory[C64_KEY_BUFFER + memory[C64_KEY_COUNT]] = petscii;
    memory[C64_KEY_COUNT]++;
    while(memory[C64_KEY_COUNT] > 4) {
        sleep_ms(10);  
    }
}

void print_bin(unsigned int v) {
    for (int i = sizeof(v)*8 - 1; i >= 0; i--) {
        putchar((v & (1u << i)) ? '1' : '0');
    }
}


void to_hex8(uint8_t value, char *buffer) {
    const char hex_chars[] = "0123456789ABCDEF";
    buffer[0] = hex_chars[(value >> 4) & 0x0F];
    buffer[1] = hex_chars[value & 0x0F];
    buffer[2] = '\0';
}

void to_hex16(uint16_t value, char *buffer) {
    const char hex_chars[] = "0123456789ABCDEF";
    buffer[0] = hex_chars[(value >> 12) & 0x0F];
    buffer[1] = hex_chars[(value >> 8) & 0x0F];
    buffer[2] = hex_chars[(value >> 4) & 0x0F];
    buffer[3] = hex_chars[value & 0x0F];
    buffer[4] = '\0';
}

bool is_buffering = false;
char cmdbuffer[32] = {0};
void put_char(char b) {
    static int buffer_index = 0;
    if (!is_buffering && b != '{' && b != 0x1b) {
        inject_key(ascii_to_petscii(b)); 
        return;
    } else if ((b == '{' || b == 0x1b) && !is_buffering)
    {
        is_buffering = true;
        buffer_index = 0;
    } 
    if (is_buffering)
    {
        cmdbuffer[buffer_index++] = b;
        cmdbuffer[buffer_index+1] = '\0'; 
        if (buffer_index + 1 > sizeof(cmdbuffer) - 1) {
            state.halt_reason = KEYBOARD_BUFFER_OVERFLOW_REASON;  
            memcpy(state.disassembly, cmdbuffer, sizeof(cmdbuffer));  
        }
        for (int i = 0; i < sizeof(terminal_keys) / sizeof(terminal_keys[0]); i++) {
            if (strcmp(cmdbuffer, terminal_keys[i].seq) == 0) {
                inject_key(terminal_keys[i].c64_keycode);
                is_buffering = false;
                buffer_index = 0;
                break;
            }
        }
        if (b == '}' || b == '\n' || b == '\r') 
        {
            if (cmdbuffer[1]=='$'){
                
                uint8_t hex_value = (uint8_t)strtol(&cmdbuffer[2], NULL, 16);
                inject_key(hex_value);
            }
            is_buffering = false;  
            buffer_index = 0;  
        }
    } 
}

void reset_irq(uint gpio, uint32_t events) {
    if (gpio == RESET_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        state.halt_flag = true;
        reset_cpu(&state);
        state.halt_flag = false;
    }
}

bool timer_callback() {
    memory[0xD012]++;
    return true;  
}

int main()
{

    
    vreg_set_voltage(CPU_CORE_VOLTAGE);
    sleep_ms(50);  
    set_sys_clock_khz(CPU_CLOCK_KHZ, true);
    sleep_ms(50);  
    stdio_init_all();
    sleep_ms(1000);  

    uint8_t counter = 0;
    uint64_t timing=0;
    int sox = BORDER_SIDE;
    int soy = BORDER_TOP;
    bool blink = true;  
    int increment = 1;  
    uint32_t last_irq_us = time_us_32();  
    uint8_t last_key_pressed = 0;
    Buffer *border_buffer = create_buffer(TOTAL_W, TOTAL_H, ' ', '@'); 
    SpriteAsset_t sprites[SPRITE_LEN] = {0};  
    memset(sprites, 0, sizeof(sprites));  
    multicore_launch_core1(core1_entry);  
    gpio_set_irq_enabled_with_callback(
        RESET_PIN,
        GPIO_IRQ_EDGE_FALL,
        true,
        &reset_irq
    );
    
    repeating_timer_t timer;
    add_repeating_timer_us(-1000000 / 250, timer_callback, NULL, &timer);

    init_terminal(TOTAL_W, TOTAL_H);
    clear_terminal();
    init_big_characters('a', ' ', 31);
    set_framerate(50); 
    char life=0;
    set_palette(palette256, sizeof(palette256) / sizeof(palette256[0]));
    int previous_fifo_index = -1;  
    ringbuffer_init();  
    while (true) 
    {
        if (state.halt_flag && state.halt_reason != 0) {
            memset(screen, 0x20, FRAME_WIDTH * FRAME_HEIGHT);  
            memset(color_data, 0x01, FRAME_WIDTH * FRAME_HEIGHT);  
            memset(background, 0x00, FRAME_WIDTH * FRAME_HEIGHT);  
            render_frame();
            printf("\033[2j\033[0m");  
            printf("\033[31m"); 
            printf("\033[0;0H"); 
            printf("****************************************************\n");
            printf("*                  SOFTWARE ERROR                  *\n");
            printf("*              Press RESET to restart              *\n");
            printf("****************************************************\n");
            printf(" %s \n", halt_reason[state.halt_reason]);
            printf(" %s\n", state.disassembly);
            printf(" CPU halted. Check the disassembly for details. *\n");
            printf(" PC:$%04X, SP:$%02X, A:$%02X, X:$%02X, Y:$%02X\n", state.program_counter, state.stack_pointer, state.accumulator, state.x_register, state.y_register);
            printf(" Flags: N=%d V=%d D=%d I=%d Z=%d C=%d\n", state.negative_flag, state.overflow_flag, state.decimal_mode, state.interrupt_disable, state.zero_flag, state.carry_flag);
            printf("\033[0m");  
            while (gpio_get(23) == 1) {
                reset_cpu(&state);
            }
        }

        while (tud_cdc_available() && ringbuffer_available() > 1) {  
            char c = tud_cdc_read_char();
            ringbuffer_push((uint8_t)c);  
        }

        frame_start();
        memcpy(sprites, &memory[SPRITE_START_ADDRESS], sizeof(sprites));
        state.dirty_sprite = false;  
        hide_terminal_cursor();
        fill_background_buffer(border_buffer, memory[BORDER_COLOR_ADDRESS]); 
        copy_to_screen(border_buffer, 0, 0); 
        printf_atc(0, 0, 0, "%d", state.mops); 
        uint8_t cur_x = memory[CURSOR_X_ADDRESS];     
        uint8_t cur_y = memory[CURSOR_Y_ADDRESS];     
        uint8_t cur_enable = memory[CURSOR_ENABLE_ADDRESS];  
        set_cursor_position(cur_x + sox, cur_y + soy);
        if (cur_enable == 0) {
            if (blink) {
                show_cursor(); 
            } else {
                hide_cursor(); 
            }
        } else {
            hide_cursor(); 
        }

        for (int i = 0; i < 25; i++)
        {
            memcpy(&screen[(i + soy) * FRAME_WIDTH + sox], &memory[TEXT_SCREEN_START + i * 40], 40);
            memcpy(&color_data[(i + soy) * FRAME_WIDTH + sox], &memory[COLOR_RAM_START + i * 40], 40);
            memset(&background[(i + soy) * FRAME_WIDTH + sox], memory[BACKGROUND_COLOR_ADDRESS], 40);
        }
        
        for (int i = 0; i < SPRITE_LEN; i++) {
            if (sprites[i].enabled) {
                blit_sprite_to_screen(&sprites[i]);
            }
        }

    #if use_host
        send_screen_packet(blink);
    #else
        state.frame_ready_flag = true;  
        memory[FRAME_READY_ADDRESS] = 1;  
        render_frame();
        
        
        
        
    #endif
        frame_end();
        if (time_us_64() - timing > 500000) {  
            timing = time_us_64();
            blink = !blink; 
        }
    }
    return 0;
}

void core1_entry() {

    
    uint32_t counter = 0;
    uint32_t mops = 0;

    
    uint8_t i2c_ctrl_old  = 0;
    uint8_t i2c_speed_old = 0xFF;  

    uint32_t time_offset = 0;
    uint32_t last_counter = 0;

    reset_cpu(&state);

    gpio_init(23);
    gpio_set_dir(23, false);
    gpio_pull_up(23);

    #define I2C_SIZE (I2C_SPEED_ADDR - I2C_CTRL_ADDR + 1)
    memset(&memory[I2C_CTRL_ADDR], 0, I2C_SIZE);

    uint32_t last_tick_us = time_us_32();  
    uint8_t last_key_count = 10;  
    uint32_t retrace_time_us = 0;  
    int previous_fifo_index = -1;  
        
    while (true) {

        counter++;

        if (memory[C64_KEY_COUNT] < 2) {  
            char key;
            if (ringbuffer_pop((uint8_t*)&key)) {  
                put_char(key);
            }
        }

         if (!state.halt_flag) \
        {
            execute_opcode(&state, read_memory(&state, state.program_counter));
        }

        if (last_tick_us + 1000 <= time_us_32()) {
            state.mops = counter - last_counter;
            last_counter = counter;
            last_tick_us = time_us_32();
        }
    }
}