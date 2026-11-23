#include <stdio.h>
#include "esp_log.h"
#include "protocol.h"
#include "ring_buffer.h"

static const char *TAG = "TELEMETRY_MAIN";

void app_main(void) {
    RingBuffer rb;
    rb_init(&rb);

    TelemetryPacket pkt;
    build_packet(&pkt, 0, 0, 0, 16384, 0, 0, 0);

    if (rb_push(&rb, &pkt) && verify_packet(&pkt)) {
        ESP_LOGI(TAG, "System initialized. Packet size: %u bytes, CRC: 0x%04X",
                 (unsigned int)sizeof(TelemetryPacket), pkt.crc);
    }
}
