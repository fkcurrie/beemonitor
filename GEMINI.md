# Gemini Project Guidelines: ESP32-S3 Bee Monitor

This document provides guidelines for the AI assistant to effectively contribute to this project.

## 1. General Instructions

*   **Prioritize Research:** Before starting any investigation, creating new code, or answering technical questions, **always** use the `google_web_search` tool. Focus searches on official documentation and reputable community sources.
*   **Verify with Official Documentation:** The ESP-IDF and ESP32-S3 documentation from Espressif are the primary sources of truth. Cross-reference information against them.
*   **Analyze Existing Code:** Before writing new code, always analyze the existing project structure, style, and patterns.

## 2. Project Overview

*   **Goal:** Develop a self-sustaining system to monitor the exterior of a beehive. The system will use a "Faster Objects, More Objects" (FOMO) AI model to reliably count bees entering and leaving the hive in real-time.

## 3. Application Logic

*   **Power Management:** The device is battery-powered and must conserve energy. It will use a deep-sleep architecture.
    *   The ESP32 will wake up on a 30-second timer.
    *   On wake, it will connect to Wi-Fi, capture an image, run the AI model, update counts, send data to the server, and update the local display.
    *   It will then enter deep sleep to minimize power consumption.
*   **State Persistence:** Bee counts must be preserved during deep sleep. They will be stored in RTC memory using the `RTC_DATA_ATTR` attribute.
*   **Counting Logic:** The system will use two virtual "tripwire" lines in the camera's field of view to determine bee direction.
    *   Crossing outer line then inner line = "entering".
    *   Crossing inner line then outer line = "leaving".
*   **Data Handling:**
    *   Every 30 seconds (on wake-up), it will send the current counts as a JSON payload (e.g., `{"in": 12, "out": 15}`) via an HTTP POST request to a user-specified server endpoint.
    *   The local counts will be displayed on a small I2C OLED screen.

## 4. Hardware Specifications

*   **Microcontroller:** ESP32-S3 (Dual-core Xtensa LX7, 240 MHz, 512KB SRAM)
*   **Memory:** 16MB Quad SPI Flash
*   **Power Source:** LiPo Battery charged by a Solar Panel.
*   **Development Board:** ESP32-S3-EYE (or a similar board with a DVP camera interface)
*   **Wireless:** 2.4 GHz Wi-Fi (802.11 b/g/n) & Bluetooth 5 (LE)
*   **Connected Peripherals:**
    *   **Camera:** OV5640 or similar (via DVP interface).
    *   **Display:** 0.96" I2C OLED Display (SSD1306).
    *   **Power Management:** Solar charge controller (e.g., CN3791 or similar MPPT board).

### Bill of Materials (Shopping List)

This is the recommended list of components for building the project hardware.

| Qty | Digi-Key P/N              | Description                             |
|:----|:--------------------------|:----------------------------------------|
| 1   | `1965-ESP32-S3-DEVKITC-1-N8R8-ND` | ESP32-S3-WROOM-1-N8R8 Dev Board         |
| 1   | `1528-4650-ND`            | Adafruit FeatherWing OLED 1.3" 128x64   |
| 1   | `1528-5841-ND`            | Adafruit OV5640 Camera Breakout (Fixed-Focus) |
| 1   | `1528-4539-ND`            | Half-Size Breadboard                    |
| 2   | `1568-12794-ND`           | Jumper Wires M/F 6" (20 pack)           |
| 1   | `380-SC-2CMK010M-ND`      | USB-C to Micro-B Cable (3.28')          |


## 5. Development Environment & Toolchain

*   **Framework:** ESP-IDF
*   **Serial Port:** `/dev/ttyUSB0` or `/dev/ttyACM0` (for Linux)
*   **Key Commands:**
    *   **Build:** `idf.py build`
    *   **Flash:** `idf.py -p [Your Serial Port] flash`
    *   **Monitor:** `idf.py -p [Your Serial Port] monitor`
    *   **All-in-one:** `idf.py -p [Your Serial Port] flash monitor`

## 6. Coding Conventions

*   **Language:** C/C++
*   **Style:** Adhere to the existing code style. If starting fresh, use the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) as a baseline.
*   **Naming:**
    *   `snake_case_for_functions_and_variables`
    *   `PascalCaseForClassesAndStructs`
    *   `UPPER_SNAKE_CASE_for_macros_and_constants`
*   **Comments:** Use Doxygen-style comments for documenting functions.

## 7. Dependencies & Components

*   **Component Manager:** ESP-IDF Component Manager.
*   **Key Libraries:**
    *   `espressif/esp32-camera`
    *   `Edge Impulse C++ Library` (contains the FOMO model)
    *   `espressif/esp-nn`
    *   `espressif/esp-http-client`
    *   `U8g2` (or similar OLED driver)

## 8. Project Management & Documentation

*   **Task Tracking (`TODO.md`):** This file contains a list of tasks to be completed. When starting a new task, please consult this file. When a task is completed, it should be marked as done.
*   **Change History (`CHANGELOG.md`):** All significant changes to the codebase, such as new features, bug fixes, or refactoring, must be documented in this file under the `[Unreleased]` section.

## 9. Wokwi Simulation Guidelines

When creating or modifying a `diagram.json` file for Wokwi simulation, the following conventions must be strictly followed to ensure correctness:

*   **Part `id`:** The `id` assigned to a part in the `parts` array is the prefix used for its pins in the `connections` array. For example, if a part has `"id": "oled"`, its pins must be referenced as `"oled:PIN_NAME"`.
*   **Pin Names:** Pin names are specific to the Wokwi part and must be verified from the official Wokwi documentation or a known working example. Do not assume standard pin names (e.g., `VCC` vs. `VIN` vs. `3V3`).
    *   **`wokwi-ssd1306` Correct Pins:** `GND`, `3V3`, `DATA`, `CLK`.
*   **Numbered Pins:** For boards like the ESP32-S3-DevKitC-1 that have multiple pins with the same label (e.g., GND, 3V3), a suffix must be used (e.g., `esp:GND.1`, `esp:3V3.1`).
*   **GPIO Pins:** GPIO pins on the main board are referenced directly by their number (e.g., `esp:5`, `esp:4`).