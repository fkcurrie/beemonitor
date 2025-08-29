# ESP32-S3 Bee Counter Wiring Guide

This document provides instructions for wiring the FeatherWing OLED and the Adafruit OV5640 Camera to the ESP32-S3-WROOM-1 board on a half-size breadboard.

## Required Materials

- ESP32-S3-WROOM-1 Board
- FEATHERWING OLED 1.3" 128X64 (Adafruit #4650)
- ADAFRUIT OV5640 CAMERA BREAKOUT (Adafruit #5841)
- Half-Size Breadboard
- Jumper Wires (Male-to-Male)

## Pinout References

- **ESP32-S3-WROOM-1:** Refer to the official datasheet for a detailed pinout. Key pins for this project are `3V3`, `GND`, `IO8` (SDA), and `IO9` (SCL).
- **FeatherWing OLED:** Uses I2C communication (`SDA`, `SCL`) and power (`3V`, `GND`).
- **Adafruit OV5640 Camera:** Uses I2C for configuration (`SDA`, `SCL`) and a parallel interface for data.

## Wiring Instructions

### Step 1: Board Placement

1.  Place the ESP32-S3-WROOM-1 board onto the breadboard, spanning the central divider.
2.  Place the FeatherWing OLED and the OV5640 Camera breakout on either side of the ESP32, ensuring their pins are on separate rows.

### Step 2: Power Connections

The ESP32-S3 will provide power to both the OLED and the camera.

1.  Connect a jumper wire from a `3V3` pin on the ESP32 to the **red power rail (+)** on the breadboard.
2.  Connect a jumper wire from a `GND` pin on the ESP32 to the **blue ground rail (-)** on the breadboard.
3.  Connect the **VIN** or `3V` pin on the FeatherWing OLED to the **red power rail (+)**.
4.  Connect the `GND` pin on the FeatherWing OLED to the **blue ground rail (-)**.
5.  Connect the `3V3` pin on the OV5640 Camera to the **red power rail (+)**.
6.  Connect the `G` (GND) pin on the OV5640 Camera to the **blue ground rail (-)**.

### Step 3: I2C Bus Connections (OLED & Camera Configuration)

Both the OLED and the camera's configuration interface share the same I2C bus.

| From Component      | From Pin | To ESP32-S3 Pin | Notes                               |
| ------------------- | -------- | --------------- | ----------------------------------- |
| **FeatherWing OLED**| `SDA`    | `IO8`           | I2C Data Line                       |
| **FeatherWing OLED**| `SCL`    | `IO9`           | I2C Clock Line                      |
| **OV5640 Camera**   | `SDA`    | `IO8`           | Shared with OLED (connect to IO8)   |
| **OV5640 Camera**   | `SCL`    | `IO9`           | Shared with OLED (connect to IO9)   |

*Tip: You can run jumpers from the ESP32's `IO8` and `IO9` pins to separate rows on the breadboard and then connect both the OLED and Camera's I2C pins to those rows.*

### Step 4: Camera Parallel Data & Control Connections

The camera requires a dedicated set of GPIO pins for high-speed data transfer and control.

| From OV5640 Camera | To ESP32-S3 Pin | Description        |
| ------------------ | --------------- | ------------------ |
| `PD` (PWDN)        | `GND`           | Power Down (tie low to enable) |
| `RT` (RESET)       | `3V3`           | Reset (tie high for normal operation) |
| `VSYNC`            | `IO6`           | Vertical Sync      |
| `HREF`             | `IO7`           | Horizontal Sync    |
| `PCLK`             | `IO13`          | Pixel Clock        |
| `XC` (XCLK)        | `IO14`          | External Clock     |
| `D2`               | `IO12`          | Data Bit 2         |
| `D3`               | `IO11`          | Data Bit 3         |
| `D4`               | `IO10`          | Data Bit 4         |
| `D5`               | `IO5`           | Data Bit 5         |
| `D6`               | `IO4`           | Data Bit 6         |
| `D7`               | `IO3`           | Data Bit 7         |
| `D8`               | `IO2`           | Data Bit 8         |
| `D9`               | `IO1`           | Data Bit 9         |

**Important Notes:**
- The pin choices above are a common configuration but may need to be adjusted in the firmware (`main.cpp`) depending on the camera library used.
- Ensure the **XCLK jumper** on the back of the camera board is set to **INT** (the default) to use its internal oscillator.
- Leave the **VM jumper** open.

## Final Check

- Double-check all connections, especially power (`3V3`) and ground (`GND`), to prevent damage.
- Ensure no wires are loose.
- The breadboard should now have the ESP32 at the center, with the OLED and camera connected to it via the power rails and dedicated GPIO pins.
