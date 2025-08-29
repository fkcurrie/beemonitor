# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [0.12.0] - 2025-08-28

### Added
- **Periodic Status Logging:** The device now prints its system name, Wi-Fi SSID, and IP address to the serial console every 30 seconds for easier monitoring.

### Changed
- **Troubleshooting:** The firmware has been temporarily modified to bypass the Wi-Fi configuration portal and connect directly to a hardcoded SSID and password for debugging purposes.
- **Troubleshooting:** The camera initialization is now temporarily disabled to prevent boot loops caused by a suspected hardware issue. This allows the web server and other functions to run.

### Fixed
- **Filesystem Corruption:** Resolved a `Failed to mount LittleFS` error by using the `uploadfs` command to correctly provision the filesystem after a full flash erase.

## [0.11.0] - 2025-08-28

### Added
- **Edge Impulse Integration:** The "Train" page now includes a section to configure Edge Impulse API credentials and directly upload all collected images to the cloud for training.
- **Asynchronous Uploading:** The Edge Impulse upload process runs in a background task on the ESP32, allowing the web UI to remain responsive.
- **Real-time Upload Status:** The "Train" page now provides real-time feedback on the image upload progress using Server-Sent Events.

### Fixed
- **Compiler Errors:** Resolved a compilation error related to the `HTTPClient` library's `sendRequest` method by correctly passing the file object as a stream pointer.
- **Function Redefinition:** Removed a duplicate placeholder function that was causing a build failure.

## [0.10.0] - 2025-08-28

### Added
- **Brute-Force Protection:** The login page now locks out the user for 5 minutes after 5 consecutive failed login attempts.
- **Secure OTA Updates:** Added support for password-protected, over-the-air (OTA) firmware updates. The "Administer" page now includes a UI for uploading new firmware, which can be flashed using the PlatformIO `upload --upload-port [IP]` command.

## [0.9.0] - 2025-08-28

### Added
- **Wi-Fi Reconnection Logic:** The device now automatically detects a lost Wi-Fi connection and attempts to reconnect for 30 seconds. If it fails, the device reboots to ensure recovery.
- **Watchdog Timer:** An ESP32 hardware watchdog timer is now enabled. If the main application loop hangs for more than 10 seconds, the device will automatically reboot.
- **Error Logging:** Critical errors, such as Wi-Fi connection failures, are now logged to a timestamped `error.log` file on the device's flash storage. The log is rotated to `error.log.bak` if it exceeds 10KB.

## [0.8.4] - 2025-08-28

### Fixed
- **Monitor Page HTML:** Corrected an issue where the `message` variable was not correctly embedded into the HTML on the "Monitor" page, resulting in literal string concatenation syntax being displayed.

## [0.8.3] - 2025-08-28

### Fixed
- **Observability Graphs:** Corrected the implementation of observability graphs to use Server-Sent Events (SSE) for real-time data updates, resolving the issue where graphs were not displaying. Removed an unused `observability.js` file and updated `TROUBLESHOOTING.md` to reflect the current SSE-based approach and debugging steps.

## [0.8.2] - 2025-08-28

### Fixed
- **Timezone Persistence:** Resolved the final outstanding issue with timezone persistence. The configured timezone is now correctly applied on boot and persists reliably across reboots.

## [0.8.1] - 2025-08-27

### Changed
- **Camera Controls:** The camera controls on the "Monitor" page are now functional. The selected resolution and JPEG quality are saved to the device's non-volatile storage, ensuring they will be applied when the camera is integrated.

## [0.8.0] - 2025-08-27

### Added
- **Camera Control UI:** The "Monitor" page now features a placeholder for the camera feed and UI controls for adjusting camera settings. This includes a dropdown for resolution and a slider for JPEG quality, based on the capabilities of the OV5640 sensor.

## [0.7.3] - 2025-08-27

### Changed
- **UI Consistency:** The Wi-Fi setup and welcome pages now share the same visual theme as the main application, including the bee icon and color scheme, for a more professional and consistent user experience.

## [0.7.2] - 2025-08-27

### Added
- **Configurable Refresh Rate:** The data refresh rate on the Observability page is now fully configurable. The device will now only send performance data at the interval selected in the UI, significantly reducing CPU load and network traffic when a slower refresh rate is chosen.

### Fixed
- **High CPU Load:** Resolved an issue where the device was sending performance data every 2 seconds, regardless of the refresh interval selected in the UI. This caused unnecessarily high CPU load.

## [0.7.1] - 2025-08-27

### Fixed
- **Timezone Persistence:** The timezone setting is now correctly loaded from non-volatile storage on boot. This resolves a long-standing bug where the user-configured timezone would not be applied after a reboot, causing the device to fall back to the default EST timezone.
- **Filesystem Corruption:** Implemented a clean-flash procedure to resolve a recurring LittleFS corruption issue that was preventing the device from booting.
- **Core Dump Partition:** Added a dedicated partition for core dumps to aid in future debugging. This resolves the "No core dump partition found!" error message on startup.

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.7.0] - 2025-08-27

### Added
- **UI Branding:** Added a bee icon to the header and login page to improve branding.
- **Wi-Fi Signal Strength Graph:** Added a new graph to the Observability page to monitor Wi-Fi signal strength (RSSI) in real-time.
- **Graph Baselines:** Added visual baselines for CPU, Memory, Flash, and Wi-Fi graphs to make them easier to interpret at a glance.
- **Build Timestamp:** The "About" page now includes the exact date and time of the firmware build.
- **IP Address Display:** The "About" page now shows both the local and public IP addresses of the device.

### Changed
- **UI Layout:** Standardized all pages on a more compact, card-based layout.
- **Admin Page:** Re-designed the Administer page with a two-column layout for better space utilization.
- **Color Scheme:** Lightened the background color to a dark grey for better contrast and readability.
- **Footer Clock:** The clock in the footer now updates in real-time every minute.

## [0.6.5] - 2025-08-27

### Fixed
- **Timezone Persistence:** Resolved the persistent timezone issue by using a POSIX TZ string (`EST5EDT,M3.2.0,M11.1.0`) instead of an IANA timezone name. This is a more robust method for embedded systems and correctly applies DST rules, ensuring the local time is always accurate.

## [0.6.4] - 2025-08-26

### Fixed
- **Timezone Persistence:** Reverted the `setenv` and `tzset` approach. Now using `configTzTime()` directly with IANA timezone names, which is the recommended method for ESP32 Arduino core. This should correctly handle daylight saving and timezone offsets.

## [0.6.3] - 2025-08-27

### Added
- **Auto-Refresh on Reboot Page:** The "Rebooting..." page now automatically refreshes every 5 seconds until the device reboots and the web server is back online.

## [0.6.2] - 2025-08-27

### Fixed
- **Timezone Persistence:** The timezone is now correctly saved to non-volatile storage, ensuring it persists across reboots.

## [0.6.1] - 2025-08-27

### Fixed
- **Observability Graphs:** The graphs on the Observability page are now rendering correctly.

## [0.6.0] - 2025-08-26

### Added
- **Observability Page:** A new tab has been added to the web interface to display real-time system performance metrics.
- **Performance API:** Created a new API endpoint at `/api/performance` to serve system metrics (CPU, Heap, Flash) as JSON.
- **Live Performance Graphs:** The Observability page features modern, auto-updating charts for CPU usage (per core), heap memory usage, and flash storage usage.
- **Configurable Refresh Rate:** Added a dropdown to the Observability page to allow selecting the refresh interval for the graphs.

### Changed
- **Timezone Detection:** The timezone is now set via a "Find and Set Timezone" button on the Administer page, which uses the device's public IP address. This is more reliable than browser-based methods.
- **UI Simplification:** Removed the complex and unreliable timezone selection UI (autocomplete, map, and dropdowns) from the Administer page.
- **System Control:** Added a dedicated "Reboot Device" button to the Administer page.

## [0.5.0] - 2025-08-26

### Changed
- **Timezone Selection:** Replaced the interactive map with a more efficient and reliable text-based autocomplete field. As the user types a city name, suggestions are fetched from an external API and the corresponding timezone is set.

### Removed
- **Leaflet.js Map:** Removed the interactive map from the "Administer" page to simplify the interface and improve performance.

## [0.4.2] - 2025-08-26

### Changed
- **Timezone Selection:** Replaced the timezone dropdown with an interactive map (using Leaflet.js and OpenStreetMap) for a more user-friendly experience.
- **UI Improvement:** The web interface footer is now "sticky" and will remain at the bottom of the page when scrolling.
- **UI Improvement:** Links in the main content area are now styled with the high-contrast theme color to improve visibility.
- **UI Improvement:** The changelog is now displayed embedded within the site template instead of opening in a new tab.

### Fixed
- **Build Error:** Corrected a C++ syntax error caused by unescaped quotes in the embedded JavaScript for the timezone map.

## [0.4.1] - 2025-08-26

### Added
- **Filesystem Support:** Integrated the LittleFS filesystem to serve static files like the changelog.
- **Build Automation:** Implemented a Python script to automatically capture the version from this file, along with build host/OS details, and generate a `version.h` header.

### Fixed
- **Build Warning:** Resolved a C++17 compiler warning by refactoring the code to use a standard iterator loop.

## [0.4.0] - 2025-08-26

### Added
- **OLED Display Integration:** The 1.3" OLED screen now displays boot status, Wi-Fi connection details, and live system information.
- **About Page:** Created a new web page that displays detailed build and version information, with a link to this changelog.

## [0.3.0] - 2025-08-26

### Added
- **mDNS Support:** The device can now be accessed on the local network via a configurable hostname (e.g., `http://beecounter.local`).
- **NTP Time Sync:** On startup, the device synchronizes its internal clock with an NTP server.
- **Timezone Configuration:** Added a dropdown menu on the "Administer" page to allow setting the local timezone.
- **Dynamic Footer:** The web interface footer now dynamically displays the device's name, firmware version, and the current system time.

## [0.2.0] - 2025-08-25

### Added
- **Full Authentication System:** Implemented a complete user login system with mandatory password changes on first use.
- **Session Management:** Basic session handling using cookies.
- **Tabbed User Interface:** Created the primary UI with "Monitor", "Train", and "Administer" tabs.
- **Factory Reset:** Added a "Factory Reset" option to the Administer tab.
- **Advanced LED Feedback:** Implemented a staged LED feedback system for detailed connection status.

### Changed
- **Application Logic Overhaul:** Restructured the firmware to support distinct Configuration and Application modes.
- **UI/UX Improvements:** The Wi-Fi setup page now sorts networks by signal strength and the password change page includes a "Show Password" feature.

### Fixed
- **DHCP Failures:** The device now correctly verifies that it has received an IP address.
- **Password Validation:** Corrected the client-side regex for password validation.

## [0.1.0] - 2025-08-25

### Added
- **Initial Project Setup:** Created `TODO.md`, `README.md`, and this `CHANGELog.md`.
- **Web Server:** Implemented a web-based configuration portal using `ESPAsyncWebServer`.
- **Wi-Fi Access Point:** The device now starts a Wi-Fi AP on boot for initial setup.
- **LED Status Indicator:** The onboard RGB LED is used to show system status.
- **Wi-Fi Scanning:** Added functionality to scan for and display available Wi-Fi networks.

### Changed
- **Toolchain:** Decided to use PlatformIO Core (CLI).
- **Memory Optimization:** Refactored web page generation to write directly to the response stream, reducing memory usage.

### Fixed
- **Serial Port Conflicts:** Resolved initial `termios.error` by identifying and terminating lingering `minicom` processes.

### Removed
- **Arduino IDE:** Removed all dependencies on the Arduino IDE in favor of a pure command-line workflow.