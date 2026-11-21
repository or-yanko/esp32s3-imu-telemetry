#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PACKET_HEADER 0xAA55

/* Prevent compiler from inserting padding bytes between struct fields */
typedef struct __attribute__((packed))
{
    uint16_t header;       /* Fixed start-of-frame marker (0xAA55) */
    uint32_t timestamp_ms; /* Timestamp in milliseconds since boot */
    int16_t accel_x;       /* Raw accelerometer X-axis value */
    int16_t accel_y;       /* Raw accelerometer Y-axis value */
    int16_t accel_z;       /* Raw accelerometer Z-axis value */
    int16_t gyro_x;        /* Raw gyroscope X-axis value */
    int16_t gyro_y;        /* Raw gyroscope Y-axis value */
    int16_t gyro_z;        /* Raw gyroscope Z-axis value */
    uint16_t crc;          /* CRC-16 checksum for error detection */
} TelemetryPacket;

/* Compute CRC-16-CCITT checksum over a byte buffer */
uint16_t calc_crc16(const uint8_t data[], size_t length);

/* Populate a TelemetryPacket struct and calculate its CRC */
void build_packet(TelemetryPacket *pkt, uint32_t ts, int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy, int16_t gz);

/* Validate packet header and verify payload integrity via CRC */
bool verify_packet(const TelemetryPacket *pkt);

#endif /* PROTOCOL_H */