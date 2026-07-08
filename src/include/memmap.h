#ifndef MEMMAP_H
#define MEMMAP_H

#define turbo_mode 0
#define use_host 0

// ── Overclocking settings ────────────────────────────────────────────────────
// Adjust these two defines to tune performance / stability.
// RP2350 default: 150 MHz @ VREG_VOLTAGE_1_10
// Tested stable:  300 MHz @ VREG_VOLTAGE_1_20  (flash speed irrelevant: copy_to_ram)
#if turbo_mode
    #define CPU_CLOCK_KHZ   300000              // target system clock in kHz
    #define CPU_CORE_VOLTAGE VREG_VOLTAGE_1_30  // core voltage (see hardware/vreg.h)
#else
    #define CPU_CLOCK_KHZ   150000              // target system clock in kHz
    #define CPU_CORE_VOLTAGE VREG_VOLTAGE_1_10  // core voltage (see hardware/vreg.h)
#endif
// ─────────────────────────────────────────────────────────────────────────────

// ── C64 memory-mapped addresses ───────────────────────────────────────────────
#define KEY_BUFFER               0x0277
#define KEY_COUNT                0x00C6
#define KEY_BUF_MAX              10

#define CURSOR_X_ADDRESS         0xD3
#define CURSOR_Y_ADDRESS         0xD6
#define CURSOR_ENABLE_ADDRESS    0xCC

#define RASTER_LINE              0xD012
#define FRAME_READY_ADDRESS      0xD013

#define TEXT_SCREEN_START        0x0400
#define COLOR_RAM_START          0xD800

#define BACKGROUND_COLOR_ADDRESS 0xD021
#define BORDER_COLOR_ADDRESS     0xD020

#define TOD_10THS_ADDRESS        0xDC04
#define TOD_SECONDS_ADDRESS      0xDC05
#define TOD_MINUTES_ADDRESS      0xDC06
#define TOD_HOURS_ADDRESS        0xDC07

// ── Terminal screen layout ────────────────────────────────────────────────────
#define BORDER_SIDE    6           // chars of border on each horizontal side
#define BORDER_TOP     2           // rows of border on top and bottom
#define C64_COLS       40
#define C64_ROWS       25
#define TOTAL_W        (C64_COLS + BORDER_SIDE * 2)
#define TOTAL_H        (C64_ROWS + BORDER_TOP  * 2)

// C64 keyboard buffer ($0277-$0280, max 10 chars) and count ($C6)
#define C64_KEY_BUFFER  0x0277
#define C64_KEY_COUNT   0x00C6
#define C64_KEY_BUF_MAX 0x0289


// ascii sprite from "█", "▄", "▀" this characters are used to draw the sprite in the terminal
typedef struct __attribute__((packed))
{
    uint8_t width;                 // $00
    uint8_t height;                // $01
    uint8_t enabled;               // $02
    int16_t x;                     // $03
    int16_t y;                     // $05
    uint8_t transparency_color;    // $07
    uint16_t pixel_color;          // $08
} SpriteAsset_t;


// new sprites located on shadow ram
#define SPRITE_START_ADDRESS 0xA000
#define SPRITE_LEN 32
#define SPRITE_END_ADDRESS (SPRITE_START_ADDRESS + SPRITE_LEN * sizeof(SpriteAsset_t)) // 8 sprites, each 64 bytes

//
// GPIO registers – each is a 32-bit little-endian value spread over 4 bytes.
// To control GPIO N: byte offset = N/8, bit within that byte = N%8 (value = 1<<(N%8))
//
// ── GPIO25 (onboard LED on Pico) ─────────────────────────────────────────────
// GPIO25 → bit 25 → byte offset 3, bit 1 within that byte → write value 2 (=1<<1)
//
//   POKE 53302, 2   : set GPIO25 as output  (direction register, byte 3)
//   POKE 53298, 2   : GPIO25 HIGH → LED ON  (state register,     byte 3)
//   POKE 53298, 0   : GPIO25 LOW  → LED OFF
//
// C64 BASIC one-liner:
//   POKE 53302,2 : POKE 53298,2      (LED on)
//   POKE 53298,0                     (LED off)
// ─────────────────────────────────────────────────────────────────────────────
#define GPIO_STATE_ADDRESS     0xD02F  // 4 bytes LE: GPIO output state   (dec 53295)
#define GPIO_DIRECTION_ADDRESS 0xD033  // 4 bytes LE: GPIO direction 1=out (dec 53299)
#define GPIO_PULLUP_ADDRESS    0xD037  // 4 bytes LE: GPIO pull-up enable  (dec 53303)

// ── I2C memory-mapped registers ($D050-$D05B) ────────────────────────────────
// Uses I2C0, SDA=GPIO4, SCL=GPIO5 by default.
//
//  $D050  I2C_ADDR   - 7-bit target device address
//  $D051  I2C_DATA   - data buffer, 8 bytes ($D051-$D058)
//  $D059  I2C_LEN    - number of bytes to transfer (1-8)
//  $D05A  I2C_CTRL   - write: 0x01=start write, 0x02=start read
//                      read:  0x80=busy, 0x40=NACK/error, 0x00=OK
//  $D05B  I2C_SPEED  - 0=100 kHz (standard), 1=400 kHz (fast)
#define I2C_ADDR_ADDR   0xD050
#define I2C_DATA_ADDR   0xD051  // 8-byte buffer: $D051..$D058
#define I2C_LEN_ADDR    0xD059
#define I2C_CTRL_ADDR   0xD05A
#define I2C_SPEED_ADDR  0xD05B

#define I2C_SDA_PIN  4
#define I2C_SCL_PIN  5
#define I2C_BUS      i2c0

// ── DMA memory-mapped registers ($D060-$D06D) ────────────────────────────────
// RP2350 has 16 DMA channels. One channel is reserved here for 6502 use.
// The transfer runs entirely on the ARM DMA controller; the 6502 is free
// to continue executing while the transfer is in progress.
//
//  $D060         DMA_CTRL       write: $01=start, $02=abort
//                               read:  $80=busy, $40=error, $00=idle
//  $D061         DMA_SIZE       0=byte, 1=halfword (2B), 2=word (4B)
//  $D062-$D065   DMA_READ_ADDR  source address, 16bit address space of the 6502
//  $D066-$D069   DMA_WRITE_ADDR destination address, 16bit address space of the 6502
//  $D06A-$D06B   DMA_COUNT      number of transfers (1-65535), little-endian
//  $D06C         DMA_READ_INC   1 = increment source address after each transfer
//  $D06D         DMA_WRITE_INC  1 = increment destination address after each transfer
//
// Addresses refer to the 6502 memory map.
// The 6502 memory array starts at: (uintptr_t)memory  (in SRAM)
// Convenience: set DMA_READ_ADDR / DMA_WRITE_ADDR to
//   (uintptr_t)memory + 0xC000  to copy from $C000 in the 6502 address space.

#define DMA_CTRL_ADDR       0xD060
#define DMA_SIZE_ADDR       0xD061
#define DMA_READ_ADDR_ADDR  0xD062  // 4 bytes, LE
#define DMA_WRITE_ADDR_ADDR 0xD066  // 4 bytes, LE
#define DMA_COUNT_ADDR      0xD06A  // 2 bytes, LE
#define DMA_READ_INC_ADDR   0xD06C
#define DMA_WRITE_INC_ADDR  0xD06D

#define DMA_CHANNEL 11  // RP2350 DMA channel reserved for 6502 use

#define TIMER_ZERO_ADDRESS 0xD06E  // 1 bytes, zeroing the timer
#define US_TIMER_ADDRESS 0xD06F  // 4 bytes, LE, microsecond timer (wraps around every ~71 minutes)

// littlefs handler saves all files in .prg format so the first two bytes are the load address (little-endian) and the rest is the file data
#define FILE_CONTROL_COMMAND 0xD100
// 0x00 = no command, 0x01 = read, 0x02 = write, 0x03 = delete, 0x04 = list, 0x05 = rename, 0x06 = create directory, 0x07 = change directory, 0x08 = get current directory
#define FILE_CONTROL_STATUS 0xD101
// 0x00 = idle, 0x01 = busy, 0x02 = success, 0x03 = error, 0x04 = file not found, 0x05 = directory not found, 0x06 = permission denied, 0x07 = invalid command, 0x08 = invalid filename, 0x09 = file already exists, 0x0A = directory already exists
#define FILE_START_ADDRESS 0xD102
#define FILE_LENGTH_ADDRESS 0xD104  // only need for saving files, not for reading
#define FILENAME_MAX_LEN 32
#define FILENAME_ADDRESS 0xD106
// save operation: write the filename to FILENAME_ADDRESS, write the starting address to FILE_START_ADDRESS, write the length to FILE_LENGTH_ADDRESS set FILE_CONTROL_COMMAND to 0x02 (write), wait for FILE_CONTROL_STATUS to become 0x02 (success) or 0x03 (error)
// load operation: write the filename to FILENAME_ADDRESS, set FILE_CONTROL_COMMAND to 0x01 (read), wait for FILE_CONTROL_STATUS to become 0x02 (success) or 0x03 (error), read the starting address from FILE_START_ADDRESS, read the length from FILE_LENGTH_ADDRESS
// delete operation: write the filename to FILENAME_ADDRESS, set FILE_CONTROL_COMMAND to 0x03 (delete), wait for FILE_CONTROL_STATUS to become 0x02 (success) or 0x03 (error)
// list operation: set FILE_CONTROL_COMMAND to 0x04 (list), wait for FILE_CONTROL_STATUS to become 0x02 (success) or 0x03 (error), read the starting address from FILE_START_ADDRESS, read the length from FILE_LENGTH_ADDRESS, the file list is a null-terminated string of filenames separated by newlines
// rename operation: write the old filename to FILENAME_ADDRESS, set the FILE_START_ADDRESS to the new filename, set FILE_CONTROL_COMMAND to 0x05 (rename), wait for FILE_CONTROL_STATUS to become 0x02 (success) or 0x03 (error)
// create directory operation: write the directory name to FILENAME_ADDRESS, set FILE_CONTROL_COMMAND to 0x06 (create directory), wait for FILE_CONTROL_STATUS to become 0x02 (success) or 0x03 (error)
// change directory operation: write the directory name to FILENAME_ADDRESS, set FILE_CONTROL_COMMAND to 0x07 (change directory), wait for FILE_CONTROL_STATUS to become 0x02 (success) or 0x03 (error)
// get current directory operation: set FILE_CONTROL_COMMAND to 0x08 (get current directory), wait for FILE_CONTROL_STATUS to become 0x02 (success) or 0x03 (error), read the starting address from FILE_START_ADDRESS, read the length from FILE_LENGTH_ADDRESS, the current directory is a null-terminated string
// The file system is case-insensitive, and the maximum path length is 256 characters. The root directory is "/". Directories are separated by "/" (forward slash). The current directory can be changed with the "CHDIR" command. The current directory can be obtained with the "pwd" command. The file system supports relative paths (e.g., "../file.txt") and absolute paths (e.g., "/dir/file.txt"). The file system supports wildcards in filenames (e.g., "*.txt"). The file system supports hidden files and directories (names starting with "."). The file system supports read-only files and directories (names starting with "_"). The file system supports symbolic links (names starting with "@"). The file system supports special files (names starting with "#"). The file system supports device files (names starting with "$"). The file system supports FIFO files (names starting with "%"). The file system supports socket files (names starting with "&"). The file system supports character device files (names starting with "!"). The file system supports block device files (names starting with "^"). The file system supports named pipes (names starting with "~"). The file system supports mount points (names starting with "="). The file system supports virtual files (names starting with "`").

#endif // MEMMAP_H