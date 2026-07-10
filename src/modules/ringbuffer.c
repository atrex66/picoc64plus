#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ringbuffer.h"

static uint8_t ring_buffer[RING_BUFFER_SIZE];
static volatile uint8_t rb_head = 0;
static volatile uint8_t rb_tail = 0;

void ringbuffer_init(void) {
    rb_head = 0;
    rb_tail = 0;
    memset(ring_buffer, 0, sizeof(ring_buffer));
}

bool ringbuffer_push(uint8_t value) {
    ring_buffer[rb_head] = value;
    uint8_t new_head = (rb_head + 1) % RING_BUFFER_SIZE;
    if (new_head == rb_tail) {
        // Buffer would be full; overwrite oldest by moving tail
        rb_tail = (rb_tail + 1) % RING_BUFFER_SIZE;
    }
    rb_head = new_head;
    return true;  // always succeeds (overwrites old data)
}

bool ringbuffer_pop(uint8_t *out_value) {
    if (rb_head == rb_tail) {
        return false;  // buffer empty
    }
    *out_value = ring_buffer[rb_tail];
    rb_tail = (rb_tail + 1) % RING_BUFFER_SIZE;
    return true;
}

bool ringbuffer_is_empty(void) {
    return rb_head == rb_tail;
}

bool ringbuffer_is_full(void) {
    return ((rb_head + 1) % RING_BUFFER_SIZE) == rb_tail;
}

uint8_t ringbuffer_available(void) {
    uint8_t occupied = (uint8_t)((rb_head - rb_tail + RING_BUFFER_SIZE) % RING_BUFFER_SIZE);
    return (uint8_t)(RING_BUFFER_SIZE - 1 - occupied);
}