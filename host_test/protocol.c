#include "protocol.h"

/* Compute CRC-16-CCITT checksum (polynomial 0x1021, initial value 0xFFFF) */
uint16_t calc_crc16(const uint8_t data[], size_t length)
{
    uint16_t crc = 0xFFFF;
    if (data == NULL)
        return 0;
    for (size_t i = 0; i < length; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

void build_packet(TelemetryPacket *pkt, uint32_t ts, int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy, int16_t gz)
{
    if (pkt == NULL)
        return;
    pkt->header = PACKET_HEADER;
    pkt->timestamp_ms = ts;
    pkt->accel_x = ax;
    pkt->accel_y = ay;
    pkt->accel_z = az;
    pkt->gyro_x = gx;
    pkt->gyro_y = gy;
    pkt->gyro_z = gz;

    /* Calculate CRC over all bytes except the final 2-byte crc field */
    size_t payload_size = sizeof(TelemetryPacket) - sizeof(uint16_t);
    pkt->crc = calc_crc16((const uint8_t *)pkt, payload_size);
}

bool verify_packet(const TelemetryPacket *pkt)
{
    if (pkt == NULL)
        return false;
    if (pkt->header != PACKET_HEADER)
        return false;

    size_t payload_size = sizeof(TelemetryPacket) - sizeof(uint16_t);
    uint16_t expected_crc = calc_crc16((const uint8_t *)pkt, payload_size);
    return (pkt->crc == expected_crc);
}