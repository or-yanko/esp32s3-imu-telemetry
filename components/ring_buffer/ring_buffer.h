#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "protocol.h"

/* Static capacity: 64 packets * 20 bytes = 1,280 bytes in RAM */
#define RING_BUFFER_CAPACITY 64

typedef struct
{

    TelemetryPacket buffer[RING_BUFFER_CAPACITY];
    size_t head;  /* Write index */
    size_t tail;  /* Read index */
    size_t count; /* Current number of stored packets */
} RingBuffer;

/* Reset indices and packet counter to zero */
void rb_init(RingBuffer *rb);

/* Copy a packet into the buffer at head; returns false if full or NULL */
bool rb_push(RingBuffer *rb, const TelemetryPacket *pkt);

/* Copy the oldest packet from tail into out_pkt; returns false if empty or NULL */
bool rb_pop(RingBuffer *rb, TelemetryPacket *out_pkt);

/* Return true if count == 0 */
bool rb_is_empty(const RingBuffer *rb);

/* Return true if count == RING_BUFFER_CAPACITY */
bool rb_is_full(const RingBuffer *rb);

#endif