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
