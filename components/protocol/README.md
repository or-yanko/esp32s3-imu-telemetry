# Binary Telemetry Protocol (`protocol`)

A deterministic, zero-overhead binary framing and integrity verification module written in portable C (`C99`/`C11`) for the **ESP32-S3 IMU Telemetry** system.

This module defines the wire format, memory layout, serialization routines, and **CRC-16-CCITT** error-detection mechanism shared across real-time **UART serial streaming**, **MicroSD binary logging**, and **host-side data parsers**.

---

## System Architecture & Data Flow

Raw hardware interfaces such as UART transmit unstructured streams of individual bytes (`uint8_t`) without packet boundaries or error checking. This protocol imposes a fixed 20-byte frame structure between the embedded producer (ESP32-S3) and the consumer (host PC or offline log reader):

```text
+-------------------+      I2C      +-----------------------------------+
|  MPU-6050 Sensor  | ------------> |         ESP32-S3 Firmware         |
| (6-Axis Acc/Gyro) |               |  build_packet() -> 20-Byte Frame  |
+-------------------+               +-----------------------------------+
                                              |               |
                                 UART (115200 bps)       SPI (FAT32)
                                              v               v
                                    +-----------------+ +---------------+
                                    |  Host Receiver  | | MicroSD Card  |
                                    | verify_packet() | | Binary Log    |
                                    +-----------------+ +---------------+
```

---

## Core Design Principles

1. **Deterministic Footprint (20 Bytes):** Every telemetry frame has a fixed length of exactly **20 bytes**. Fixed-length binary frames eliminate string-formatting overhead (`snprintf` / `sscanf`) on the microcontroller and allow O(1) buffer indexing on the receiver.
2. **Cross-Architecture ABI Compatibility:** All fields use explicit fixed-width integer types from `<stdint.h>` (`uint16_t`, `int16_t`, `uint32_t`). The structure is declared with `__attribute__((packed))` to prevent compilers from inserting alignment padding bytes across 32-bit (Xtensa LX7) and 64-bit (ARM64 / x86_64) processors.
3. **Little-Endian Byte Ordering:** Multi-byte integers are stored and transmitted in **Little-Endian** format (least significant byte first), which is native to both the ESP32-S3 and modern x86/ARM host machines.
4. **Hardware-Independent Portability:** The module relies solely on standard C headers (`<stdint.h>`, `<stddef.h>`, `<stdbool.h>`) with zero ESP-IDF driver dependencies, enabling identical compilation on both the target MCU and native host unit-test suites.

---

## Packet Frame Layout

Total frame size: **20 Bytes** (`sizeof(TelemetryPacket) == 20`).

```text
 0      1 2            5 6      7 8      9 10    11 12    13 14    15 16    17 18    19
+--------+--------------+--------+--------+--------+--------+--------+--------+--------+
| Header | Timestamp ms | AccelX | AccelY | AccelZ | Gyro X | Gyro Y | Gyro Z | CRC-16 |
| 0xAA55 |   uint32_t   | int16  | int16  | int16  | int16  | int16  | int16  | uint16 |
+--------+--------------+--------+--------+--------+--------+--------+--------+--------+
|<----------------------- Protected by CRC-16 (18 Bytes) ---------------------->|
```

| Byte Offset | Size (Bytes) | Field Name | C Type | Value / Description |
| :--- | :--- | :--- | :--- | :--- |
| `0x00` | 2 | `header` | `uint16_t` | Fixed start-of-frame sync word: `0xAA55` (`0x55`, `0xAA` on wire) |
| `0x02` | 4 | `timestamp_ms` | `uint32_t` | Monotonic system uptime in milliseconds since boot |
| `0x06` | 2 | `accel_x` | `int16_t` | Raw 16-bit signed accelerometer ADC reading (X-axis) |
| `0x08` | 2 | `accel_y` | `int16_t` | Raw 16-bit signed accelerometer ADC reading (Y-axis) |
| `0x0A` | 2 | `accel_z` | `int16_t` | Raw 16-bit signed accelerometer ADC reading (Z-axis) |
| `0x0C` | 2 | `gyro_x` | `int16_t` | Raw 16-bit signed gyroscope ADC reading (X-axis) |
| `0x0E` | 2 | `gyro_y` | `int16_t` | Raw 16-bit signed gyroscope ADC reading (Y-axis) |
| `0x10` | 2 | `gyro_z` | `int16_t` | Raw 16-bit signed gyroscope ADC reading (Z-axis) |
| `0x12` | 2 | `crc` | `uint16_t` | CRC-16-CCITT checksum computed over bytes `0x00` through `0x11` |

### Wire Representation Example

For a sample captured at `timestamp_ms = 1500` (`0x000005DC`) with `accel = (120, -45, 16384)` and `gyro = (10, 20, -5)`, the serialized 20-byte frame appears on the wire in Little-Endian byte order as:

```text
Offset 0x00..0x05 : 55 AA | DC 05 00 00             (header = 0xAA55, ts = 1500 ms)
Offset 0x06..0x0B : 78 00 | D3 FF | 00 40           (accel_x = 120, accel_y = -45, accel_z = 16384)
Offset 0x0C..0x11 : 0A 00 | 14 00 | FB FF           (gyro_x = 10, gyro_y = 20, gyro_z = -5)
Offset 0x12..0x13 : <CRC_LSB> <CRC_MSB>             (CRC-16-CCITT over bytes 0x00..0x11)
```

---

## C API Reference (`protocol.h`)

### `TelemetryPacket`
Packed structure representing a single 20-byte telemetry frame.

```c
typedef struct __attribute__((packed)) {
    uint16_t header;
    uint32_t timestamp_ms;
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    uint16_t crc;
} TelemetryPacket;
```

### `calc_crc16`
```c
uint16_t calc_crc16(const uint8_t data[], size_t length);
```
Computes a 16-bit CRC-16-CCITT checksum over an arbitrary byte buffer.
* **Parameters:** `data` — pointer to input byte array; `length` — number of bytes to process.
* **Safety:** Returns `0` immediately if `data == NULL`.

### `build_packet`
```c
void build_packet(TelemetryPacket *pkt, uint32_t ts,
                  int16_t ax, int16_t ay, int16_t az,
                  int16_t gx, int16_t gy, int16_t gz);
```
Populates all payload fields of `pkt`, sets `pkt->header = 0xAA55`, computes the CRC-16 checksum over the first 18 bytes (`sizeof(TelemetryPacket) - sizeof(uint16_t)`), and writes the result into `pkt->crc`.
* **Safety:** Performs a null-pointer guard (`if (pkt == NULL) return;`).

### `verify_packet`
```c
bool verify_packet(const TelemetryPacket *pkt);
```
Validates an incoming or stored telemetry packet on the receiver side.
* **Checks performed:**
  1. Verifies `pkt != NULL`.
  2. Verifies `pkt->header == 0xAA55`.
  3. Recomputes CRC-16 over bytes `0..17` and verifies exact equality with `pkt->crc`.
* **Returns:** `true` if the frame is synchronized and intact; `false` if corrupted or misaligned.

---

## Error Detection: CRC-16-CCITT

Electrical noise on UART lines or flash write errors can flip individual bits inside a frame. The protocol uses bitwise polynomial division modulo 2 (**CRC-16-CCITT False**) to detect single-bit, double-bit, and burst errors.

* **Generator Polynomial:** $G(x) = x^{16} + x^{12} + x^5 + 1$ (`0x1021`)
* **Initial Seed:** `0xFFFF` (ensures leading `0x00` bytes affect the remainder)
* **Bit Processing Order:** MSB-first (`0x8000` mask test followed by left-shift and conditional XOR)
* **Final XOR Value:** `0x0000`
* **Protected Byte Range:** Bytes `0` through `17` (`18` bytes total)
* **Standard Compliance Vector:** ASCII `"123456789"` evaluates to **`0x29B1`**

---

## Receiver Synchronization Logic

When the host connects mid-stream or encounters a dropped byte over UART, it uses a deterministic framing sequence to recover synchronization:

1. **Header Scan:** Read incoming bytes until the 2-byte sequence `0x55`, `0xAA` (`0xAA55` in Little-Endian) is detected.
2. **Frame Accumulation:** Read the remaining `18` bytes into a 20-byte buffer.
3. **Integrity Validation:** Cast the buffer to `const TelemetryPacket *` and call `verify_packet()`.
4. **Commit or Discard:**
   * If `verify_packet()` returns `true`, extract the sensor values and advance by 20 bytes.
   * If `verify_packet()` returns `false` (indicating either bit corruption or a false-positive `0xAA55` occurrence inside sensor payload bytes), discard the frame and resume scanning from the next byte offset.

---

## Bandwidth & Throughput Budget

At the target IMU sampling rate of $f_s = 200\text{ Hz}$:

$$\text{Payload Data Rate} = 20\text{ bytes/frame} \times 200\text{ frames/s} = 4,000\text{ bytes/s} = 32,000\text{ bps}$$

Over a standard **115,200 bps** UART link (8N1 framing, 10 bits per byte):

$$\text{Effective UART Bandwidth} = \frac{115,200}{10} = 11,520\text{ bytes/s}$$

$$\text{Link Utilization} = \frac{4,000}{11,520} \approx 34.7\%$$

This leaves over **65%** link headroom for bidirectional CLI commands, diagnostic logs, and burst transfers without saturating the serial bus.

---

## Host-Side Unit Testing

The protocol implementation is verified natively on macOS/Linux using `clang` via `test_protocol.c`:

```bash
clang -Wall -Wextra protocol.c test_protocol.c -o protocol_test && ./protocol_test
```

**Verified Test Cases:**
* Standard CRC-16-CCITT vector validation (`"123456789"` $\rightarrow$ `0x29B1`).
* Exact binary structure size verification (`sizeof(TelemetryPacket) == 20`).
* Round-trip packet serialization (`build_packet`) and validation (`verify_packet`).
* Single-bit fault injection (`pkt.accel_x ^= 0x0001`) confirming corrupted frames are rejected.