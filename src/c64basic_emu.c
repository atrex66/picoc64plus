
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
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


CPUState state = {0};

void core1_entry();

#define C64_FRAME_US 33333 // ~30 Hz frame time in microseconds

// extern uint8_t read_memory(CPUState *state, uint16_t address);

static uint8_t to_bcd(uint8_t value) {
    return ((value / 10) << 4) | (value % 10);
}

static void inject_key(uint8_t petscii) {
    if (petscii == 0) return;
    if (memory[C64_KEY_COUNT] < memory[C64_KEY_BUF_MAX]) {
        memory[C64_KEY_BUFFER + memory[C64_KEY_COUNT]] = petscii;
        memory[C64_KEY_COUNT]++;
    }
}

void print_bin(unsigned int v) {
    for (int i = sizeof(v)*8 - 1; i >= 0; i--) {
        putchar((v & (1u << i)) ? '1' : '0');
    }
}

// ─────────────────────────────────────────────────────────────────────────────

uint8_t hex_to_byte(const char* hex) {
    uint8_t value = 0;
    for (int i = 0; i < 2; i++) {
        char c = hex[i];
        uint8_t digit = 0;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        }
        value = (value << 4) | digit;
    }
    return value;
}

void tud_cdc_rx_cb(uint8_t itf) {

    if (itf != 0) return;  // only handle interface 0
   
    char buf[32];
    uint32_t count = tud_cdc_read(buf, sizeof(buf));

     if (buf[0] == 0x1B && buf[1] == '[')   // ESC [ sequence = cursor key
    {  // ESC sequence
        char c = buf[2];
        if (c == 'A') 
        {  // up arrow
            inject_key(0x91);
        } else if (c == 'B') 
        {  // down arrow
            inject_key(0x11);
        } else if (c == 'C') 
        {  // right arrow
            inject_key(0x1D);
        } else if (c == 'D') 
        {  // left arrow
            inject_key(0x9D);
        } else if (c == 'H') 
        {  // home key
            inject_key(0x33);
        } else if (c == 'F') 
        {  // end key
            inject_key(0x03);
        } else if (c == 0x33)
        {  // del key
            inject_key(0x14);  
        }
    } else {
        for (uint32_t i = 0; i < count; i++) {
            inject_key(ascii_to_petscii(buf[i]));
            sleep_ms(1);  // small delay to avoid overwhelming the buffer
            while (memory[C64_KEY_COUNT] >= memory[C64_KEY_BUF_MAX] - 1) 
            {
                sleep_ms(20);  // wait for buffer to have space
            }
        }
        uint8_t b;
        while (tud_cdc_available()) 
        {
            uint32_t count = tud_cdc_read(&b, 1);
            inject_key(ascii_to_petscii(b));
            sleep_ms(1);  // small delay to avoid overwhelming the buffer
            while (memory[C64_KEY_COUNT] >= memory[C64_KEY_BUF_MAX] - 1) 
            {
                sleep_ms(20);  // wait for buffer to have space
            }
        }
    }
}

/*
void send_screen_packet(bool blink) {
    C64ScreenPacket pkt;
    pkt.magic[0]=0xC6; pkt.magic[1]=0x40; pkt.magic[2]=0xDE; pkt.magic[3]=0xAD;

    memcpy(pkt.screen, screen, (TOTAL_W * TOTAL_H));
    memcpy(pkt.color,  color_data,   (TOTAL_W * TOTAL_H));
    memcpy(pkt.background, background, (TOTAL_W * TOTAL_H));

    // Cursor in full-frame coords so the host needs no border offset.
    pkt.cursor_x   = memory[CURSOR_X_ADDRESS] + BORDER_SIDE;
    pkt.cursor_y   = memory[CURSOR_Y_ADDRESS] + BORDER_TOP;
    pkt.cursor_on  = blink;
    pkt._pad       = 0;
    fwrite(&pkt, sizeof(pkt), 1, stdout);
    fflush(stdout);
}
*/

/*
void init_littlefs(lfs_t lfs) {
    lfs_cfg = pico_lfs_init(FLASH_OFFSET, FS_SIZE);
    if (!lfs_cfg)
        panic("out of memory");


    int err = lfs_mount(&lfs, lfs_cfg);
    if (err != LFS_ERR_OK) {

        err = lfs_format(&lfs, lfs_cfg);
        if (err != LFS_ERR_OK)
        panic("failed to format filesystem");
        err = lfs_mount(&lfs, lfs_cfg);
        if (err != LFS_ERR_OK)
        panic("failed to mount new filesystem");
    }
}
*/

int main()
{
    uint8_t counter = 0;
    uint64_t timer=0;
    int sox = BORDER_SIDE;
    int soy = BORDER_TOP;
    bool blink = true;  // cursor blink state
    int increment = 1;  // sprite movement increment
    uint32_t last_irq_us = time_us_32();  // CIA1 60 Hz IRQ timer
    uint8_t last_key_pressed = 0;
    Buffer *border_buffer = create_buffer(TOTAL_W, TOTAL_H, ' ', '@'); // full frame buffer including border
    SpriteAsset_t sprites[SPRITE_LEN] = {0};  // Array to hold sprite data
    memset(sprites, 0, sizeof(sprites));  // Initialize sprite data to zero

    // Raise voltage before raising clock, lower clock before lowering voltage
    vreg_set_voltage(CPU_CORE_VOLTAGE);
    sleep_ms(50);  // let regulator settle
    set_sys_clock_khz(CPU_CLOCK_KHZ, true);
   
    stdio_init_all();

    sleep_ms(1000);  // wait for USB serial to be ready

    // ── I2C init ──────────────────────────────────────────────────────────────
    i2c_init(I2C_BUS, 100 * 1000);  // 100 kHz default
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
     init_terminal(TOTAL_W, TOTAL_H);
    clear_terminal();
    init_big_characters('a', ' ', 31);
    set_framerate(25); // Set the desired framerate to 30 FPS    
    set_palette(palette256, sizeof(palette256) / sizeof(palette256[0]));
    
    multicore_launch_core1(core1_entry);  // start 6502 VM on core 1
  
    while (true) 
    {

        frame_start();
        // tud_task();  // TinyUSB device task
        memcpy(sprites, &memory[SPRITE_START_ADDRESS], sizeof(sprites));
        state.dirty_sprite = false;  // Reset dirty flag

        hide_terminal_cursor();
        fill_background_buffer(border_buffer, memory[BORDER_COLOR_ADDRESS]); // Fill border buffer with default background color
        copy_to_screen(border_buffer, 0, 0); // Copy border buffer to screen
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
        render_frame();
    #endif
        frame_end();
        if (time_us_64() - timer > 500000) {  // Toggle blink every 500 ms
            timer = time_us_64();
            blink = !blink; // Toggle blink state
        }
    }
}

void core1_entry() {
 
    // multicore_lockout_victim_init();

    uint32_t counter = 0;
    uint32_t mops = 0;

    uint32_t dirty_gpio_state     = 0;
    uint32_t dirty_gpio_direction  = 0;
    uint32_t dirty_gpio_pullup     = 0;

    // PWM shadow state (one entry per slice)
    uint16_t pwm_wrap[12]    = {[0 ... 11] = 0xFFFF};
    uint16_t pwm_level_a[12] = {0};
    uint16_t pwm_level_b[12] = {0};
    uint16_t pwm_enabled_old = 0;

    // I2C shadow
    uint8_t i2c_ctrl_old  = 0;
    uint8_t i2c_speed_old = 0xFF;  // force first-run init

    uint8_t dma_ctrl_old = 0;
    uint32_t time_offset = 0;

    uint32_t last_counter = 0;

    // DMA channel claim
    dma_channel_claim(DMA_CHANNEL);
    dma_channel_config cfg = dma_channel_get_default_config(DMA_CHANNEL);
  
    // Default DMA increment state: both read and write increment (copy mode).
    // memory[] is zero-initialised in C, so without this they default to 0
    // (fixed-address mode) until the user calls DMAINCR explicitly.
    memory[0xD06C] = 1;  // DMA_READ_INC  default = 1
    memory[0xD06D] = 1;  // DMA_WRITE_INC default = 1

    memory[0xD100] = 0;  // File control register, default = 0 (no file open)

    reset_cpu(&state);

    gpio_init(23);
    gpio_set_dir(23, false);
    gpio_pull_up(23);

    uint32_t last_tick_us = time_us_32();  // CIA1 50 Hz IRQ timer


    while (true) {

        //uint16_t fifty_hz = (state.cia_write.timer_a_control & 0x80) >> 7 ? 20000 : 16667; // 50 / 60 Hz based on bit 7 of Timer A control register

        //if (last_tick_us != time_us_64()) {  // ~50 Hz
        //  last_tick_us = time_us_64();
        //  cia_tick(&state);  // tick the CIA timers and TOD clock
        //}

        if (gpio_get(23) == 0) 
        {
            //reset_cpu(&state);
            reset_cpu(&state);
        }
        // emulate vic2 raster line counter increment
        if ((counter++ & 0xff) == 0xff){
            memory[0xD012]++;
        }
        if (dirty_gpio_state != state.gpio_state)
        {
            gpio_put_masked(state.gpio_direction, state.gpio_state);  // Update GPIO pins based on the state register
        }

        if (dirty_gpio_direction != state.gpio_direction)
        {
            //gpio_init_mask(state.gpio_direction);  // Initialize GPIO pins based on the direction register
            //gpio_set_dir_masked(state.gpio_direction, state.gpio_direction);  // Set GPIO pin directions based on the direction register

            // set GPIO pin directions based on the direction register
            for (int i = 0; i < 32; i++) 
            {
                // gpio_init must be called before gpio_set_dir to place the
                // pin in SIO mode; skip pins that stay as inputs to avoid
                // disturbing unrelated peripherals.
                if ((state.gpio_direction >> i) & 1 && i != 23) {
                    gpio_init(i);
                }
                if (i != 23)  // skip pin 23 to avoid disturbing the RUN/STOP key
                {
                gpio_set_dir(i, (state.gpio_direction >> i) & 1);
                }
            }
        }

        if (dirty_gpio_pullup != state.gpio_pullup)
        {
            // set GPIO pin pull-up resistors based on the pull-up register
            for (int i = 0; i < 32; i++) 
            {
                if (i != 23)  // skip pin 23 to avoid disturbing the RUN/STOP key
                {
                    if ((state.gpio_pullup >> i) & 1u) 
                    {
                        gpio_pull_up(i);
                    } 
                    else 
                    {
                        gpio_disable_pulls(i);
                    }
                }
            }
        }
        execute_opcode(&state, read_memory(&state, state.program_counter));
        dirty_gpio_state      = state.gpio_state;
        dirty_gpio_direction  = state.gpio_direction;
        dirty_gpio_pullup     = state.gpio_pullup;

        state.gpio_state     = memory[GPIO_STATE_ADDRESS] | (memory[GPIO_STATE_ADDRESS + 1] << 8) | (memory[GPIO_STATE_ADDRESS     + 2] << 16) | (memory[GPIO_STATE_ADDRESS     + 3] << 24);
        state.gpio_direction = memory[GPIO_DIRECTION_ADDRESS] | (memory[GPIO_DIRECTION_ADDRESS + 1] << 8) | (memory[GPIO_DIRECTION_ADDRESS + 2] << 16) | (memory[GPIO_DIRECTION_ADDRESS + 3] << 24);
        state.gpio_pullup    = memory[GPIO_PULLUP_ADDRESS] | (memory[GPIO_PULLUP_ADDRESS + 1] << 8) | (memory[GPIO_PULLUP_ADDRESS + 2] << 16) | (memory[GPIO_PULLUP_ADDRESS + 3] << 24);
        
        // ── PWM update ───────────────────────────────────────────────────────
        {
            uint8_t  sel        = memory[PWM_SELECT_ADDR] & 0x0F;  // slice 0-11
            uint16_t new_enable = memory[PWM_ENABLE_ADDR];
            uint16_t new_wrap   = memory[PWM_WRAP_LO_ADDR]   | (memory[PWM_WRAP_HI_ADDR]   << 8);
            uint16_t new_lev_a  = memory[PWM_LEVEL_A_LO_ADDR]| (memory[PWM_LEVEL_A_HI_ADDR]<< 8);
            uint16_t new_lev_b  = memory[PWM_LEVEL_B_LO_ADDR]| (memory[PWM_LEVEL_B_HI_ADDR]<< 8);

            bool changed = (new_wrap   != pwm_wrap[sel]    ||
                            new_lev_a  != pwm_level_a[sel] ||
                            new_lev_b  != pwm_level_b[sel] ||
                            new_enable != pwm_enabled_old);

            if (changed) {
                pwm_wrap[sel]    = new_wrap;
                pwm_level_a[sel] = new_lev_a;
                pwm_level_b[sel] = new_lev_b;
                pwm_enabled_old  = new_enable;

                // Assign GPIOs for this slice to PWM function
                gpio_set_function(sel * 2,     GPIO_FUNC_PWM);
                gpio_set_function(sel * 2 + 1, GPIO_FUNC_PWM);

                pwm_config cfg = pwm_get_default_config();
                pwm_config_set_wrap(&cfg, new_wrap ? new_wrap : 0xFFFF);
                pwm_init(sel, &cfg, false);
                pwm_set_both_levels(sel, new_lev_a, new_lev_b);
                pwm_set_enabled(sel, (new_enable >> sel) & 1);
            }
        }

        // ── I2C update ───────────────────────────────────────────────────────
        {
            uint8_t speed = memory[I2C_SPEED_ADDR];
            if (speed != i2c_speed_old) {
                i2c_speed_old = speed;
                i2c_set_baudrate(I2C_BUS, speed ? 400000 : 100000);
            }

            uint8_t ctrl = memory[I2C_CTRL_ADDR];
            if ((ctrl & 0x03) && !(ctrl & 0x80)) {  // new transaction, not already busy
                memory[I2C_CTRL_ADDR] = 0x80;       // mark busy
                uint8_t addr = memory[I2C_ADDR_ADDR];
                uint8_t len  = memory[I2C_LEN_ADDR];
                if (len == 0 || len > 8) len = 1;

                int result;
                if (ctrl & 0x01) {  // write
                    result = i2c_write_blocking(I2C_BUS, addr, &memory[I2C_DATA_ADDR], len, false);
                } else {            // read
                    result = i2c_read_blocking(I2C_BUS, addr, &memory[I2C_DATA_ADDR], len, false);
                }
                // 0x00=OK, 0x40=NACK/error
                memory[I2C_CTRL_ADDR] = (result == PICO_ERROR_GENERIC) ? 0x40 : 0x00;
            }
        }

        // ── DMA update ───────────────────────────────────────────────────────
        {
            uint8_t ctrl = memory[DMA_CTRL_ADDR];

            // Update busy flag from actual DMA hardware
            if (memory[DMA_CTRL_ADDR] & 0x80) {
                if (!dma_channel_is_busy(DMA_CHANNEL)) {
                    memory[DMA_CTRL_ADDR] = 0x00;  // transfer complete
                }
            }

            // Abort request
            if ((ctrl & 0x02) && (ctrl & 0x80)) {
                dma_channel_abort(DMA_CHANNEL);
                memory[DMA_CTRL_ADDR] = 0x00;
            }

            // Start request — only if not already busy
            if ((ctrl & 0x01) && !(ctrl & 0x80)) {
                uint16_t src = memory[DMA_READ_ADDR_ADDR]
                    | ((uint16_t)memory[DMA_READ_ADDR_ADDR + 1] << 8);

                uint16_t dst = memory[DMA_WRITE_ADDR_ADDR]
                    | ((uint16_t)memory[DMA_WRITE_ADDR_ADDR + 1] << 8);
                // 6502 címek -> memory[] pointerek
                uint8_t *src_ptr = &memory[src];
                uint8_t *dst_ptr = &memory[dst];

                uint16_t count = memory[DMA_COUNT_ADDR]
                    | ((uint16_t)memory[DMA_COUNT_ADDR + 1] << 8);

                uint8_t size = memory[DMA_SIZE_ADDR];
                channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
                channel_config_set_read_increment(&cfg, memory[DMA_READ_INC_ADDR] ? true : false);
                channel_config_set_write_increment(&cfg, memory[DMA_WRITE_INC_ADDR] ? true : false);
                channel_config_set_dreq(&cfg, DREQ_FORCE);
                channel_config_set_irq_quiet(&cfg, true);

                // Biztonsági ellenőrzés
                if ((src + count) <= sizeof(memory) &&
                    (dst + count) <= sizeof(memory)) 
                {
                    dma_channel_configure(
                        DMA_CHANNEL,
                        &cfg,
                        dst_ptr,
                        src_ptr,
                        count,
                        true
                    );
                }
            }
            if (dma_channel_is_busy(DMA_CHANNEL)) {
                memory[DMA_CTRL_ADDR] |= 0x80;  // busy
            } else {
                memory[DMA_CTRL_ADDR] &= ~0x80; // not busy
            }
        }
 
        if (last_tick_us + 1000 <= time_us_32()) {
            state.mops = counter - last_counter;
            last_counter = counter;
            last_tick_us = time_us_32();
        }
    }
}