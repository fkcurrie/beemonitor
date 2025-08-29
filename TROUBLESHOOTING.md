# Troubleshooting Log

## Current Issues

### Observability Graphs Not Displaying
- **Symptom:** The "Observability" page loads and shows the "Refresh Rate" dropdown, but no graphs appear.
- **Investigation & Attempts:**
    1.  **Initial Implementation:** The feature was initially designed with a separate `observability.js` file and a `/api/performance` endpoint. This approach was abandoned due to complexity and memory constraints.
    2.  **Current Implementation (Server-Sent Events):** The current implementation embeds JavaScript directly into the `Observability` page HTML and uses Server-Sent Events (SSE) via the `/events` endpoint to push performance data to the client. The client-side JavaScript listens for `performance_update` events and updates the charts accordingly.
    3.  **Debugging Steps:**
        -   **Check the browser's Developer Console (F12):**
            -   **"Console" tab:** Look for any JavaScript errors. The embedded script now includes `console.log` statements to indicate when the script loads, when SSE connects/disconnects, and when `performance_update` events are received.
            -   **"Network" tab:** Confirm that the `/events` endpoint is being accessed and that `performance_update` events are being received with valid JSON data.
- **Next Steps:**
    -   Based on the console output, identify if the JavaScript is failing to execute, if SSE events are not being received, or if the data within the events is malformed.

## Common Issues

### Device is Unresponsive or in a Bad State
- **Symptom:** The device is not connecting to Wi-Fi, the web interface is inaccessible, or you have forgotten the admin password.
- **Solution:** Perform a **Factory Reset**. This will erase all saved settings and return the device to its initial configuration mode. You can do this in two ways:
    1.  **Web Interface:** If you can still log in, navigate to the "Administer" tab and click the "Factory Reset" button.
    2.  **Manual Erase:** Connect the device to your computer and run the following PlatformIO command:
        ```bash
        platformio run --target erase
        ```
        After the erase is complete, you must re-upload the firmware:
        ```bash
        platformio run --target upload
        ```

### `termios.error` with PlatformIO Serial Monitor
- **Symptom:** Running `pio device monitor` fails with the error `termios.error: (25, 'Inappropriate ioctl for device')`.
- **Diagnosis:** This appears to be an issue specific to PlatformIO's serial monitor implementation, as other serial tools like `minicom` can connect successfully.
- **Workaround:** Use `minicom` to view the serial output from the device: `minicom -b 115200 -D /dev/ttyACM0`.

## Resolved Issues

### Blank Web Page on Load
- **Resolution:** This was a memory overflow issue caused by building the entire HTML page in a single `String` object. The web page generation has been refactored to use `AsyncResponseStream` and write directly to the response in small chunks.

### `termios.error: (25, 'Inappropriate ioctl for device')`
- **Resolution:** This error, which prevented flashing, was not a kernel or driver issue. It was caused by a serial port conflict. The `minicom` process was not terminating correctly and was holding a lock on the `/dev/ttyACM0` port, preventing `esptool.py` from accessing it.
- **Solution:** Use `lsof /dev/ttyACM0` to check for locking processes and `kill <PID>` to terminate them before attempting to upload firmware.

### Garbled Output in Serial Monitor
- **Resolution:** The serial monitor (`minicom`) was displaying garbage characters because its baud rate did not match the rate set in the firmware.
- **Solution:** Set the monitor's baud rate to `115200` to match the `monitor_speed` in `platformio.ini`.

### `invalid header: 0x8ec30f94`
- **Resolution:** This error indicated a problem with the flash configuration.
- **Solution:** Setting the `board_build.flash_mode = qio` in `platformio.ini` resolved the issue.