# Bee Counter ML Model Research Plan

This document outlines the plan for training and deploying a machine learning model to detect and count bees entering a hive.

## 1. Model Selection: FOMO (Faster Objects, More Objects)

FOMO is the ideal choice for this project due to its design for real-time object detection on resource-constrained devices like the ESP32. It is computationally efficient and capable of detecting multiple small objects in a single frame.

## 2. Platform Selection: Edge Impulse

Edge Impulse is the leading platform for developing embedded machine learning models. It provides a complete, end-to-end workflow from data collection to deployment and has first-class support for the ESP32. They are also the original developers of the FOMO algorithm.

## 3. Development Workflow

The project will follow these steps:

### Step 1: Data Collection
- **Objective:** Gather an initial dataset of images of the hive entrance.
- **Method:** Use a smartphone or other camera to capture images.
- **Image Requirements:**
    - A variety of images showing single bees, multiple bees, and no bees.
    - Images captured under different lighting conditions (sunny, cloudy, etc.).
    - A starting dataset of 50-100 images is recommended.

### Step 2: Edge Impulse Project Setup
- **Action:** Create a free account at [Edge Impulse](https://www.edgeimpulse.com/) and create a new project.

### Step 3: Data Ingestion & Labeling
- **Action:** Upload the collected images to the Edge Impulse project.
- **Action:** Use the "Labeling queue" to draw bounding boxes around every bee in every image. This is a manual but critical step.

### Step 4: Impulse Design
- **Action:** In the Edge Impulse studio, create an "Impulse" with the following configuration:
    - **Input Block:** Image data, likely 96x96 pixels.
    - **Processing Block:** "Image" (Grayscale).
    - **Learning Block:** "Object Detection (FOMO)".

### Step 5: Model Training
- **Action:** Navigate to the "FOMO" tab and begin the training process.
- **Verification:** Analyze the resulting confusion matrix and accuracy scores to determine model performance.

### Step 6: Deployment
- **Action:** From the "Deployment" tab, export the trained model as an **"Arduino library"**. This will generate a `.zip` file.

### Step 7: Integration
- **Action:** Integrate the downloaded library into the PlatformIO project.
- **Action:** Write the necessary C++ code to:
    - Initialize the camera.
    - Capture frames.
    - Pass the frames to the Edge Impulse model for inference.
    - Process the results (i.e., count the detected bees).
    - Display the camera feed and bee count on the "Monitor" page.

---

**Next Action:** The immediate next step is for the user to collect the initial image dataset as described in Step 1.
