
## Recently Completed (v0.12.0 - 2025-09-07)

### 🎥 Complete Camera Integration
- [x] **OV3660 Camera Integration:** Successfully integrated camera from esp32camsd project into BeeCounter
  - Live camera feed with 500ms auto-refresh on Monitor page
  - Real-time camera stream via `/stream` endpoint
  - Single photo capture via `/capture` endpoint  
  - Camera sensor status via `/status` endpoint
- [x] **Real-time Camera Controls:** Resolution and JPEG quality settings apply immediately without restart
  - UXGA, SXGA, XGA, SVGA, VGA, CIF, QVGA resolution support
  - JPEG quality adjustment (10-63)
  - Parameter validation and error handling
- [x] **Hardware Migration:** Updated from ESP32-S3-DevKitC-1 to Freenove ESP32-S3-WROOM Camera Board
  - OV3660 sensor configuration with DVP interface
  - 8MB PSRAM detection and utilization
  - ESP32S3_EYE pin mapping integration

### 🌐 User Interface Improvements  
- [x] **Fixed Navigation Tabs:** All tabs (Train, Observability, Administer) now properly route and function
- [x] **Human-readable Timezone Display:** Converts POSIX format to readable names (e.g., "Eastern Time (US & Canada)")
- [x] **Camera Feed Error Handling:** Comprehensive status messages and graceful degradation

### 🔧 System Stability
- [x] **Resolved Watchdog Timeouts:** Fixed persistent 20-second timeout issues during camera initialization
- [x] **Brownout Detector Disable:** Prevents camera initialization timeouts
- [x] **Strategic Watchdog Management:** Extended timeouts and reset points throughout setup process
- [x] **PSRAM Integration:** Proper detection and buffer allocation for camera operations

---

## Previous Completed Features

- [x] **Fix Timezone Display and Persistence:** The time in the footer does not always reflect the correct timezone after being set, and the timezone setting does not persist across reboots. Investigate and ensure the system time is correctly updated, displayed, and saved to non-volatile storage.
- [x] **Implement Monitor Page:** Design and build the UI to display real-time bee counting data.
- [x] **Implement Train Page:** Create the interface for the machine learning/training component.
- [x] **Add Wi-Fi Signal Strength Graph:** Added a new graph to the Observability page to monitor Wi-Fi signal strength (RSSI) in real-time.
- [x] **Add mDNS Support:** Allow accessing the device at a friendly hostname like `http://beecounter.local`.
- [x] **Add "Change Password" to Admin Page:** Allow the admin to change their password after the initial setup.
- [x] **Implement Dynamic Footer:** Show device name, version, and current date/time in the footer of all pages.
- [x] **Implement Autocomplete Timezone Selection:** The user can type a city name and select from a list of matching timezones.

---
### Suggested Future Enhancements

#### Security
- [x] **Brute-Force Protection:** Lock login attempts after several failed tries to prevent password guessing.
- [x] **Secure OTA Updates:** Implement Over-the-Air firmware updates with cryptographic signature validation.
- [ ] **HTTPS/SSL:** Explore adding SSL/TLS to the web server for encrypted communication (may be resource-intensive).
- [ ] **Improved Session Management:** Use a more cryptographically secure method for generating session IDs.

#### Stability
- [x] **Watchdog Timer:** Implement a watchdog to automatically reboot the device if the main application loop hangs.
- [x] **Wi-Fi Reconnection Logic:** Add robust logic to automatically reconnect to Wi-Fi if the connection is lost during operation.
- [x] **Error Logging:** Save critical errors (e.g., connection failures) to a log file on the device's flash storage for easier debugging.
- [x] **Enhanced Debugging:** Added periodic status updates to the serial console and temporarily disabled problematic hardware to ensure boot reliability during troubleshooting.

---
### Completed
- [x] **Fix Monitor Page HTML:** Corrected an issue where the `message` variable was not correctly embedded into the HTML on the "Monitor" page.
- [x] **Auto-Refresh on Reboot Page:** The "Rebooting..." page now automatically refreshes every 5 seconds until the device reboots and the web server is back online.
- [x] **Fix Observability Graphs:** Corrected the implementation of observability graphs to use Server-Sent Events (SSE) for real-time data updates, resolving the issue where graphs were not displaying.
- [x] **Clean up Observability Files:** Removed unused `data/observability.js` and updated `TROUBLESHOOTING.md` to reflect the current SSE-based approach and debugging steps.
- [x] **Add Observability Page:** Created a new tab with live performance graphs.
- [x] **Automate Versioning:** Create a build script to automatically extract the version from `CHANGELOG.md` and pass it to the compiler to prevent version mismatches.
- [x] **Implement Full Authentication:** Create a secure login system with mandatory password changes.
- [x] **Implement Factory Reset:** Add a secure method to erase all user settings.
- [x] **Create Tabbed UI:** Build the main application interface with Monitor, Train, and Administer tabs.
- [x] **Implement Advanced LED Feedback:** Provide detailed, staged status updates via the onboard LED.
- [x] **Persist Configuration:** Save all Wi-Fi and user settings to the device's flash memory.
- [x] **Implement Station (Client) Mode:** The device now connects to a user-configured Wi-Fi network.
- [x] **Verify Fix for Blank Page Issue:** The web configuration portal now loads correctly.
- [x] **Implement Web Server:** Create a web portal for configuration.
- [x] **Implement Wi-Fi Scanning:** Scan for and display local Wi-Fi networks.
- [x] **Control Onboard LED:** Use the RGB LED as a status indicator.
- [x] **Establish Reliable Flashing:** Resolve all serial port and driver issues.
