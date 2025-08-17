# ESP32-S3 Bee Monitor

This project aims to create a self-sustaining, AI-powered bee monitor using an ESP32-S3. It uses a camera to capture images of a beehive entrance, runs an object detection model to count bees entering and leaving, and sends the data to a remote server.

## Features

*   **On-Device Configuration:** On first boot, the device starts a Wi-Fi Access Point (`BeeMonitor-Setup`) and hosts a web page to configure Wi-Fi credentials, system name, and user access.
*   **AI-Powered Counting:** Uses a TensorFlow Lite Micro model to detect bees in real-time.
*   **Continuous Operation:** Designed for continuous operation during daylight hours, powered by a solar panel and LiPo battery.
*   **Data Logging:** Sends bee counts to a server via HTTP POST every 30 seconds.
*   **Local Display:** Shows the current bee counts and system status on a small OLED display.

## Hardware Requirements

A complete Bill of Materials with specific part numbers is available in the `GEMINI.md` file.

*   **Development Board:** ESP32-S3-DevKitC-1 (or a similar ESP32-S3 board with sufficient GPIOs).
*   **Camera:** OV5640 or similar DVP (Parallel Interface) camera module.
*   **Display:** 0.96" or 1.3" I2C OLED Display (SSD1306/SSD1315 driver).
*   **Accessories:** Breadboard, jumper wires, and a USB-C to Micro-B cable.

## Wiring

### OLED Display (I2C)

| OLED Pin | ESP32-S3 Pin |
|:---------|:-------------|
| GND      | GND.1        |
| 3V3      | 3V3.1        |
| DATA     | GPIO5        |
| CLK      | GPIO4        |

*Note: The I2C bus (GPIO4 & GPIO5) is shared between the camera's configuration interface (SCCB) and the OLED display.*

### Camera (DVP)

| Adafruit OV5640 Pin | ESP32-S3-DevKitC-1 Pin | Purpose                |
|:-------------------|:-------------------------|:-----------------------|
| **3V3**            | **3V3**                  | 3.3V Power             |
| **GND**            | **GND**                  | Ground                 |
| **SIOD**           | **GPIO5**                | SCCB Data (I2C SDA)    |
| **SIOC**           | **GPIO4**                | SCCB Clock (I2C SCL)   |
| **PCLK**           | **GPIO14**               | Pixel Clock            |
| **VSYNC**          | **GPIO6**                | Vertical Sync          |
| **HREF**           | **GPIO7**                | Horizontal Reference   |
| **XCLK**           | **GPIO15**               | System Clock for Camera|
| **D7**             | **GPIO16**               | Data Bit 7             |
| **D6**             | **GPIO17**               | Data Bit 6             |
| **D5**             | **GPIO18**               | Data Bit 5             |
| **D4**             | **GPIO12**               | Data Bit 4             |
| **D3**             | **GPIO11**               | Data Bit 3             |
| **D2**             | **GPIO10**               | Data Bit 2             |
| **D1**             | **GPIO9**                | Data Bit 1             |
| **D0**             | **GPIO13**               | Data Bit 0             |
| **PWDN**           | *(Leave Unconnected)*    | Power Down (Active Low)|
| **RST**            | *(Leave Unconnected)*    | Reset (Active Low)     |

*Note: The I2C bus (GPIO4 & GPIO5) is shared between the camera's configuration interface and the OLED display.*

## Software Setup

This project is built using the Espressif IoT Development Framework (ESP-IDF).

1.  **Clone the Repository:**
    ```bash
    git clone <repository_url>
    cd beemonitor
    ```

2.  **Initialize Submodules:** The project uses git submodules for its components.
    ```bash
    git submodule update --init --recursive
    ```

3.  **Setup ESP-IDF:** Follow the official Espressif documentation to install and configure the ESP-IDF. Make sure to source the export script before building.
    ```bash
    # In the esp-idf directory
    ./install.sh
    # In your project directory
    source /path/to/esp-idf/export.sh
    ```

## How to Build and Flash

1.  **Configure the Project:** (Optional) If you need to change project settings, use the menuconfig tool.
    ```bash
    idf.py menuconfig
    ```

2.  **Build the Firmware:**
    ```bash
    idf.py build
    ```

3.  **Flash the Firmware:** Connect your ESP32-S3 board, find its serial port name (e.g., `/dev/ttyUSB0` on Linux), and run the flash command.
    ```bash
    idf.py -p /dev/ttyUSB0 flash monitor
    ```

This will flash the compiled code onto the board and open a serial monitor to view the log output.
