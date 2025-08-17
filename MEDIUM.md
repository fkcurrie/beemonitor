# The First Five Minutes: How to Onboard Your AI Co-Developer for Project Success

*Before you ask an AI to write a single line of code, a five-minute investment in three key files—GEMINI.md, TODO.md, and CHANGELOG.md—can mean the difference between a frustrating series of corrections and a streamlined, professional development workflow.*

---

Artificial intelligence is rapidly evolving from a novel tool into a genuine co-developer. Powerful AI assistants like Google's Gemini CLI can now write complex code, debug hardware interfaces, and configure entire projects. But like any new team member, they require a proper onboarding. You wouldn't ask a human developer to start on a new project without a briefing; you shouldn't expect an AI to either. Asking an AI to simply "vibe code" without context is a recipe for failure. To unlock its true potential, you must first provide a clear and comprehensive project brief.

I'm currently developing a solar-powered bee monitor using an ESP32-S3, a camera, and a machine learning model. The complete project, including all the documentation files discussed here, is available on [GitHub](https://github.com/fkcurrie/beemonitor). From the very beginning—before writing any C++ or configuring a single build file—I established a simple documentation framework. This "setup-first" methodology ensures my AI collaborator has the necessary context to function as a true partner, not just a code generator.

This framework consists of three essential files that should be created in the first five minutes of any new project.

### 1. [`GEMINI.md`](https://github.com/fkcurrie/beemonitor/blob/main/GEMINI.md): The AI's Project Charter

The `GEMINI.md` file is the single most critical document in your project. It is the AI's project charter, its source of truth. Creating this file *first* ensures that every subsequent action the AI takes is grounded in the specific realities of your project. It eliminates guesswork and prevents the generation of generic, ill-fitting code that wastes valuable time.

For my Bee Monitor, the `GEMINI.md` immediately establishes the technical landscape:

**General Instructions:**
```markdown
## 1. General Instructions

*   **Operational Note:** Do not apologize. When an error occurs, focus directly on analyzing the problem and presenting a solution.
*   **Prioritize Research:** Before starting any investigation, creating new code, or answering technical questions, **always** use the `google_web_search` tool.
*   **Verify with Official Documentation:** The ESP-IDF and ESP32-S3 documentation from Espressif are the primary sources of truth.
```
This section sets the operational ground rules for the AI. It's akin to setting team communication standards, ensuring the AI's behavior—like its focus on solutions over apologies—aligns with my preferences and improves the efficiency of our collaboration.

**Hardware Specifications:**
```markdown
## 4. Hardware Specifications

*   **Microcontroller:** ESP32-S3 (Dual-core Xtensa LX7, 240 MHz, 512KB SRAM)
*   **Development Board:** ESP32-S3-DevKitC-1
*   **Connected Peripherals:**
    *   **Camera:** OV5640 or similar (via DVP interface).
    *   **Display:** 0.96" I2C OLED Display (SSD306).
```
In embedded systems, specifics are everything. This section prevents the AI from hallucinating incorrect pin numbers or using drivers for the wrong hardware—a common and frustrating problem that can lead to hours of debugging.

**Development Environment:**
```markdown
## 5. Development Environment & Toolchain

*   **Framework:** ESP-IDF
*   **Key Commands:**
    *   **Build:** `idf.py build`
    *   **Flash:** `idf.py -p [Your Serial Port] flash`
```
This ensures the AI uses the correct build commands and understands the project's ecosystem. It prevents it from suggesting commands for the wrong build system (e.g., `make` instead of `idf.py`) or using Arduino conventions in an ESP-IDF project.

### 2. [`TODO.md`](https://github.com/fkcurrie/beemonitor/blob/main/TODO.md): The Strategic Roadmap

A `TODO.md` file, when created at the project's inception, is more than a simple checklist; it's a strategic plan. It forces you to think through the development arc, break down the work into logical, sequential tasks, and establish clear priorities. This provides a roadmap that both you and your AI can follow, preventing "yak shaving" and keeping the project focused on the most valuable features first.

From day one, my `TODO.md` laid out the path from setup to deployment:

```markdown
# Bee Monitor - To-Do List

### Next Steps
- [ ] Set up ESP-IDF development environment.
- [ ] Create basic project structure.
- [ ] Implement camera driver.
- [ ] Add TensorFlow Lite Micro component.
- [ ] Implement the OLED display driver and UI.
- [ ] Set up a complete provisioning web server.
- [ ] Train a custom bee detection model.

### Completed Tasks
(empty)
```
With this roadmap in place, I can start a development session with a simple, powerful instruction: "Let's work on the next item in the `TODO.md`." As tasks are completed and moved to the "Completed Tasks" section, it provides a satisfying sense of progress and keeps the AI's context up-to-date on what has already been accomplished.

### 3. [`CHANGELOG.md`](https://github.com/fkcurrie/beemonitor/blob/main/CHANGELOG.md): The Project's Professional Ledger

Creating a `CHANGELOG.md` from the beginning establishes a professional tone for the project. It signals that this is a serious endeavor where changes are tracked, and progress is documented. For long-term projects, it's an invaluable tool for debugging, as it provides a clear timeline of when specific changes were introduced. If a bug appears, the changelog can help narrow down the set of commits that might have caused it.

This file is based on the [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) standard, which provides a clear and consistent format for logging changes. It also adheres to [Semantic Versioning (SemVer)](https://semver.org/), where version numbers (e.g., `1.2.3`) convey meaning about the nature of the changes.

My `CHANGELOG.md` was initialized with the project's birth:

```markdown
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- New features that are not yet part of a release.

## [0.1.0] - 2025-08-12

### Added
- Created `GEMINI.md` with project specifications.
- Created `TODO.md` to track project tasks.
- Created `CHANGELOG.md` to track project history.
- Installed and configured the ESP-IDF development environment.
```
By instructing the AI to update the `[Unreleased]` section after every significant milestone, I ensure the project's evolution is always clearly documented. When it's time for a new release, I can simply change `[Unreleased]` to the new version number and date.

### Conclusion: Onboard Your AI, Then Code

The temptation to jump straight into coding is strong, but when working with an AI co-developer, this is a mistake. The five minutes it takes to create `GEMINI.md`, `TODO.md`, and `CHANGELOG.md` is the most valuable investment you can make. This "setup-first" approach is an act of professional discipline that pays for itself almost immediately.

It provides the essential context, direction, and professional structure that allows your AI to perform at its best. It elevates the AI from a simple tool to a true collaborator, leading to a faster, more efficient, and ultimately more successful development experience.

---

### Key Files Mentioned in This Article:
*   **Project Repository:** [https://github.com/fkcurrie/beemonitor](https://github.com/fkcurrie/beemonitor)
*   **AI Project Brief:** [`GEMINI.md`](https://github.com/fkcurrie/beemonitor/blob/main/GEMINI.md)
*   **Project Roadmap:** [`TODO.md`](https://github.com/fkcurrie/beemonitor/blob/main/TODO.md)
*   **Development History:** [`CHANGELOG.md`](https://github.com/fkcurrie/beemonitor/blob/main/CHANGELOG.md)