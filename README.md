# ESP32-S3 "BeeCounter" Project

This project is for developing a "BeeCounter" device on an ESP32-S3 board using the PlatformIO Core command-line interface.

## Current Status

The device firmware is now a fully-featured application with two primary modes: **Configuration Mode** and **Application Mode**.

-   **Configuration Mode:** On first boot or after a factory reset, the device starts a Wi-Fi Access Point. A user can connect to this AP to configure the device's Wi-Fi connection and set a system name.
-   **Application Mode:** After successful configuration, the device connects to the designated Wi-Fi network and starts a secure, password-protected web application. This application provides a tabbed interface for monitoring, training, system observability, and administration.

## Features

*   **Wi-Fi Configuration Portal:** A user-friendly, web-based portal to scan for and connect to a local Wi-Fi network.
*   **Persistent Settings:** All settings (Wi-Fi credentials, device name, admin password) are saved to the device's non-volatile flash storage.
*   **Full User Authentication:** A secure login system with a default username (`admin`) and password (`admin`).
*   **Mandatory Password Change:** The default password must be changed on first login to a complex password (8+ characters, with uppercase, number, and symbol).
*   **Tabbed Web Application:** A modern, responsive web UI with "Monitor", "Train", "Observability", and "Administer" tabs.
*   **System Observability:** A dedicated page with live, auto-updating graphs for CPU usage (per core), heap memory, and flash storage. Includes a configurable refresh rate.
*   **Automatic Timezone Detection:** A "Find and Set Timezone" button on the Administer page automatically configures the device's timezone based on its public IP address.
*   **Factory Reset:** A secure function in the "Administer" tab to wipe all settings and return the device to its initial configuration state.
*   **Advanced LED Status:** The onboard LED provides detailed feedback on the device's state, including Wi-Fi connection progress, IP acquisition, and server status.

## First-Time Setup

1.  **Power On:** Power on the ESP32-S3 device. The onboard LED will flash **orange**, indicating it's in Configuration Mode.
2.  **Connect to AP:** On your computer or phone, connect to the Wi-Fi network named **"BeeCounter-Setup"**.
3.  **Access Portal:** A captive portal should automatically open. If not, open a web browser and navigate to `http://192.168.4.1`.
4.  **Configure Wi-Fi:** Follow the on-screen instructions to select your local Wi-Fi network, enter the password, and give your device a name.
5.  **Reboot:** The device will save the settings and reboot.

## Normal Operation

1.  **Login:** After rebooting, the device will connect to your Wi-Fi. The LED will show its progress (blue, flashing green, then solid green). Once the LED is **solid green**, find the device's IP address from your router's client list.
2.  **Access Application:** Navigate to the device's IP address in a web browser.
3.  **First Login:** Log in with the username `admin` and password `admin`.
4.  **Change Password:** You will be immediately redirected and required to set a new, complex password.
5.  **Use Application:** Once the password is changed, you will have access to the main application dashboard.

## Development

1.  **Install PlatformIO Core:** Follow the official instructions to install the PlatformIO Core CLI.
2.  **Connect Board:** Connect the ESP32-S3 board to your computer.
3.  **Build & Upload:**
    ```bash
    platformio run --target upload
    ```
4.  **Monitor Output:**
    ```bash
    platformio device monitor -b 115200
    ```
5.  **Erase Flash (Factory Reset):** To manually perform a factory reset:
    ```bash
    platformio run --target erase
    ```
