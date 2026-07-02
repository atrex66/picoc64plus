
#ifndef PICO65_HW_H
#define PICO65_HW_H

/* Simple cc65 hardware support library (C89) */

typedef unsigned char  u8;
typedef unsigned int   u16;
typedef unsigned long  u32;

#define PEEK(addr)      (*(volatile u8*)(addr))
#define POKE(addr,val)  (*(volatile u8*)(addr) = (u8)(val))

/* Register map */
#define GPIO_STATE_ADDRESS      0xD02F
#define GPIO_DIRECTION_ADDRESS  0xD033
#define GPIO_PULLUP_ADDRESS     0xD037

#define PWM_ENABLE_ADDR         0xD040
#define PWM_SELECT_ADDR         0xD041
#define PWM_WRAP_LO_ADDR        0xD042
#define PWM_WRAP_HI_ADDR        0xD043
#define PWM_LEVEL_A_LO_ADDR     0xD044
#define PWM_LEVEL_A_HI_ADDR     0xD045
#define PWM_LEVEL_B_LO_ADDR     0xD046
#define PWM_LEVEL_B_HI_ADDR     0xD047

#define I2C_ADDR_ADDR           0xD050
#define I2C_DATA_ADDR           0xD051
#define I2C_LEN_ADDR            0xD059
#define I2C_CTRL_ADDR           0xD05A
#define I2C_SPEED_ADDR          0xD05B

#define DMA_CTRL_ADDR           0xD060
#define DMA_SIZE_ADDR           0xD061
#define DMA_READ_ADDR_ADDR      0xD062
#define DMA_WRITE_ADDR_ADDR     0xD066
#define DMA_COUNT_ADDR          0xD06A
#define DMA_READ_INC_ADDR       0xD06C
#define DMA_WRITE_INC_ADDR      0xD06D

#define GPIO_INPUT  0
#define GPIO_OUTPUT 1

#define GPIO_LOW    0
#define GPIO_HIGH   1

#define DMA_BYTE    0
#define DMA_HALF    1
#define DMA_WORD    2

static void gpio_mode(u8 gpio, u8 output)
{
    volatile u8* p=(volatile u8*)(GPIO_DIRECTION_ADDRESS+(gpio>>3));
    u8 m=(u8)(1u<<(gpio&7));
    if(output) *p|=m; else *p&=(u8)~m;
}

static void gpio_write(u8 gpio, u8 value)
{
    volatile u8* p=(volatile u8*)(GPIO_STATE_ADDRESS+(gpio>>3));
    u8 m=(u8)(1u<<(gpio&7));
    if(value) *p|=m; else *p&=(u8)~m;
}

static u8 gpio_read(u8 gpio)
{
    volatile u8* p=(volatile u8*)(GPIO_STATE_ADDRESS+(gpio>>3));
    return (u8)((*p>>(gpio&7))&1);
}

static void gpio_pullup(u8 gpio,u8 enable)
{
    volatile u8* p=(volatile u8*)(GPIO_PULLUP_ADDRESS+(gpio>>3));
    u8 m=(u8)(1u<<(gpio&7));
    if(enable) *p|=m; else *p&=(u8)~m;
}

static void pwm_enable(u8 slice,u8 enable)
{
    if(enable) PEEK(PWM_ENABLE_ADDR)|=(1u<<slice);
    else PEEK(PWM_ENABLE_ADDR)&=(u8)~(1u<<slice);
}

static void pwm_select(u8 slice){ POKE(PWM_SELECT_ADDR,slice); }

static void pwm_set_wrap(u16 wrap)
{
    POKE(PWM_WRAP_LO_ADDR,(u8)wrap);
    POKE(PWM_WRAP_HI_ADDR,(u8)(wrap>>8));
}

static void pwm_set_level_a(u16 level)
{
    POKE(PWM_LEVEL_A_LO_ADDR,(u8)level);
    POKE(PWM_LEVEL_A_HI_ADDR,(u8)(level>>8));
}

static void pwm_set_level_b(u16 level)
{
    POKE(PWM_LEVEL_B_LO_ADDR,(u8)level);
    POKE(PWM_LEVEL_B_HI_ADDR,(u8)(level>>8));
}

static u8 dma_busy(void)
{
    return (PEEK(DMA_CTRL_ADDR)&0x80)!=0;
}

static void dma_wait(void)
{
    while(dma_busy()) ;
}

static void dma_abort(void)
{
    POKE(DMA_CTRL_ADDR,0x02);
}

static void dma_setup(u32 src,u32 dst,u16 count,u8 size,u8 rinc,u8 winc)
{
    POKE(DMA_SIZE_ADDR,size);

    POKE(DMA_READ_ADDR_ADDR+0,(u8)src);
    POKE(DMA_READ_ADDR_ADDR+1,(u8)(src>>8));
    POKE(DMA_READ_ADDR_ADDR+2,(u8)(src>>16));
    POKE(DMA_READ_ADDR_ADDR+3,(u8)(src>>24));

    POKE(DMA_WRITE_ADDR_ADDR+0,(u8)dst);
    POKE(DMA_WRITE_ADDR_ADDR+1,(u8)(dst>>8));
    POKE(DMA_WRITE_ADDR_ADDR+2,(u8)(dst>>16));
    POKE(DMA_WRITE_ADDR_ADDR+3,(u8)(dst>>24));

    POKE(DMA_COUNT_ADDR,(u8)count);
    POKE(DMA_COUNT_ADDR+1,(u8)(count>>8));

    POKE(DMA_READ_INC_ADDR,rinc);
    POKE(DMA_WRITE_INC_ADDR,winc);

}

static void dma_start(){
    POKE(DMA_CTRL_ADDR,0x01);
}

static void dma_copy(u32 src,u32 dst,u16 count,u8 size)
{
    dma_setup(src,dst,count,size,1,1);
    dma_start();
    dma_wait();
}

static void dma_copy_noblock(u32 src,u32 dst,u16 count,u8 size)
{
    dma_setup(src,dst,count,size,1,1);
    dma_start();
}

#endif
