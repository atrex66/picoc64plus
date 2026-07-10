#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stdbool.h>
#include "structs.h"
#include "enums.h"

static const char* halt_reason[] = {
    [0] = "No reason",
    [1] = "Hello fellow programmer!",
    [2] = "UNKNOWN SYSCALL",
    [3] = "HALT instruction",
    [4] = "Illegal opcode or unknown reason",
    [5] = "GPIO_SET_PINMODE",
    [6] = "GPIO_SET_PINOUT",
    [7] = "GPIO_SET_PINPULL",
    [8] = "GPIO_GET_PINSTATE",
    [9] = "ADC_ILLEGAL_CHANNEL",
    [10] = "DMA_CHANNEL_ALREADY_CLAIMED",
    [11] = "KEYBOARD_BUFFER_OVERFLOW"
};

extern void (*arm_opcode_functions[256])(CPUState *state);

void RESET_CPU_SYS(CPUState *state);

void HELLO_FROM_ARM(CPUState *state);

void PWM_SETUP(CPUState *state);
void PWM_SET_FREQUENCY(CPUState *state);
void PWM_SET_DUTY(CPUState *state);
void PWM_CONTROL(CPUState *state);
void PWM_DEINIT(CPUState *state);

void GPIO_SET_PINMODE(CPUState *state);
void GPIO_SET_PINOUT(CPUState *state);
void GPIO_SET_PINPULL(CPUState *state);
void GPIO_READ_PIN(CPUState *state);

int16_t read_onboard_temperature(const char unit);

void ADC_INIT(CPUState *state);
void ADC_READ(CPUState *state);

void GET_IRQ_SOURCE(CPUState *state);
void CLEAR_IRQ_SOURCE(CPUState *state);
void SET_GPIO_IRQ(CPUState *state);
void DISABLE_GPIO_IRQ(CPUState *state);

void DMA_INIT(CPUState *state);
void DMA_SET_SIZE(CPUState *state);
void DMA_INCREMENT(CPUState *state);
void DMA_SET_TRANSFER_COUNT(CPUState *state);
void DMA_SET_SRC(CPUState *state);
void DMA_SET_DST(CPUState *state);
void DMA_START(CPUState *state);
void DMA_WAIT_FOR_COMPLETE(CPUState *state);
void DMA_ABORT(CPUState *state);
void DMA_GET_STATUS(CPUState *state);

void RENDER_WAIT_FRAME(CPUState *state);

#endif 