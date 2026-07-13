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
#include "6502_disasm.h"
// #include "editor.h"

CPUState state = {0};

extern uint16_t pwm_wrap[8];
extern uint16_t pwm_freq[8];
extern uint16_t pwm_lvl[8];
extern uint16_t dmasrc;
extern uint16_t dmadst;
extern uint16_t dma_transfer_count;
extern uint8_t dma_incr;
extern uint8_t dma_size;
extern bool use_utf8;

#define RESET_PIN 23  

uint8_t irq_counter = 0;
Buffer *border_buffer;
bool debug_mode = false;
typedef void (*key_handler_t)(uint8_t key);
bool is_buffering = false;
char cmdbuffer[32] = {0};
int buffer_index = 0;
key_handler_t current_key_handler = NULL;


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

void put_char(char b) {
    if (!is_buffering && b != '{' && b != 0x1b) {
        if (current_key_handler == inject_key) {
            current_key_handler(ascii_to_petscii(b)); 
        } else {
            current_key_handler(b); 
        }
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
        for (int i = 0; i < sizeof(terminal_keys) / sizeof(terminal_keys[0]); i++) {
            if (strcmp(cmdbuffer, terminal_keys[i].seq) == 0) {
                if (terminal_keys[i].c64_keycode == 0xFF) {
                    debug_mode = !debug_mode;
                    is_buffering = false;
                    buffer_index = 0;
                    return;
                }
                current_key_handler(terminal_keys[i].c64_keycode);
                is_buffering = false;
                buffer_index = 0;
                break;
            }
        }
        if (b == '}' || b == '\n' || b == '\r') 
        {
            if (cmdbuffer[1]=='$'){
                
                uint8_t hex_value = (uint8_t)strtol(&cmdbuffer[2], NULL, 16);
                current_key_handler(hex_value);
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
        ringbuffer_init();
        state.halt_flag = false;
    }
}

bool timer_callback() {
    memory[0xD012]++;
    if (irq_counter++ % 0x05 == 0) {
        if (memory[0xdc0d] && 1) {  
            if (!debug_mode) {
                state.pico_irq_source = 0x01;  
                state.IRQ_input = true;  
            }
        }
    }
    return true;  
}

Buffer *toprow_buffer;
Buffer *editor_buffer;
static int crx = 0;
static int cry = 0;
char code;
bool blink = true;
bool freerun_mode = true;  
uint64_t timing=0;

void init_editor() {
    toprow_buffer = create_buffer(F_W, 1, ' ', 0xFf);
    editor_buffer = create_buffer(F_W, F_H - 1, ' ', 0xFf);
    memset(cmdbuffer, 0, sizeof(cmdbuffer));
    buffer_index = 0;
    is_buffering = false;
}

void handle_editor_input(uint8_t input) {
    code = input;
    switch (input) {
        case 0x9d: 
            if (crx > 0) crx--;
            return;
        case 0x1d: 
            if (crx < editor_buffer->width - 1) crx++;
            return;
        case 0x11: 
            if (cry < editor_buffer->height - 1) cry++;
            return;
        case 0x91: 
            if (cry > 0) cry--;
            return;
        case 0x85: // F5 step
            state.halt_flag = false;
            return;
        case 0x87: // F7 freerun mode
            freerun_mode = !freerun_mode;
            state.halt_flag = !freerun_mode;
            return;
        case 0xfe:
            debug_mode = !debug_mode;
            state.halt_flag = false;
            return;
        default:
            break;
    }
    if (input == '\n' || input == '\r') {
        crx = 0;
        cry++;
        if (cry >= editor_buffer->height) {
            cry = editor_buffer->height - 1;
        }
    } else if (input == 127) {
        if (crx > 0) {
            memcpy(&editor_buffer->data[cry * editor_buffer->width + crx - 1],
                   &editor_buffer->data[cry * editor_buffer->width + crx],
                   editor_buffer->width - crx);
            char_buffer_atc(editor_buffer->width, cry, 0x0f, ' ', editor_buffer);
            crx--;
        }
    } else if (input >= 32 && input <= 126) {
        memmove(&editor_buffer->data[cry * editor_buffer->width + crx + 1],
                &editor_buffer->data[cry * editor_buffer->width + crx],
                editor_buffer->width - crx - 1);
        char_buffer_atc(crx, cry, 0x0f, input, editor_buffer);
        crx++;
        if (crx >= editor_buffer->width) {
            crx = 0;
            cry++;
            if (cry >= editor_buffer->height) {
                cry = editor_buffer->height - 1;
            }
        }
    }
}

void render_editor() {
    blit_to_screen(border_buffer, 0, 0);
    copy_to_screen(toprow_buffer, 6, 2);
    copy_to_screen(editor_buffer, 6, 3);
}

void editor_main_loop() {
    // show_cursor();
    init_editor();
    clear_terminal();
    fill_background_buffer(toprow_buffer, 0x0f);
    // char_buffer_atc(0, 0, 0x00, '>', toprow_buffer);
    use_utf8 = false;
    // state.halt_flag = true;
    char c;
    char *disasm;
    cursor_enable = false;
    uint16_t old_pc = 0;
    char prev_disasm[10][40] = {0};
    char linecolors[10] = {0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x01};
    char disa[40] = {0};
    current_key_handler = handle_editor_input;
    while (debug_mode) { 
        frame_start();
        if (tud_cdc_available()) {
            c = (char)tud_cdc_read_char();
            put_char(c);
        }
        fill_background_buffer(border_buffer, memory[BORDER_COLOR_ADDRESS]); 
        copy_to_screen(border_buffer, 0, 0); 
        printf_atc(0, 0, 0, "%d", state.mops); 
        // state.halt_flag = false;
        if (old_pc != state.program_counter) {
            old_pc = state.program_counter;
            disasm = decode_instruction(state.program_counter);
            snprintf(disa, sizeof(disa), "$%04X: %s", state.program_counter, disasm);
            for (int i = 9; i > 0; i--) {
                strncpy(prev_disasm[i], prev_disasm[i - 1], sizeof(prev_disasm[i]) - 1);
                prev_disasm[i][sizeof(prev_disasm[i]) - 1] = '\0';
            }
            strncpy(prev_disasm[0], disa, sizeof(prev_disasm[0]) - 1);
            prev_disasm[0][sizeof(prev_disasm[0]) - 1] = '\0';
        }
        printf_atc_buffer(0, 0, 0x01, editor_buffer, "PC:$%04X SP:$%02X A:$%02X X:$%02X Y:$%02X", state.program_counter, state.stack_pointer, state.accumulator, state.x_register, state.y_register);
        printf_atc_buffer(0, 1, 0x01, editor_buffer, "Flags: N=%d V=%d D=%d I=%d Z=%d C=%d", state.negative_flag, state.overflow_flag, state.decimal_mode, state.interrupt_disable, state.zero_flag, state.carry_flag);
        printf_atc_buffer(0, 3, 0x01, editor_buffer, "Instr cache:");
        printf_atc_buffer(0, 28, 0x01, border_buffer, "F5-Step  F6-Run F7-Breakpoint F9-Quit");
        for (int i = 9; i >= 0; i--) {
            if (prev_disasm[i][0] != '\0') {
                printf_atc_buffer(0, 4 + (9 - i), 0x01, editor_buffer, "%s    ", prev_disasm[i]);
            }
        }
        //display last 10 elements of the stack
        printf_atc_buffer(0, 15, 0x01, editor_buffer, "Stack (top 10):");
        for (int i = 0; i < 10; i++) {
            int16_t sp = state.stack_pointer + i;
            uint8_t value = read_memory(&state, STACK_BASE + sp);
            printf_atc_buffer(0, 16 + i, 0x01, editor_buffer, "$%02X: $%02X", sp, value);
        }
        // set_cursor_position(6 + crx, 3 + cry);
        render_editor();
        render_frame();
        printf("\033[0m\n");
        printf("last code:0x%02X\n", code);
        frame_end();
    }
    use_utf8 = true;
    debug_mode = false;
    init_editor();
    freerun_mode = true;
    state.halt_flag = false;
}

void send_screen_packet(bool blink) {
    C64ScreenPacket pkt;
    memcpy(pkt.magic, (uint8_t[]){FRAME_MAGIC_0, FRAME_MAGIC_1, FRAME_MAGIC_2, FRAME_MAGIC_3}, 4);
    memcpy(pkt.screen, screen, C64_SCREEN_SIZE);
    memcpy(pkt.color, color_data, C64_SCREEN_SIZE);
    memcpy(pkt.background, background, C64_SCREEN_SIZE);
    pkt.cursor_x = memory[CURSOR_X_ADDRESS] + BORDER_SIDE;
    pkt.cursor_y = memory[CURSOR_Y_ADDRESS] + BORDER_TOP;
    pkt.cursor_on = (memory[CURSOR_ENABLE_ADDRESS] == 0 && blink) ? 0 : 1;  
    pkt._pad = 0;  
    tud_cdc_write((const uint8_t *)&pkt, sizeof(pkt));
}

int main()
{
   
    vreg_set_voltage(CPU_CORE_VOLTAGE);
    sleep_ms(50);  
    set_sys_clock_khz(CPU_CLOCK_KHZ, true);
    sleep_ms(50);  
    stdio_init_all();
    sleep_ms(1000);  

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    uint8_t counter = 0;

    int sox = BORDER_SIDE;
    int soy = BORDER_TOP;
    int increment = 1;  
    uint32_t last_irq_us = time_us_32();  
    uint8_t last_key_pressed = 0;
    border_buffer = create_buffer(TOTAL_W, TOTAL_H, ' ', '@'); 
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
        current_key_handler = inject_key;
        if (state.halt_flag && state.halt_reason != NO_REASON) {
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

            }
        }

        if (debug_mode) {
            freerun_mode = false;
            editor_main_loop();
            continue;
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
        // the cursor blink is handled by the VM timer interrupt
        // blink = memory[CURSOR_BLINK_ADDRESS] && 0x01;
        blink = false;
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
        printf_atc_ascii(0, 28, 0x00, "F9-DEBUG SHIFT+INS=PASTE F12=RESET");
        
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
        #if debug
        printf("PC:$%04X SP:$%02X A:$%02X X:$%02X Y:$%02X\n", state.program_counter, state.stack_pointer, state.accumulator, state.x_register, state.y_register);
        printf("Flags: N=%d V=%d D=%d I=%d Z=%d C=%d\n\n", state.negative_flag, state.overflow_flag, state.decimal_mode, state.interrupt_disable, state.zero_flag, state.carry_flag);
        printf("Disassembly: %s\033[K\n\n", state.disassembly);
        #endif

    #endif
        frame_end();
        //if (time_us_64() - timing > 500000) {  
        //    timing = time_us_64();
        //    blink = !blink; 
        //}
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
        
    uint16_t debug_address = 0x8078;
    bool debug_step = false;

    while (true) {

 
        if (memory[C64_KEY_COUNT] < 2) {  
            char key;
            if (ringbuffer_pop((uint8_t*)&key)) {  
                put_char(key);
            }
        }

        /*
        if (!state.interrupt_disable && state.IRQ_input == true) {
            debug_mode = true;
            state.halt_flag = true;
        }
        */

        while (state.halt_flag) {
            sleep_ms(10);
        }

        counter++;
        execute_opcode(&state, read_memory(&state, state.program_counter));

        if (!freerun_mode) {
            state.halt_flag = true;
        }

        if (last_tick_us + 1000 <= time_us_32()) {
            state.mops = counter - last_counter;
            last_counter = counter;
            last_tick_us = time_us_32();
        }
    }
}