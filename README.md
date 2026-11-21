# ESP32-S3 Real-Time IMU Telemetry & Data Logger

A deterministic, multi-core embedded telemetry and data-logging system built in C on the **ESP32-S3-WROOM** using **ESP-IDF (v5.4)** and **FreeRTOS**. 

Designed without high-level Arduino abstractions, this project focuses on low-level firmware engineering: datasheet-driven peripheral drivers, hardware-timed sampling, lock-free/synchronized inter-task buffering, binary framing with CRC-16 error detection, and host-side unit testing.

---

## Key Engineering Highlights

* **Datasheet-Driven Drivers (Bare I2C):** Custom C drivers written directly from component register maps (MPU6050 6-axis IMU and PCF8574T I2C I/O expander for LCD1602) with explicit error handling and burst-read transactions.
* **Deterministic Sampling & Sensor Fusion:** Hardware timer-driven data acquisition at $200\text{ Hz}$ paired with a Complementary Filter for real-time Roll and Pitch estimation.
* **Dual-Core FreeRTOS Architecture:** Explicit core affinity separating time-critical sensor acquisition (Core 1) from I/O-bound MicroSD binary logging and UART telemetry streaming (Core 0).
* **Zero-Drop Memory Management:** Static memory allocation using a circular Ring Buffer and Double Buffering to decouple high-frequency sensor reads from variable-latency flash writes.
* **Custom Binary Framing Protocol:** Fixed-size, 20-byte packed C structures transmitted over UART and logged to SD, protected by a bitwise **CRC-16-CCITT** checksum (see [`PROTOCOL.md`](PROTOCOL.md)).
* **Host-Side Verification (`host_test/`):** Hardware-independent C test suite compiled natively with `clang` to validate protocol framing, struct alignment, and bit-error detection prior to target deployment.

---

## System Architecture

```text
+-----------------------------------------------------------------------+
|                           ESP32-S3-WROOM                              |
|                                                                       |
|  +-----------------------------+     +-----------------------------+  |
|  |           CORE 1            |     |           CORE 0            |  |
|  |     (Real-Time Domain)      |     |       (I/O & Comms)         |  |
|  |                             |     |                             |  |
|  |  [HW Timer @ 200 Hz]        |     |  [Storage Task]             |  |
|  |          |                  |     |    |---> SD_MMC (Binary Log)|  |
|  |          v                  |     |    |                        |  |
|  |  [IMU Acquisition Task]     |     |  [Telemetry / CLI Task]     |  |
|  |    |---> I2C Burst Read     |     |    |---> UART TX (Packets)  |  |
|  |    |---> Sensor Fusion      |     |    |<--- UART RX (Commands) |  |
|  |          |                  |     |                             |  |
|  +----------|------------------+     +--------------^--------------+  |
|             |                                       |                 |
|             +--------> [ Thread-Safe Ring Buffer ] -+                 |
+-----------------------------------------------------------------------+
```

---

## Hardware Pinout & Constraints

Configured for the **Freenove ESP32-S3-WROOM** module (8MB/16MB Flash, 8MB Octal PSRAM):

| Peripheral / Interface | Signal | ESP32-S3 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **I2C Bus (MPU6050 & LCD)** | `SDA` | `GPIO 4` | External/internal pull-ups, $400\text{ kHz}$ Fast Mode |
| **I2C Bus (MPU6050 & LCD)** | `SCL` | `GPIO 5` | Shared bus (`0x68` IMU, `0x27` LCD) |
| **IMU Interrupt** | `INT` | `GPIO 6` | Data-ready hardware interrupt |
| **MicroSD (SD_MMC 1-bit)** | `CMD` | `GPIO 38` | Dedicated onboard SD slot pin |
| **MicroSD (SD_MMC 1-bit)** | `CLK` | `GPIO 39` | Dedicated onboard SD slot pin |
| **MicroSD (SD_MMC 1-bit)** | `D0` | `GPIO 40` | Dedicated onboard SD slot pin |
| **Status Indicator** | `LED` | `GPIO 2` | Onboard status LED |
| *Reserved (Do Not Use)* | *PSRAM* | `GPIO 35-37` | Reserved internally for OPI PSRAM |

---

## Repository Structure

```text
esp32s3-imu-telemetry/
├── README.md               # Project overview and architecture
├── PROTOCOL.md             # Binary framing & CRC-16 specification
├── host_test/              # Native macOS/Linux unit tests (clang)
│   ├── protocol.h          # Packed telemetry packet definitions
│   ├── protocol.c          # CRC-16-CCITT and framing implementation
│   └── test_main.c         # Unit test runner
├── main/                   # ESP-IDF application entry & RTOS tasks (Planned)
└── components/             # Modular C drivers & middleware (Planned)
    ├── mpu6050/            # Custom I2C driver for MPU6050
    ├── lcd1602/            # Custom I2C driver for PCF8574T + HD44780
    ├── ring_buffer/        # Static circular buffer
    └── storage/            # SD_MMC binary file logger
```

---

## Build & Host-Side Testing

### Running Host-Side Unit Tests (No Hardware Required)
The protocol and signal-processing modules are written in standard C11 and can be compiled and verified directly on the host machine:

```bash
cd host_test
clang -std=c11 -Wall -Wextra protocol.c test_main.c -o test_protocol
./test_protocol
```

### Target Firmware Build (ESP-IDF v5.4)
```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.wchusbserial* flash monitor
```

---

## Development Roadmap

- [x] Repository setup and binary telemetry packet header (`protocol.h`)
- [ ] CRC-16-CCITT implementation and host-side unit test verification
- [ ] Static Ring Buffer implementation and stress testing
- [ ] Complementary Filter orientation math module
- [ ] ESP-IDF project configuration (`esp32s3`, OPI PSRAM, `SD_MMC`)
- [ ] Bare-metal I2C driver for MPU6050 (`WHO_AM_I`, calibration, burst reads)
- [ ] Dual-core FreeRTOS task orchestration, SD logging, and UART CLI