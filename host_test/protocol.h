#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PACKET_HEADER 0xAA55

typedef struct __attribute__((packed))
{
    uint16_t header;       // מזהה קבוע לתחילת חבילה (0xAA55)
    uint32_t timestamp_ms; // זמן במילישניות מתחילת המדידה
    int16_t accel_x;       // תאוצה גולמית בציר X
    int16_t accel_y;       // תאוצה גולמית בציר Y
    int16_t accel_z;       // תאוצה גולמית בציר Z
    int16_t gyro_x;        // מהירות זוויתית גולמית X
    int16_t gyro_y;        // מהירות זוויתית גולמית Y
    int16_t gyro_z;        // מהירות זוויתית גולמית Z
    uint16_t crc;          // קוד בדיקת תקינות (CRC-16)
} TelemetryPacket;

uint16_t calc_crc16(const uint8_t data[], size_t length);
void build_packet(TelemetryPacket *pkt, uint32_t ts, int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy, int16_t gz);
bool verify_packet(const TelemetryPacket *pkt);

#endif