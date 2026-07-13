#ifndef MEMMAP_H
#define MEMMAP_H

#define turbo_mode 1
#define use_host 0

#if turbo_mode
    #define CPU_CLOCK_KHZ   300000              
    #define CPU_CORE_VOLTAGE VREG_VOLTAGE_1_30  
#else
    #define CPU_CLOCK_KHZ   150000              
    #define CPU_CORE_VOLTAGE VREG_VOLTAGE_1_10  
#endif

#define KEY_BUFFER               0x0277
#define KEY_COUNT                0x00C6
#define KEY_BUF_MAX              10

#define CURSOR_X_ADDRESS         0xD3
#define CURSOR_Y_ADDRESS         0xD6
#define CURSOR_ENABLE_ADDRESS    0xCC
#define CURSOR_BLINK_ADDRESS     0xCF

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

#define BORDER_SIDE    6           
#define BORDER_TOP     2           
#define C64_COLS       40
#define C64_ROWS       25
#define TOTAL_W        (C64_COLS + BORDER_SIDE * 2)
#define TOTAL_H        (C64_ROWS + BORDER_TOP  * 2)
#define C64_SCREEN_SIZE (TOTAL_W * TOTAL_H)

#define FRAME_MAGIC_0   0xC6
#define FRAME_MAGIC_1   0x40
#define FRAME_MAGIC_2   0xDE
#define FRAME_MAGIC_3   0xAD

#pragma pack(push, 1)
typedef struct {
    uint8_t  magic[4];
    uint8_t  screen[C64_SCREEN_SIZE];   // PETSCII codes
    uint8_t  color[C64_SCREEN_SIZE];    // fg colour index 0-15
    uint8_t  background[C64_SCREEN_SIZE];  // $D021
    uint8_t  cursor_x;
    uint8_t  cursor_y;
    uint8_t  cursor_on;                 // 0 = on (visible), else off
    uint8_t  _pad;
} C64ScreenPacket;
#pragma pack(pop)

#define C64_KEY_BUFFER  0x0277
#define C64_KEY_COUNT   0x00C6
#define C64_KEY_BUF_MAX 0x0289

typedef struct __attribute__((packed))
{
    uint8_t width;                 
    uint8_t height;                
    uint8_t enabled;               
    int16_t x;                     
    int16_t y;                     
    uint8_t transparency_color;    
    uint16_t pixel_color;          
} SpriteAsset_t;

#define SPRITE_START_ADDRESS 0xA000
#define SPRITE_LEN 32
#define SPRITE_END_ADDRESS (SPRITE_START_ADDRESS + SPRITE_LEN * sizeof(SpriteAsset_t)) 

#define GPIO_STATE_ADDRESS     0xD02F  
#define GPIO_DIRECTION_ADDRESS 0xD033  
#define GPIO_PULLUP_ADDRESS    0xD037  

#define I2C_ADDR_ADDR   0xD050
#define I2C_DATA_ADDR   0xD051  
#define I2C_LEN_ADDR    0xD059
#define I2C_CTRL_ADDR   0xD05A
#define I2C_SPEED_ADDR  0xD05B

#define I2C_SDA_PIN  4
#define I2C_SCL_PIN  5
#define I2C_BUS      i2c0

#define DMA_CTRL_ADDR       0xD060
#define DMA_SIZE_ADDR       0xD061
#define DMA_READ_ADDR_ADDR  0xD062  
#define DMA_WRITE_ADDR_ADDR 0xD066  
#define DMA_COUNT_ADDR      0xD06A  
#define DMA_READ_INC_ADDR   0xD06C
#define DMA_WRITE_INC_ADDR  0xD06D

#define DMA_CHANNEL 11  

#define TIMER_ZERO_ADDRESS 0xD06E  
#define US_TIMER_ADDRESS 0xD06F  

#define FILE_CONTROL_COMMAND 0xD100

#define FILE_CONTROL_STATUS 0xD101

#define FILE_START_ADDRESS 0xD102
#define FILE_LENGTH_ADDRESS 0xD104  
#define FILENAME_MAX_LEN 32
#define FILENAME_ADDRESS 0xD106

#endif 