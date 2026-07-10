#ifndef RINGBUFFER_H
#define RINGBUFFER_H
#include <stdint.h>
#include <stdbool.h>

#define RING_BUFFER_SIZE 128

void ringbuffer_init();
bool ringbuffer_push(uint8_t value);
bool ringbuffer_pop(uint8_t *out_value);
bool ringbuffer_is_empty();
bool ringbuffer_is_full();
uint8_t ringbuffer_available(void);

#endif // RINGBUFFER_H