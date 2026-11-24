#include <stdio.h>
#include <stdint.h>
#include "protocol.h"

int main(void)
{
       /* 1. Verify raw CRC-16 calculation against standard test vector */
       const uint8_t test_data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
       uint16_t crc_original = calc_crc16(test_data, sizeof(test_data));
       printf("CRC result: 0x%04X (expected: 0x29B1) -> %s\n",
              crc_original, (crc_original == 0x29B1) ? "PASS" : "FAIL");

       /* 2. Build a telemetry packet and verify its byte size and integrity */
       TelemetryPacket pkt;
       build_packet(&pkt, 1500, 120, -45, 16384, 10, 20, -5);

       printf("Packet size: %zu bytes (expected: 20) -> %s\n",
              sizeof(TelemetryPacket), (sizeof(TelemetryPacket) == 20) ? "PASS" : "FAIL");
       printf("Valid packet check: %s\n",
              verify_packet(&pkt) ? "PASS" : "FAIL");

       /* 3. Flip a single bit in accelerometer X-axis and verify error detection */
       pkt.accel_x ^= 0x0001;
       printf("Corrupted payload check: %s\n",
              !verify_packet(&pkt) ? "PASS (error detected)" : "FAIL");

       return 0;
}