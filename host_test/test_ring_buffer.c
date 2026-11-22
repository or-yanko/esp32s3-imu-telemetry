#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"
#include "ring_buffer.h"

int main(void)
{
    RingBuffer rb;
    TelemetryPacket in_pkt;
    TelemetryPacket out_pkt;

    /* 1. Verify initial empty state */
    rb_init(&rb);
    bool empty_init = rb_is_empty(&rb) && !rb_is_full(&rb) && !rb_pop(&rb, &out_pkt);
    printf("1. Initial empty state: %s\n", empty_init ? "PASS" : "FAIL");

    /* 2. Fill buffer to exact capacity (64 packets) and check overflow rejection */
    bool push_all_ok = true;
    for (uint32_t i = 0; i < RING_BUFFER_CAPACITY; i++)
    {
        build_packet(&in_pkt, i * 5, (int16_t)i, -10, 16384, 1, 2, 3);
        if (!rb_push(&rb, &in_pkt))
        {
            push_all_ok = false;
        }
    }

    build_packet(&in_pkt, 9999, 0, 0, 0, 0, 0, 0);
    bool overflow_rejected = !rb_push(&rb, &in_pkt);
    bool full_check = push_all_ok && rb_is_full(&rb) && !rb_is_empty(&rb) && overflow_rejected;
    printf("2. Full capacity (64) & overflow rejection: %s\n", full_check ? "PASS" : "FAIL");

    /* 3. Pop 32 packets to advance tail to index 32 and verify FIFO order + CRC */
    bool first_half_ok = true;
    for (uint32_t i = 0; i < 32; i++)
    {
        if (!rb_pop(&rb, &out_pkt) ||
            !verify_packet(&out_pkt) ||
            out_pkt.timestamp_ms != i * 5 ||
            out_pkt.accel_x != (int16_t)i)
        {
            first_half_ok = false;
        }
    }
    printf("3. Pop first 32 packets (FIFO & CRC check): %s\n", first_half_ok ? "PASS" : "FAIL");

    /* 4. Push 32 new packets to force wrap-around past index 63 to 0 */
    bool wrap_push_ok = true;
    for (uint32_t i = 64; i < 96; i++)
    {
        build_packet(&in_pkt, i * 5, (int16_t)i, -10, 16384, 1, 2, 3);
        if (!rb_push(&rb, &in_pkt))
        {
            wrap_push_ok = false;
        }
    }

    /* Drain all remaining 64 packets (indices 32..63 then wrapped 64..95) */
    bool wrap_pop_ok = true;
    for (uint32_t i = 32; i < 96; i++)
    {
        if (!rb_pop(&rb, &out_pkt) ||
            !verify_packet(&out_pkt) ||
            out_pkt.timestamp_ms != i * 5 ||
            out_pkt.accel_x != (int16_t)i)
        {
            wrap_pop_ok = false;
        }
    }

    bool wrap_complete = wrap_push_ok && wrap_pop_ok && rb_is_empty(&rb);
    printf("4. Circular wrap-around & full drain: %s\n", wrap_complete ? "PASS" : "FAIL");

    return 0;
}