# Bee Monitor - To-Do List

## Current Stage: Ready for Hardware Integration

The firmware now successfully compiles and includes support for the camera, AI model, Wi-Fi, and OLED display. The next stage is to flash the firmware onto the physical hardware and begin implementing the core bee counting logic.

### Next Steps
- [ ] Assemble and wire the hardware components.
- [ ] Flash the firmware to the board and verify the full loop (capture, infer, send, sleep, display).
- [ ] Implement the "tripwire" counting logic.
- [ ] Train a custom bee detection model.
- [ ] Replace the placeholder model with the bee detection model.

---

## Completed Tasks
- [x] Resolve the "app partition is too small" build error.
- [x] Successfully build the complete firmware.
- [x] Implement the OLED display driver and UI.
- [x] Set up ESP-IDF development environment.
- [x] Create basic project structure.
- [x] Implement camera driver.
- [x] Add TensorFlow Lite Micro component.
- [x] Add placeholder person detection model.
- [x] Integrated Wi-Fi, HTTP client, and deep-sleep logic.

## Future Development
- [ ] Implement power management with the solar charger.

