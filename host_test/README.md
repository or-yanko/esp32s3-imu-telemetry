# Binary Telemetry Protocol Specification (v1.0)

This document defines the wire format, memory layout, and integrity verification mechanism used for serial (UART) telemetry streaming and binary MicroSD logging in the **ESP32-S3 IMU Telemetry** project.

---

## Design Principles

1. **Deterministic Footprint:** Every telemetry frame has a fixed length of exactly **20 bytes**. Fixed-length binary frames eliminate string-parsing overhead (`snprintf`/`sscanf`) on the microcontroller and simplify parser state machines on the receiver.
2. **Cross-Architecture ABI Compatibility:** All fields use explicit fixed-width integer types from `<stdint.h>` (`uint16_t`, `int16_t`, `uint32_t`). The structure is declared with `__attribute__((packed))` to prevent compiler-inserted alignment padding across 32-bit (Xtensa LX7) and 64-bit (ARM64/x86_64) architectures.
3. **Byte Ordering (Endianness):** Multi-byte fields are transmitted in **Little-Endian** format, which is native to both the ESP32-S3 and modern host CPUs, allowing zero-copy casting on compatible architectures.
4. **Frame Synchronization & Integrity:** Each frame begins with a 2-byte magic sync word (`0xAA55`) and terminates with a 2-byte **CRC-16-CCITT** checksum computed over the preceding 18 bytes.

---

## Packet Frame Layout

Total frame size: **20 Bytes** (`sizeof(TelemetryPacket) == 20`).

| Byte Offset | Size (Bytes) | Field Name | C Type | Value / Description |
| :--- | :--- | :--- | :--- | :--- |
| `0x00` | 2 | `header` | `uint16_t` | Fixed synchronization word: `0xAA55` (`0x55`, `0xAA` on wire) |
| `0x02` | 4 | `timestamp_ms` | `uint32_t` | Monotonic system uptime in milliseconds since boot |
| `0x06` | 2 | `accel_x` | `int16_t` | Raw 16-bit signed accelerometer ADC reading (X-axis) |
| `0x08` | 2 | `accel_y` | `int16_t` | Raw 16-bit signed accelerometer ADC reading (Y-axis) |
| `0x0A` | 2 | `accel_z` | `int16_t` | Raw 16-bit signed accelerometer ADC reading (Z-axis) |
| `0x0C` | 2 | `gyro_x` | `int16_t` | Raw 16-bit signed gyroscope ADC reading (X-axis) |
| `0x0E` | 2 | `gyro_y` | `int16_t` | Raw 16-bit signed gyroscope ADC reading (Y-axis) |
| `0x10` | 2 | `gyro_z` | `int16_t` | Raw 16-bit signed gyroscope ADC reading (Z-axis) |
| `0x12` | 2 | `crc` | `uint16_t` | CRC-16-CCITT checksum covering bytes `0x00` through `0x11` |

---

## Error Detection: CRC-16-CCITT

To detect bit flips caused by electrical noise over UART or flash write corruption, the receiver validates every incoming frame before processing.

* **Algorithm:** CRC-16-CCITT (False)
* **Generator Polynomial:** $0x1021$ ($x^{16} + x^{12} + x^5 + 1$)
* **Initial Seed:** $0xFFFF$
* **Input Reflection / Output Reflection:** False / False (MSB-first bitwise shift)
* **Final XOR Value:** $0x0000$
* **Protected Range:** Bytes `0` to `17` (`sizeof(TelemetryPacket) - sizeof(uint16_t)`).

---

## Bandwidth & Throughput Budget

At the target sampling frequency of $f_s = 200\text{ Hz}$:

$$\text{Payload Data Rate} = 20\text{ bytes/frame} \times 200\text{ frames/s} = 4,000\text{ bytes/s} = 32,000\text{ bps}$$

Over a standard **115,200 bps** UART link (8N1 framing, 10 bits per byte):

$$\text{Effective UART Bandwidth} = \frac{115,200}{10} = 11,520\text{ bytes/s}$$

$$\text{Link Utilization} = \frac{4,000}{11,520} \approx 34.7\%$$

This leaves over $65\%$ headroom for CLI command responses, debug logs, and burst retransmissions without saturating the serial bus.