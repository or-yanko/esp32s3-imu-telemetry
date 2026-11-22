#include "ring_buffer.h"
#include <string.h>

void rb_init(RingBuffer *rb)
{
    if (rb == NULL)
    {
        return;
    }
    memset(rb->buffer, 0, sizeof(rb->buffer));
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool rb_is_empty(const RingBuffer *rb)
{
    if (rb == NULL)
    {
        return true;
    }
    return (rb->count == 0);
}

bool rb_is_full(const RingBuffer *rb)
{
    if (rb == NULL)
    {
        return false;
    }
    return (rb->count >= RING_BUFFER_CAPACITY);
}
