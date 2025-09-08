# Development Guide

## Testing with WiFi Credentials

For development and testing, you can create a `config.h` file to avoid entering WiFi credentials through the web interface every time.

### Setup:

1. Copy the example config file:
   ```bash
   cp config.h.example config.h
   ```

2. Edit `config.h` with your WiFi credentials:
   ```cpp
   #define WIFI_SSID "YourNetworkName"
   #define WIFI_PASSWORD "YourPassword"
   #define DEBUG_WIFI_ENABLED  // Uncomment this line
   ```

3. Build and flash as normal:
   ```bash
   platformio run --target upload
   ```

### How it works:

- The `config.h` file is excluded from git via `.gitignore`
- When `DEBUG_WIFI_ENABLED` is defined, the firmware will use the config.h credentials instead of stored preferences
- This bypasses the normal configuration flow and connects directly to your network
- Remove or comment out `DEBUG_WIFI_ENABLED` to return to normal operation

### Security:

- **Never commit `config.h`** - it's automatically excluded from git
- The `config.h.example` file shows the format but contains no real credentials
- Production builds should not include the config file