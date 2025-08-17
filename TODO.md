# Bee Monitor - To-Do List

## Current Stage: Implementing Web UI

The project is currently focused on implementing a modern, styled web interface for the provisioning server. The basic provisioning functionality is in place, but the work to embed the themed assets (CSS, images) is ongoing and has encountered linker errors.

### Next Steps
- [ ] Resolve the linker errors related to embedding web assets (`style.css`, `bee1.jpg`, `bee2.jpg`).
- [ ] Successfully build the firmware with the complete, styled web interface.
- [ ] Continue with the plan to replace the placeholder AI model.

---

## Completed Tasks
- [x] Set up a complete provisioning web server with Wi-Fi scanning.
- [x] Added user authentication fields to the configuration.
- [x] Added a version and copyright splash screen on boot.
- [x] Resolved the "app partition is too small" build error.
- [x] Successfully build the complete firmware (without web assets).
- [x] Implement the OLED display driver and UI.
- [x] Set up ESP-IDF development environment.
- [x] Create basic project structure.
- [x] Implement camera driver.
- [x] Add TensorFlow Lite Micro component.
- [x] Add placeholder person detection model.
- [x] Integrated Wi-Fi and HTTP client for continuous data reporting.

## Future Development
- [ ] Implement the "tripwire" counting logic.
- [ ] Train a custom bee detection model.
- [ ] Replace the placeholder model with the bee detection model.
- [ ] Implement power management with the solar charger.


