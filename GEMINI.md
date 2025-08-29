# BeeCounter Project: Gemini Configuration

This file contains all the configuration, history, and workflow details for the BeeCounter project.

---

## General Principles & Troubleshooting
- **If blocked, search the internet:** When encountering a difficult issue, use the `google_web_search` tool to find potential solutions and workarounds. The solution is likely on the internet.
- **Do not apologize for failures:** Maintain a persistent and helpful, problem-solving approach. The primary goal is to solve the user's problem.
- **Exhaust software solutions first:** When encountering errors, do not immediately assume a hardware fault (e.g., "blame the USB cable"). Exhaust all possible software-based troubleshooting steps before suspecting hardware.

---

## Development Environment
- **Toolchain:** PlatformIO Core (CLI)
- **PlatformIO Executable Path:** `/home/fcurrie/.local/share/pipx/venvs/platformio/bin/platformio`

## ESP32 Device Path
- The ESP32 device is located at `/dev/ttyACM0` on the Raspberry Pi development host.

## Project Configuration
- The ESP-IDF is located at `/home/fcurrie/esp/esp-idf`.

## Hardware Details
- **Board:** ESP32-S3-WROOM-1
- **Chip Type:** ESP32-S3 (QFN56) (revision v0.2)
- **Features:** Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz, Embedded PSRAM 8MB (AP_3v3)
- **Crystal Frequency:** 40MHz
- **USB Mode:** USB-Serial/JTAG
- **MAC Address:** 98:a3:16:f0:4b:ac
- **Flash Manufacturer:** c8
- **Flash Device:** 4017
- **Flash Size:** 8MB
- **Flash Type:** Quad (4 data lines)
- **Flash Voltage:** 3.3V

## Hardware Components
- **OLED Display:** FEATHERWING OLED 1.3" 128X64
  - **DigiKey P/N:** 1528-4650-ND
  - **Manufacturer P/N:** 4650
- **Camera:** ADAFRUIT OV5640 CAMERA BREAKOUT
  - **DigiKey P/N:** 1528-5841-ND
  - **Manufacturer P/N:** 5841
- **Breadboard:** HALF-SIZE BREADBOARD WITH MOUNTING
  - **DigiKey P/N:** 1528-4539-ND
  - **Manufacturer P/N:** 4539

## Sources of Truth
- **Primary Documentation:** https://docs.freenove.com/projects/fnk0099/en/latest/
- **ESP-IDF Get Started Guide (ESP32-S3):** https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html
- **ESP-IDF Linux Setup:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/linux-macos-setup.html#get-started-linux-macos-first-steps
- **ESP32 Forums:** https://esp32.com/
- **Reddit - r/esp32:** https://www.reddit.com/r/esp32/ (A good resource for community support and troubleshooting)

## Project History
**2025-08-25:** Conducted an extensive troubleshooting session to resolve a persistent `termios.error: (25, 'Inappropriate ioctl for device')` when connecting to an ESP32-S3 board on a Raspberry Pi host. The process involved:
1.  Resolving an initial `invalid header` error by correcting the partition table and setting the appropriate flash mode (`qio`) in `platformio.ini`.
2.  Performing a full, forced erase of the device flash.
3.  Testing with a known-good, pre-compiled firmware (CircuitPython).
4.  Verifying and correcting host-side issues: installed udev rules, checked user permissions, and disabled interfering services like `ModemManager`.
5.  Isolating the problem to the Raspberry Pi host by swapping the ESP32 board and USB cable, which confirmed the error was host-specific.
6.  As a final step, downgraded the Raspberry Pi kernel from the 6.12 series to the stable 6.1.21 version via `apt-get`.
7.  After a reboot, the kernel downgrade was confirmed to have resolved the issue, enabling reliable flashing and communication.

## Development Workflow
- **PlatformIO Verbose Output:** Always use the `-v` flag with `platformio run` commands for detailed output.
- **Serial Monitor:** To access the serial port, run `platformio device monitor` in a separate terminal. Attempting to run it in the same terminal as other commands may result in a `termios.error`.
- **Iterative Development Cycle:** Implement features in larger, more complete chunks. To streamline testing, temporarily disable authentication mechanisms. Validate the running application's behavior and output using `curl` against the device's web server endpoints.
- Before upgrading the version, make a copy of the `src/main.cpp` file with the version number as part of the filename (e.g., `main.cpp.v0.7.0`).
- After any successful code modification, ensure the following documentation files are reviewed and updated to reflect the changes:
    - `TODO.md`: Mark completed tasks and add new ones if applicable.
    - `CHANGELOG.md`: Add a new entry under a new version header. The version number should be incremented according to Semantic Versioning (SemVer) principles:
        - **MAJOR** (e.g., 1.x.x -> 2.0.0): For incompatible API changes.
        - **MINOR** (e.g., 0.1.x -> 0.2.0): For adding functionality in a backward-compatible manner.
        - **PATCH** (e.g., 0.1.5 -> 0.1.6): For backward-compatible bug fixes.
    - `README.md`: Update if the changes affect project setup, configuration, or usage.
