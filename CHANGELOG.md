# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Added a map to the web interface for location selection.
- Implemented storing and retrieving of latitude and longitude.
- Added SNTP time synchronization.
- Added fetching of sunrise and sunset times from an online API.

### Changed
- Modified the power management strategy from a deep-sleep model to a continuous-operation model to allow for uninterrupted bee counting.

### Removed
- Removed all Wokwi simulator-related files and configurations from the project.

### Added
- Implemented a full Wi-Fi provisioning web server in AP mode.
- Added Wi-Fi scanning to the provisioning portal.
- Added configuration for system name, username, and password.
- Added version and copyright splash screen to the OLED display on boot.
- Implemented OLED display support using the U8g2 library to show bee counts.
- Integrated Wi-Fi and HTTP client for continuous data reporting.

### Fixed
- Resolved the "app partition is too small" build error by correcting the flash size in `sdkconfig`.
- Fixed various compiler warnings related to missing struct initializers.
- Fixed linker errors when compiling C++ code with C libraries by adding `extern "C"` guards.

## [0.3.0] - 2025-08-12

### Added
- Integrated the TensorFlow Lite Micro component.
- Added a placeholder person detection model.
- Implemented the basic C++ structure for camera capture and TFLM inference.

### Changed
- Converted the main application file from C to C++ (`beemonitor.cpp`).

### Fixed
- Resolved all compilation errors related to component dependencies and C++ compilation.

## [0.2.0] - 2025-08-12

### Added
- Implemented the "Data Collector" firmware.
    - Initializes the camera and starts a Wi-Fi AP.
    - Streams video via a simple web server to allow for dataset capture.

## [0.1.0] - 2025-08-12

### Added
- Created `GEMINI.md` with project specifications.
- Created `TODO.md` to track project tasks.
- Created `CHANGELOG.md` to track project history.
- Installed and configured the ESP-IDF development environment.
