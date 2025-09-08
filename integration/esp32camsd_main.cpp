#include "esp_camera.h"
#include <WiFi.h>
#include "FS.h"
#include "SD_MMC.h"
#include "esp_http_server.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "ws2812.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "app_httpd.h"
#include "esp_core_dump.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"

// SD Card Pins
#define SD_MMC_CLK 39
#define SD_MMC_CMD 38
#define SD_MMC_D0  40

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// NTP Client for timekeeping (Toronto Time, EDT: UTC-4)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -14400, 60000);

unsigned long lastStatusTime = 0;
unsigned long lastLogTime = 0;
unsigned long lastHealthCheck = 0;
unsigned long bootTime = 0;
uint32_t httpRequestCount = 0;
unsigned long lastCpuTime = 0;
float cpuLoadPercent = 0.0;

void createDir(fs::FS &fs, const char * path){
    Serial.printf("DEBUG: Creating directory: %s\n", path);
    if(fs.mkdir(path)){
        Serial.printf("INFO: Directory created: %s\n", path);
    } else {
        Serial.printf("WARN: Directory creation failed or already exists: %s\n", path);
    }
}

int countFiles(fs::FS &fs, const char * dirname){
    Serial.printf("DEBUG: Counting files in: %s\n", dirname);
    File root = fs.open(dirname);
    if(!root || !root.isDirectory()){
        Serial.printf("DEBUG: Directory not accessible: %s\n", dirname);
        return 0;
    }
    int fileCount = 0;
    File file = root.openNextFile();
    while(file){
        if(!file.isDirectory()){
            fileCount++;
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    Serial.printf("DEBUG: Found %d files in %s\n", fileCount, dirname);
    return fileCount;
}

void logDirectory(fs::FS &fs, const char * dirname, int numTabs, File &logFile) {
    File root = fs.open(dirname);
    if(!root){
        logFile.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        logFile.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        for (uint8_t i=0; i<numTabs; i++) {
            logFile.print("  ");
        }
        if(file.isDirectory()){
            logFile.print(file.name());
            logFile.println("/");
            logDirectory(fs, file.path(), numTabs + 1, logFile);
        } else {
            logFile.print(file.name());
            logFile.printf(" (%d bytes)\n", file.size());
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

void logSystemHealth() {
    unsigned long currentTime = millis();
    unsigned long uptime = currentTime - bootTime;
    
    // Calculate CPU load (simplified approximation based on loop timing)
    if (lastCpuTime > 0) {
        unsigned long timeDelta = currentTime - lastCpuTime;
        // Estimate CPU load based on how much time we spend in delays vs processing
        // This is a rough approximation - ESP32 doesn't have direct CPU load APIs
        if (timeDelta > 10000) { // Only calculate if we have enough time delta
            cpuLoadPercent = 100.0 * (1.0 - (50.0 / (timeDelta / 200.0))); // Based on 50ms delay every ~200ms cycle
            if (cpuLoadPercent < 0) cpuLoadPercent = 0;
            if (cpuLoadPercent > 100) cpuLoadPercent = 100;
        }
    }
    lastCpuTime = currentTime;
    
    // Get memory information - use consistent approach for all calculations
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedHeap = totalHeap - freeHeap;
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    
    uint32_t freePsram = ESP.getFreePsram();
    uint32_t totalPsram = ESP.getPsramSize();
    uint32_t usedPsram = totalPsram - freePsram;
    
    uint32_t cpuFreq = ESP.getCpuFreqMHz();
    
    // Calculate memory usage percentages
    float heapUsage = ((float)usedHeap / totalHeap) * 100.0;
    float psramUsage = totalPsram > 0 ? ((float)usedPsram / totalPsram) * 100.0 : 0;
    
    // Check WiFi status
    String wifiStatus = "DISCONNECTED";
    int rssi = 0;
    if (WiFi.status() == WL_CONNECTED) {
        wifiStatus = "CONNECTED";
        rssi = WiFi.RSSI();
    }
    
    // Print comprehensive system health
    Serial.println("=== SYSTEM HEALTH MONITOR ===");
    Serial.printf("UPTIME: %lu ms (%lu min)\n", uptime, uptime/60000);
    Serial.printf("CPU: %d MHz (Load: %.1f%%)\n", cpuFreq, cpuLoadPercent);
    Serial.printf("HEAP: %d/%d bytes (%.1f%% used, min free: %d)\n", 
                  usedHeap, totalHeap, heapUsage, minFreeHeap);
    if (totalPsram > 0) {
        Serial.printf("PSRAM: %d/%d bytes (%.1f%% used)\n", 
                      usedPsram, totalPsram, psramUsage);
        Serial.printf("TOTAL_MEM: %d/%d bytes (%.1f%% used)\n", 
                      usedHeap + usedPsram, totalHeap + totalPsram, 
                      ((float)(usedHeap + usedPsram) / (totalHeap + totalPsram)) * 100.0);
    }
    Serial.printf("WIFI: %s (RSSI: %d dBm)\n", wifiStatus.c_str(), rssi);
    Serial.printf("HTTP_REQUESTS: %d total\n", httpRequestCount);
    
    // Check for memory pressure
    if (heapUsage > 80.0) {
        Serial.printf("WARNING: High heap usage: %.1f%%\n", heapUsage);
    }
    if (freeHeap < 50000) {
        Serial.printf("CRITICAL: Low free heap: %d bytes\n", freeHeap);
    }
    if (minFreeHeap < 30000) {
        Serial.printf("ALERT: Minimum free heap was very low: %d bytes\n", minFreeHeap);
    }
    
    // Check camera status
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        Serial.printf("CAMERA: OK (PID=0x%02x, Quality=%d, Frame=%d)\n", 
                      s->id.PID, s->status.quality, s->status.framesize);
    } else {
        Serial.println("CAMERA: ERROR - Sensor not available");
    }
    
    // Check SD card status with improved calculation
    if (SD_MMC.cardType() != CARD_NONE) {
        uint64_t totalBytes = SD_MMC.cardSize();
        uint64_t usedBytes = SD_MMC.usedBytes();
        
        // If SD_MMC.usedBytes() returns 0 (which it often does), calculate manually
        if (usedBytes == 0) {
            // Quick estimation by counting files in /camera directory
            int photoCount = countFiles(SD_MMC, "/camera");
            usedBytes = photoCount * 150000; // Rough estimate: 150KB per photo
        }
        
        Serial.printf("STORAGE: %.1f/%.1f MB used (%.1f%%)\n", 
                      usedBytes / (1024.0 * 1024.0), 
                      totalBytes / (1024.0 * 1024.0),
                      ((float)usedBytes / totalBytes) * 100.0);
    } else {
        Serial.println("STORAGE: ERROR - SD card not available");
    }
    
    Serial.println("==============================\n");
}

void logSystemDetails() {
    if (SD_MMC.cardType() == CARD_NONE) {
        Serial.println("DEBUG: No SD card available for logging");
        return;
    }

    File logFile = SD_MMC.open("/system_log.txt", FILE_WRITE);
    if (!logFile) {
        Serial.println("ERROR: Failed to open system_log.txt for writing.");
        return;
    }

    timeClient.update();
    String formattedTime = timeClient.getFormattedTime();

    logFile.print("\n--- SYSTEM LOG: ");
    logFile.print(formattedTime);
    logFile.println(" ---");

    if (WiFi.status() == WL_CONNECTED) {
        logFile.printf("WiFi Status: Connected, IP: %s, RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
        Serial.printf("DEBUG: WiFi Status - IP: %s, RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
        logFile.println("WiFi Status: Disconnected");
        Serial.println("DEBUG: WiFi Status - Disconnected");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    uint64_t usedBytes = SD_MMC.usedBytes() / (1024 * 1024);
    int photoCount = countFiles(SD_MMC, "/camera");
    logFile.printf("Storage: %llu/%llu MB used, %d photos\n", usedBytes, cardSize, photoCount);
    Serial.printf("DEBUG: Storage - %llu/%llu MB used, %d photos\n", usedBytes, cardSize, photoCount);

    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        logFile.printf("Camera: Detected PID=0x%02x, Quality=%d, Framesize=%d\n", s->id.PID, s->status.quality, s->status.framesize);
        Serial.printf("DEBUG: Camera - PID=0x%02x, Quality=%d, Framesize=%d\n", s->id.PID, s->status.quality, s->status.framesize);
    } else {
        logFile.println("Camera: Not detected");
        Serial.println("ERROR: Camera sensor not available");
    }
    
    logFile.println("\n-- File System --");
    logDirectory(SD_MMC, "/", 0, logFile);
    logFile.println("------------------------------");
    logFile.close();
    
    Serial.printf("DEBUG: System log written at %s\n", formattedTime.c_str());
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println("\n\n--- ESP32-S3 CAMERA DEBUG BOOT ---");
    
    // Initialize boot time for uptime tracking
    bootTime = millis();
    
    // Enable watchdog timer for monitoring
    esp_task_wdt_init(30, true); // 30 second watchdog
    esp_task_wdt_add(NULL); // Add current task to watchdog
    Serial.printf("DEBUG: ESP32 Chip Model: %s, Revision: %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("DEBUG: Flash Size: %d bytes, PSRAM Size: %d bytes\n", ESP.getFlashChipSize(), ESP.getPsramSize());
    
    // Check for previous crash dumps
    Serial.println("DEBUG: Checking for crash dumps...");
    if (esp_core_dump_image_check() == ESP_OK) {
        esp_core_dump_summary_t dump_summary;
        if (esp_core_dump_get_summary(&dump_summary) == ESP_OK) {
            Serial.printf("WARN: Found crash dump! Task: %s\n", dump_summary.exc_task);
        }
        Serial.println("INFO: Previous crash dump found - erasing for new session");
        esp_core_dump_image_erase();
    } else {
        Serial.println("INFO: No previous crash dump found");
    }

    Serial.println("DEBUG: Initializing WS2812 LED...");
    ws2812Init();
    ws2812SetColor(1); // Red - initializing
    
    Serial.println("DEBUG: Initializing SD Card...");
    delay(500);
    SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
    // Use Freenove official parameters: mount="/sdcard", mode1bit=true, format_if_mount_failed=true
    if(!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)){
        Serial.println("ERROR: SD Card Mount Failed!");
        ws2812SetColor(1); // Red - SD error
    } else {
        Serial.println("INFO: SD Card initialized successfully.");
        uint8_t cardType = SD_MMC.cardType();
        Serial.printf("DEBUG: SD Card Type: %s\n", 
                      (cardType == CARD_MMC) ? "MMC" : 
                      (cardType == CARD_SD) ? "SDSC" : 
                      (cardType == CARD_SDHC) ? "SDHC" : "UNKNOWN");
        uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
        Serial.printf("DEBUG: SD Card Size: %llu MB\n", cardSize);
        createDir(SD_MMC, "/camera");
        Serial.println("INFO: Camera directory ready.");
    }

    Serial.println("DEBUG: Configuring camera...");
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;  // Restore original 20MHz for OV3660
    config.frame_size = FRAMESIZE_SVGA;  // Conservative resolution
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;  // Reduce to single buffer for stability
    
    if(psramFound()){
        Serial.printf("DEBUG: PSRAM detected (%d bytes). Using conservative PSRAM settings.\n", ESP.getPsramSize());
        config.jpeg_quality = 12;  // Keep moderate quality
        config.fb_count = 1;       // Single buffer for stability
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;  // More stable mode
        config.frame_size = FRAMESIZE_SVGA;  // Conservative resolution
    } else {
        Serial.println("WARN: PSRAM not detected. Using DRAM settings.");
        config.frame_size = FRAMESIZE_CIF;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }

    Serial.println("DEBUG: Initializing camera sensor...");
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Camera init failed with error 0x%x (%s)\n", err, esp_err_to_name(err));
        ws2812SetColor(1); // Red - camera error
        return;
    }
    
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        Serial.printf("INFO: Camera initialized successfully - PID: 0x%02x\n", s->id.PID);
        Serial.printf("DEBUG: Camera settings - Quality: %d, Framesize: %d, Format: %d\n", 
                      s->status.quality, s->status.framesize, config.pixel_format);
        
        // Skip OV3660 sensor corrections temporarily to debug capture issues
        if (s->id.PID == OV3660_PID) {
            Serial.println("DEBUG: Detected OV3660 - Using minimal configuration for debugging");
            Serial.println("INFO: Skipping all sensor corrections to test basic capture");
        } else {
            Serial.printf("WARN: Unexpected sensor PID: 0x%02x (Expected OV3660: 0x%02x)\n", 
                         s->id.PID, OV3660_PID);
        }
    } else {
        Serial.println("ERROR: Failed to get camera sensor");
        ws2812SetColor(1); // Red
        return;
    }
    
    Serial.printf("DEBUG: Connecting to WiFi SSID: %s...\n", ssid);
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    Serial.println("");

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("INFO: WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("DEBUG: WiFi RSSI: %d dBm, Channel: %d\n", WiFi.RSSI(), WiFi.channel());
        ws2812SetColor(2); // Green - WiFi connected
        
        Serial.println("DEBUG: Starting NTP client...");
        timeClient.begin();
        timeClient.update();
        Serial.printf("DEBUG: Current time: %s\n", timeClient.getFormattedTime().c_str());
        
        Serial.println("DEBUG: Starting camera web servers...");
        startCameraServer();
        Serial.printf("INFO: Camera Ready! Main interface: http://%s/\n", WiFi.localIP().toString().c_str());
        Serial.printf("INFO: Stream server: http://%s:81/stream\n", WiFi.localIP().toString().c_str());
        
        ws2812SetColor(3); // Blue - servers started
        
        // Log initial system state
        logSystemDetails();
    } else {
        Serial.println("ERROR: WiFi connection failed!");
        ws2812SetColor(1); // Red - WiFi failed
    }
    
    Serial.println("DEBUG: Setup complete");
}

void loop() {
    unsigned long currentTime = millis();

    // System health monitoring every 10 seconds
    if (currentTime - lastHealthCheck >= 10000) {
        lastHealthCheck = currentTime;
        logSystemHealth();
        
        // Reset watchdog timer to prevent reset
        esp_task_wdt_reset();
    }

    // Log detailed system information every 2 minutes  
    if (currentTime - lastLogTime >= 120000) {
        lastLogTime = currentTime;
        logSystemDetails();
    }
    
    // Very short delay to allow other tasks to run
    delay(50);
}
