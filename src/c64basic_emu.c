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
#include "tusb.h"        // fontos!
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

#define RESET_PIN 23  // GPIO pin for reset button

void core1_entry();

static uint8_t to_bcd(uint8_t value) {
    return ((value / 10) << 4) | (value % 10);
}

static void inject_key(uint8_t petscii) {

    memory[C64_KEY_BUFFER + memory[C64_KEY_COUNT]] = petscii;
    memory[C64_KEY_COUNT]++;
    while(memory[C64_KEY_COUNT] > 4) {
        sleep_ms(10);  // Wait until there's space in the keyboard buffer
    }
}

void print_bin(unsigned int v) {
    for (int i = sizeof(v)*8 - 1; i >= 0; i--) {
        putchar((v & (1u << i)) ? '1' : '0');
    }
}

// ─────────────────────────────────────────────────────────────────────────────
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
        inject_key(ascii_to_petscii(b)); // Inject the character directly if not buffering
        return;
    } else if ((b == '{' || b == 0x1b) && !is_buffering)
    {
        is_buffering = true;
        buffer_index = 0;
    } 
    if (is_buffering)
    {
        cmdbuffer[buffer_index++] = b;
        cmdbuffer[buffer_index+1] = '\0'; // Null-terminate the command string
        if (buffer_index + 1 > sizeof(cmdbuffer) - 1) {
            state.halt_reason = KEYBOARD_BUFFER_OVERFLOW_REASON;  // Set halt reason to indicate buffer overflow
            memcpy(state.disassembly, cmdbuffer, sizeof(cmdbuffer));  // Copy the command that caused the overflow to disassembly
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
                // hexadecimal input, convert to PETSCII and inject
                uint8_t hex_value = (uint8_t)strtol(&cmdbuffer[2], NULL, 16);
                inject_key(hex_value);
            }
            is_buffering = false;  // End buffering on closing brace or newline
            buffer_index = 0;  // Reset buffer index
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
    return true;  // Keep the timer repeating
}

int main()
{

    // Raise voltage before raising clock, lower clock before lowering voltage
    vreg_set_voltage(CPU_CORE_VOLTAGE);
    sleep_ms(50);  // let regulator settle
    set_sys_clock_khz(CPU_CLOCK_KHZ, true);
    sleep_ms(50);  // let regulator settle
    stdio_init_all();
    sleep_ms(1000);  // wait for USB serial to be ready

    uint8_t counter = 0;
    uint64_t timing=0;
    int sox = BORDER_SIDE;
    int soy = BORDER_TOP;
    bool blink = true;  // cursor blink state
    int increment = 1;  // sprite movement increment
    uint32_t last_irq_us = time_us_32();  // CIA1 60 Hz IRQ timer
    uint8_t last_key_pressed = 0;
    Buffer *border_buffer = create_buffer(TOTAL_W, TOTAL_H, ' ', '@'); // full frame buffer including border
    SpriteAsset_t sprites[SPRITE_LEN] = {0};  // Array to hold sprite data
    memset(sprites, 0, sizeof(sprites));  // Initialize sprite data to zero
    multicore_launch_core1(core1_entry);  // start 6502 VM on core 1
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
    set_framerate(50); // Set the desired framerate to 50 FPS    
    char life=0;
    set_palette(palette256, sizeof(palette256) / sizeof(palette256[0]));
    int previous_fifo_index = -1;  // Initialize previous FIFO index for change detection
    ringbuffer_init();  // Initialize the ring buffer for keyboard input
    while (true) 
    {
        if (state.halt_flag && state.halt_reason != 0) {
            memset(screen, 0x20, FRAME_WIDTH * FRAME_HEIGHT);  // Clear screen to spaces
            memset(color_data, 0x01, FRAME_WIDTH * FRAME_HEIGHT);  // Clear
            memset(background, 0x00, FRAME_WIDTH * FRAME_HEIGHT);  // Clear
            render_frame();
            printf("\033[2j\033[0m");  // reset attributes to default before printing
            printf("\033[31m"); // set text color to red
            printf("\033[0;0H"); // move cursor to top-left corner
            printf("****************************************************\n");
            printf("*                  SOFTWARE ERROR                  *\n");
            printf("*              Press RESET to restart              *\n");
            printf("****************************************************\n");
            printf(" %s \n", halt_reason[state.halt_reason]);
            printf(" %s\n", state.disassembly);
            printf(" CPU halted. Check the disassembly for details. *\n");
            printf(" PC:$%04X, SP:$%02X, A:$%02X, X:$%02X, Y:$%02X\n", state.program_counter, state.stack_pointer, state.accumulator, state.x_register, state.y_register);
            printf(" Flags: N=%d V=%d D=%d I=%d Z=%d C=%d\n", state.negative_flag, state.overflow_flag, state.decimal_mode, state.interrupt_disable, state.zero_flag, state.carry_flag);
            printf("\033[0m");  // reset attributes to default after printing   
            while (gpio_get(23) == 1) {
                reset_cpu(&state);
            }
        }

        while (tud_cdc_available() && ringbuffer_available() > 1) {  // Check if there's data available in the USB CDC buffer and space in the ring buffer
            char c = tud_cdc_read_char();
            ringbuffer_push((uint8_t)c);  // Push the character into the ring buffer
        }

        frame_start();
        memcpy(sprites, &memory[SPRITE_START_ADDRESS], sizeof(sprites));
        state.dirty_sprite = false;  // Reset dirty flag
        hide_terminal_cursor();
        fill_background_buffer(border_buffer, memory[BORDER_COLOR_ADDRESS]); // Fill border buffer with default background color
        copy_to_screen(border_buffer, 0, 0); // Copy border buffer to screen
        printf_atc(0, 0, 0, "%d", state.mops); // Copy text and color data to border buffer
        uint8_t cur_x = memory[CURSOR_X_ADDRESS];     // 0..39
        uint8_t cur_y = memory[CURSOR_Y_ADDRESS];     // 0..24
        uint8_t cur_enable = memory[CURSOR_ENABLE_ADDRESS];  // 0 = cursor on, otherwise off
        set_cursor_position(cur_x + sox, cur_y + soy);
        if (cur_enable == 0) {
            if (blink) {
                show_cursor(); // Show cursor
            } else {
                hide_cursor(); // Hide cursor
            }
        } else {
            hide_cursor(); // Hide cursor
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
        state.frame_ready_flag = true;  // set the frame ready flag in the CPU state
        memory[FRAME_READY_ADDRESS] = 1;  // set the frame ready flag in memory
        render_frame();
        //printf("\n\033[0m");
        //printf("OP/MS: %d\033[K", state.mops);
        //printf("IRQ address: $%04X\033[K\n", memory[0x0314] | (memory[0x0315] << 8));
        //printf("NMI address: $%04X\033[K\n", memory[0x0318] | (memory[0x0319] << 8));
    #endif
        frame_end();
        if (time_us_64() - timing > 500000) {  // Toggle blink every 500 ms
            timing = time_us_64();
            blink = !blink; // Toggle blink state
        }
    }
    return 0;
}

void core1_entry() {

    //mutex_init(&memoryMutex);  // Initialize the mutex for memory access
    uint32_t counter = 0;
    uint32_t mops = 0;

    // I2C shadow
    uint8_t i2c_ctrl_old  = 0;
    uint8_t i2c_speed_old = 0xFF;  // force first-run init

    uint32_t time_offset = 0;
    uint32_t last_counter = 0;

    reset_cpu(&state);

    gpio_init(23);
    gpio_set_dir(23, false);
    gpio_pull_up(23);

    #define I2C_SIZE (I2C_SPEED_ADDR - I2C_CTRL_ADDR + 1)
    memset(&memory[I2C_CTRL_ADDR], 0, I2C_SIZE);

    uint32_t last_tick_us = time_us_32();  // CIA1 50 Hz IRQ timer
    uint8_t last_key_count = 10;  // Last key count for detecting changes in the keyboard buffer
    uint32_t retrace_time_us = 0;  // Time of the last raster line update
    int previous_fifo_index = -1;  // Initialize previous FIFO index for change detection
        
    while (true) {

        counter++;

        if (memory[C64_KEY_COUNT] < 2) {  // Check if there's space in the keyboard buffer
            char key;
            if (ringbuffer_pop((uint8_t*)&key)) {  // Pop a character from the ring buffer
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