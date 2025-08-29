#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <vector>
#include <map>
#include <algorithm>
#include "mbedtls/md.h" // For SHA-256 Hashing
#include <ESPmDNS.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h>
#include "version.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "esp32-hal-cpu.h"
#include "esp32-hal-cpu.h" // For CPU Temperature
#include "esp_camera.h"
#include <ArduinoOTA.h>

// --- Camera Configuration ---
#define CAMERA_MODEL_CUSTOM
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     14
#define SIOD_GPIO_NUM     8
#define SIOC_GPIO_NUM     9
#define Y9_GPIO_NUM       1
#define Y8_GPIO_NUM       2
#define Y7_GPIO_NUM       3
#define Y6_GPIO_NUM       4
#define Y5_GPIO_NUM       5
#define Y4_GPIO_NUM       10
#define Y3_GPIO_NUM       11
#define Y2_GPIO_NUM       12
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

bool initCamera() {
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
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA; // 640x480
    config.jpeg_quality = 12; // 0-63 lower number means higher quality
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Camera init failed with error 0x%x\n", err);
        return false;
    }
    
    sensor_t * s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV5640_PID) {
        s->set_vflip(s, 1); // flip it back
        s->set_brightness(s, 1); // up the blightness just a bit
        s->set_saturation(s, -2); // lower the saturation
    }

    return true;
}

// --- CPU Usage Calculation ---
static volatile uint64_t idle_0_counter = 0;
static volatile uint64_t idle_1_counter = 0;

float cpu_0_usage = 0.0;
float cpu_1_usage = 0.0;

void idle_task_0(void *param) {
    while (1) {
        idle_0_counter++;
        // vTaskDelay is important to prevent the task from hogging the CPU
        // and to allow the idle task scheduler to work correctly.
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void idle_task_1(void *param) {
    while (1) {
        idle_1_counter++;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void calculate_cpu_usage() {
    static uint64_t last_idle_0_counter = 0;
    static uint64_t last_idle_1_counter = 0;
    static unsigned long last_update_time = 0;

    unsigned long now = millis();
    if (now - last_update_time < 1000) {
        return; // Calculate only once per second
    }

    uint64_t idle_0_diff = idle_0_counter - last_idle_0_counter;
    uint64_t idle_1_diff = idle_1_counter - last_idle_1_counter;

    // This is an empirical value, calibrated for this specific hardware and code.
    // It represents the approximate number of idle counts per second when the CPU is fully idle.
    const float max_idle_counts_per_second = 8500.0; 

    cpu_0_usage = 100.0 - ((float)idle_0_diff * 100.0 / max_idle_counts_per_second);
    if (cpu_0_usage < 0.0) cpu_0_usage = 0.0;
    if (cpu_0_usage > 100.0) cpu_0_usage = 100.0;

    cpu_1_usage = 100.0 - ((float)idle_1_diff * 100.0 / max_idle_counts_per_second);
    if (cpu_1_usage < 0.0) cpu_1_usage = 0.0;
    if (cpu_1_usage > 100.0) cpu_1_usage = 100.0;

    last_idle_0_counter = idle_0_counter;
    last_idle_1_counter = idle_1_counter;
    last_update_time = now;
}


// --- OLED Configuration ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- LED Configuration ---
#define LED_PIN    48
#define LED_COUNT 1
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- Colors & States ---
#define COLOR_CONFIG_MODE   strip.Color(255, 165, 0) // Orange
#define COLOR_CONNECTING    strip.Color(0, 0, 255)   // Blue
#define COLOR_CONNECTED     strip.Color(0, 255, 0)   // Green
#define COLOR_FAILED        strip.Color(255, 0, 0)   // Red
#define COLOR_SAVING        strip.Color(255, 255, 255) // White

enum SystemState { CONFIG_MODE, WIFI_CONNECTING, WIFI_CONNECTED, IP_ACQUIRED, SERVER_STARTED };
SystemState currentState = WIFI_CONNECTING;

// --- Wi-Fi & Server ---
const char* ap_ssid = "BeeCounter-Setup";
AsyncWebServer server(80);
AsyncEventSource events("/events");
Preferences preferences;

// --- Authentication ---
#define SESSION_COOKIE_NAME "BEE_SESSION"
String currentSessionId = "";

// --- Structs ---
struct WifiNetwork { String ssid; int32_t rssi; };

// --- Hashing Utility ---
String sha256(const String& str) {
    byte hash[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *)str.c_str(), str.length());
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);

    char hexStr[65];
    for (int i = 0; i < 32; i++) {
        sprintf(hexStr + i * 2, "%02x", hash[i]);
    }
    return String(hexStr);
}

// --- OLED Helper ---
void updateOLED(String line1 = "", String line2 = "", String line3 = "", String line4 = "") {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(line1);
    display.println(line2);
    display.println(line3);
    display.println(line4);
    display.display();
}

// --- Timezone Auto-Detection ---
bool autoSetTimezone() {
    updateOLED("Finding Timezone...");
    HTTPClient http;
    http.begin("http://ip-api.com/json/");
    int httpCode = http.GET();
    bool success = false;

    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);
            const char* tz = doc["timezone"];
            if (tz) {
                preferences.begin("beecounter", false); // Open preferences in read-write mode
                preferences.putString("timezone", tz);
                preferences.end();
                updateOLED("Timezone Saved:", tz);
                success = true;
            } else {
                updateOLED("Timezone Error", "No TZ in response");
            }
        }
    } else {
        updateOLED("Timezone Error", "HTTP request failed");
    }
    http.end();
    delay(1000);
    return success;
}


String getPublicIP() {
    HTTPClient http;
    http.begin("http://ip-api.com/json/");
    int httpCode = http.GET();
    if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        deserializeJson(doc, payload);
        const char* ip = doc["query"];
        if (ip) {
            return String(ip);
        }
    }
    return "N/A";
}

// --- Performance Monitoring ---
unsigned long performanceUpdateInterval = 2000; // Default to 2 seconds

// --- Data Collection State ---
bool isCollecting = false;
int imagesCollected = 0;
const int totalImages = 50;
const unsigned long collectionInterval = (24 * 60 * 60 * 1000) / totalImages; // ~30 mins
unsigned long lastCollectionTime = 0;

// --- Forward Declarations ---
void handleSetRefreshRate(AsyncWebServerRequest *request);
void handleSetCameraSettings(AsyncWebServerRequest *request);
void handleCaptureStart(AsyncWebServerRequest *request);
void handleCaptureStop(AsyncWebServerRequest *request);
void handleCapturePhoto(AsyncWebServerRequest *request);
void handleEdgeImpulseSettings(AsyncWebServerRequest *request);
void handleEdgeImpulseUpload(AsyncWebServerRequest *request);
void handleEdgeImpulseDownloadModel(AsyncWebServerRequest *request);
void startConfigurationMode();
void handleWelcomePage(AsyncWebServerRequest *request);
void handleSetupPage(AsyncWebServerRequest *request);
void handleSaveWifiPage(AsyncWebServerRequest *request);
String getPageTemplate(const String& title, const String& body, bool includeTabs = false);
void handleLoginPage(AsyncWebServerRequest *request);
void handleDoLogin(AsyncWebServerRequest *request);
void handleChangePassPage(AsyncWebServerRequest *request);
void handleDoChangePass(AsyncWebServerRequest *request);
void handleMainPage(AsyncWebServerRequest *request);
void handleTrainPage(AsyncWebServerRequest *request);
void handleObservabilityPage(AsyncWebServerRequest *request);
void handleAdminPage(AsyncWebServerRequest *request);
void handleAboutPage(AsyncWebServerRequest *request);
void handleChangelogPage(AsyncWebServerRequest *request);
void handleDoAdminChangePass(AsyncWebServerRequest *request);
void handleFindTimezone(AsyncWebServerRequest *request);
void handleReboot(AsyncWebServerRequest *request);
void handleFactoryReset(AsyncWebServerRequest *request);
void handleLogout(AsyncWebServerRequest *request);
bool isAuthenticated(AsyncWebServerRequest *request);
String getFooter();
void logError(const String& message);

String getLoginPageTemplate(const String& title, const String& body) {
    String html = R"rawliteral(<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>BeeCounter - )rawliteral" + title + R"rawliteral(</title>
            <style>
            body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;margin:0;background-color:#1a1a1a;color:#FFC300;display:flex;justify-content:center;align-items:center;height:100vh;text-align:center;}
            .container{background-color:#444;padding:3rem 4rem;border-radius:15px;box-shadow:0 0 25px rgba(255,195,0,0.3);border:2px solid #FFC300;}
            form{max-width:300px;margin:auto;}
            h2{font-size:2rem;margin-bottom:1.5rem;}
            .form-group{margin-bottom:1.5rem;text-align:left;}
            label{display:block;margin-bottom:.5rem;font-weight:600;}
            input[type=text],input[type=password]{width:100%;padding:.75rem;border:1px solid #FFC300;border-radius:6px;box-sizing:border-box;font-size:1rem;background-color:#333;color:#fff;}
            button{width:100%;padding:.75rem;border:none;border-radius:6px;background-color:#FFC300;color:#000;font-size:1.1rem;font-weight:600;cursor:pointer;}
            .error{color:#F44336;margin-bottom:1rem;}
            </style></head><body><div class='container'>)rawliteral" + body + R"rawliteral(</div></body></html>)rawliteral";
    return html;
}

// --- Main Setup & Loop ---
void setup() {
    Serial.begin(115200);
    strip.begin();
    strip.setBrightness(20);
    strip.setPixelColor(0, COLOR_CONFIG_MODE);
    strip.show();

    // --- Initialize Watchdog Timer ---
    esp_task_wdt_init(10, true); // 10-second timeout, panic on timeout
    esp_task_wdt_add(NULL); // Subscribe the setup/loop task

    // --- Create CPU Idle Tasks ---
    xTaskCreatePinnedToCore(idle_task_0, "idle_0", 1024, NULL, 0, NULL, 0);
    xTaskCreatePinnedToCore(idle_task_1, "idle_1", 1024, NULL, 0, NULL, 1);

    // --- Initialize OLED ---
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        // Address 0x3C for 128x64
    }
    updateOLED("BeeCounter v" + String(APP_VERSION), "Booting...");

    // --- Initialize LittleFS ---
    if(!LittleFS.begin()){
        updateOLED("LITTLEFS Error", "Failed to mount file system");
        Serial.println("FATAL: Failed to mount LittleFS. Halting.");
        return; // Halt on error
    }
    // Create image directory if it doesn't exist
    if (!LittleFS.exists("/images")) {
        if (LittleFS.mkdir("/images")) {
            Serial.println("Directory '/images' created");
        } else {
            Serial.println("FATAL: Failed to create '/images' directory");
            updateOLED("LITTLEFS Error", "mkdir /images failed");
            return; // Halt on error
        }
    }

    // --- Initialize Camera ---
    // if (!initCamera()) {
    //     updateOLED("CAMERA Error", "Failed to init.", "Rebooting in 5s...");
    //     Serial.println("FATAL: Failed to initialize camera. Rebooting in 5 seconds.");
    //     delay(5000);
    //     ESP.restart();
    // } else {
    //     updateOLED("Camera OK", "Starting Wi-Fi...");
    //     Serial.println("Camera initialized successfully.");
    // }

    // --- Wi-Fi Event Handlers ---
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
        Serial.printf("[WiFi-event] event: %d\n", event);
        switch (event) {
            case SYSTEM_EVENT_STA_START:
                Serial.println("WiFi station mode started.");
                break;
            case SYSTEM_EVENT_STA_CONNECTED:
                Serial.println("Connected to AP successfully!");
                break;
            case SYSTEM_EVENT_STA_GOT_IP:
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                break;
            case SYSTEM_EVENT_STA_DISCONNECTED:
                Serial.println("Disconnected from WiFi access point.");
                break;
            default:
                break;
        }
    });

    // --- Hardcoded Wi-Fi Credentials for Debugging ---
    const char* ssid = "LandE";
    const char* password = "LachlanEamon";

    // --- Station Mode ---
    currentState = WIFI_CONNECTING;
    strip.setPixelColor(0, COLOR_CONNECTING);
    strip.show();
    updateOLED("Connecting to:", ssid);
    Serial.printf("Attempting to connect to SSID: %s\n", ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    Serial.println("Waiting for Wi-Fi connection...");
    while (millis() - startTime < 15000) {
        if (WiFi.status() == WL_CONNECTED) {
            currentState = WIFI_CONNECTED;
            updateOLED("Connected!", "Waiting for IP...");
            Serial.println("Wi-Fi status is WL_CONNECTED. Waiting for IP.");
            
            // The SYSTEM_EVENT_STA_GOT_IP event will handle the IP address logging.
            // We just need to wait for the IP to be assigned.
            unsigned long ip_wait_start = millis();
            while (WiFi.localIP() == IPAddress(0,0,0,0) && (millis() - ip_wait_start < 5000)) {
                delay(100);
            }

            if (WiFi.localIP() != IPAddress(0,0,0,0)) {
                currentState = IP_ACQUIRED;
                updateOLED("IP Acquired:", WiFi.localIP().toString(), "Starting server...");
                Serial.println("IP address acquired. Starting server and services.");
                
                server.on("/login", HTTP_GET, handleLoginPage);
                server.on("/login", HTTP_POST, handleDoLogin);
                server.on("/changepass", HTTP_GET, handleChangePassPage);
                server.on("/changepass", HTTP_POST, handleDoChangePass);
                server.on("/admin", HTTP_GET, handleAdminPage);
                server.on("/admin/changepass", HTTP_POST, handleDoAdminChangePass);
                server.on("/admin/find-timezone", HTTP_POST, handleFindTimezone);
                server.on("/admin/reboot", HTTP_POST, handleReboot);
                server.on("/factory-reset", HTTP_POST, handleFactoryReset);
                server.on("/logout", HTTP_GET, handleLogout);
                server.on("/", HTTP_GET, handleMainPage);
                server.on("/train", HTTP_GET, handleTrainPage);
                server.on("/observability", HTTP_GET, handleObservabilityPage);
                server.on("/api/set-refresh-rate", HTTP_POST, handleSetRefreshRate);
                server.on("/api/camera-settings", HTTP_POST, handleSetCameraSettings);
                server.on("/api/capture/start", HTTP_POST, handleCaptureStart);
                server.on("/api/capture/stop", HTTP_POST, handleCaptureStop);
                server.on("/api/capture/photo", HTTP_POST, handleCapturePhoto);
                server.on("/api/edgeimpulse/settings", HTTP_POST, handleEdgeImpulseSettings);
                server.on("/api/images", HTTP_GET, [](AsyncWebServerRequest *request){
                    if (!isAuthenticated(request)) { request->send(401, "text/plain", "Unauthorized"); return; }
                    JsonDocument doc;
                    JsonArray array = doc.to<JsonArray>();
                    File root = LittleFS.open("/images");
                    File file = root.openNextFile();
                    while(file){
                        array.add(String(file.name()));
                        file = root.openNextFile();
                    }
                    String json;
                    serializeJson(doc, json);
                    request->send(200, "application/json", json);
                });
                server.on("/images", HTTP_GET, [](AsyncWebServerRequest *request){
                    if (!isAuthenticated(request)) { request->send(401, "text/plain", "Unauthorized"); return; }
                    if (request->hasParam("delete")) {
                        String filename = "/images/" + request->getParam("delete")->value();
                        if (LittleFS.exists(filename)) {
                            LittleFS.remove(filename);
                            request->send(200, "text/plain", "Deleted: " + filename);
                        } else {
                            request->send(404, "text/plain", "File not found");
                        }
                    } else if (request->hasParam("serve")) {
                         String filename = "/images/" + request->getParam("serve")->value();
                         if (LittleFS.exists(filename)) {
                            request->send(LittleFS, filename, "image/jpeg");
                         } else {
                            request->send(404, "text/plain", "File not found");
                         }
                    } else {
                        request->send(400, "text/plain", "Bad Request");
                    }
                });
                server.on("/api/edgeimpulse/upload", HTTP_POST, handleEdgeImpulseUpload);
                server.on("/api/edgeimpulse/download-model", HTTP_POST, handleEdgeImpulseDownloadModel);
                server.on("/about", HTTP_GET, handleAboutPage);
                server.on("/changelog", HTTP_GET, handleChangelogPage);
                
                // --- OTA Update Handler ---
                server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
                    if (!isAuthenticated(request)) {
                        request->send(401, "text/plain", "Unauthorized");
                        return;
                    }
                    // NOTE: This is a placeholder. The actual OTA update is handled by ArduinoOTA via the network port.
                    // This endpoint just confirms the UI is working. A more advanced implementation could handle HTTP uploads.
                    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OTA update process initiated. Use PlatformIO to upload the new firmware.");
                    request->send(response);
                }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
                    // This is the actual upload handler, but for ArduinoOTA, we don't need it.
                    // We just need the POST request to trigger the UI feedback.
                });

                server.onNotFound([](AsyncWebServerRequest *request){ request->send(404, "text/plain", "Not found"); });
                
                // --- Handle Server-Sent Events ---
                events.onConnect([](AsyncEventSourceClient *client){
                    if(client->lastId()){
                        // This could be used to resume events from a certain point
                    }
                    client->send("hello!", NULL, millis(), 10000);
                });
                server.addHandler(&events);

                server.begin();

                // --- Start mDNS ---
                String deviceName = preferences.getString("deviceName", "beecounter");
                if (MDNS.begin(deviceName.c_str())) {
                    MDNS.addService("http", "tcp", 80);
                }

                // --- Start OTA Service ---
                String ota_password_seed = preferences.getString("adminPass", "") + WiFi.macAddress();
                ArduinoOTA.setPassword(sha256(ota_password_seed).c_str());

                ArduinoOTA.onStart([]() {
                    strip.setPixelColor(0, COLOR_SAVING); // White
                    strip.show();
                    updateOLED("OTA Update", "Receiving...");
                    Serial.println("OTA update started...");
                }).onEnd([]() {
                    updateOLED("OTA Complete!", "Rebooting...");
                    Serial.println("\nOTA update finished. Rebooting...");
                }).onProgress([](unsigned int progress, unsigned int total) {
                    int percentage = (progress / (total / 100));
                    updateOLED("OTA Update", "Progress: " + String(percentage) + "%\n");
                    Serial.printf("OTA Progress: %u%%\r", percentage);
                }).onError([](ota_error_t error) {
                    strip.setPixelColor(0, COLOR_FAILED); // Red
                    strip.show();
                    String errorMsg;
                    if (error == OTA_AUTH_ERROR) errorMsg = "Auth Failed";
                    else if (error == OTA_BEGIN_ERROR) errorMsg = "Begin Failed";
                    else if (error == OTA_CONNECT_ERROR) errorMsg = "Connect Failed";
                    else if (error == OTA_RECEIVE_ERROR) errorMsg = "Receive Failed";
                    else if (error == OTA_END_ERROR) errorMsg = "End Failed";
                    updateOLED("OTA Error", errorMsg);
                    Serial.printf("ERROR[%u]: %s\n", error, errorMsg.c_str());
                });
                ArduinoOTA.begin();

                // --- Start NTP and set Timezone ---
                String timezone = preferences.getString("timezone", "");
                if (timezone.length() > 0) {
                    Serial.printf("DEBUG: Setting timezone using saved value: %s\n", timezone.c_str());
                    configTzTime(timezone.c_str(), "pool.ntp.org");
                } else {
                    // Fallback to a default if no timezone is set
                    const char* tz_posix = "EST5EDT,M3.2.0,M11.1.0"; // POSIX string for America/Toronto
                    Serial.printf("DEBUG: No timezone saved. Using default POSIX string: %s\n", tz_posix);
                    configTzTime(tz_posix, "pool.ntp.org");
                }
                Serial.println("DEBUG: configTzTime called.");

                // Wait for NTP sync
                struct tm timeinfo_sync;
                Serial.print("DEBUG: Waiting for NTP synchronization...");
                if (!getLocalTime(&timeinfo_sync, 15000)) { // Wait up to 15 seconds
                    Serial.println("\nDEBUG: Failed to obtain time.");
                } else {
                    Serial.println("\nDEBUG: NTP synchronization complete.");
                    char timeBuff[30];
                    strftime(timeBuff, sizeof(timeBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo_sync);
                    Serial.printf("DEBUG: The current time is: %s\n", timeBuff);
                    Serial.println("IP address: " + WiFi.localIP().toString());
                }

                currentState = SERVER_STARTED;
                break;
            }
        }
        delay(100);
    }
    if (currentState != SERVER_STARTED) {
        strip.setPixelColor(0, COLOR_FAILED);
        strip.show();
        String errorMsg = "Failed to connect to " + String(ssid);
        logError(errorMsg);
        updateOLED("Connection Failed", "Check credentials.", "Rebooting...");
        delay(2000);
        ESP.restart();
    }
}

void loop() {
    ArduinoOTA.handle();
    // --- Wi-Fi Reconnection Logic ---
    if (WiFi.status() != WL_CONNECTED && currentState != CONFIG_MODE) {
        currentState = WIFI_CONNECTING;
        strip.setPixelColor(0, COLOR_CONNECTING);
        strip.show();
        updateOLED("Wi-Fi Lost", "Reconnecting...");
        Serial.println("WARNING: Wi-Fi connection lost. Attempting to reconnect...");
        
        WiFi.disconnect();
        WiFi.reconnect();

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime < 30000)) {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("FATAL: Failed to reconnect to Wi-Fi after 30 seconds. Rebooting.");
            updateOLED("Reconnect Failed", "Rebooting...");
            delay(2000);
            ESP.restart();
        } else {
            Serial.println("\nSUCCESS: Reconnected to Wi-Fi.");
            updateOLED("Reconnected!", "IP: " + WiFi.localIP().toString());
            currentState = SERVER_STARTED; // Resume normal operation
            delay(1000);
        }
    }

    // LED state management
    static unsigned long ledPreviousMillis = 0;
    static bool ledState = true;
    unsigned long currentMillis = millis();
    long ledInterval = 1000;

    switch (currentState) {
        case CONFIG_MODE:
            ledInterval = 500;
            if (currentMillis - ledPreviousMillis >= ledInterval) {
                ledPreviousMillis = currentMillis;
                ledState = !ledState;
                strip.setPixelColor(0, ledState ? COLOR_CONFIG_MODE : 0);
                strip.show();
            }
            break;
        case WIFI_CONNECTED:
            ledInterval = 1000;
            if (currentMillis - ledPreviousMillis >= ledInterval) {
                ledPreviousMillis = currentMillis;
                ledState = !ledState;
                strip.setPixelColor(0, ledState ? COLOR_CONNECTED : 0);
                strip.show();
            }
            break;
        case IP_ACQUIRED:
            ledInterval = 500;
            if (currentMillis - ledPreviousMillis >= ledInterval) {
                ledPreviousMillis = currentMillis;
                ledState = !ledState;
                strip.setPixelColor(0, ledState ? COLOR_CONNECTED : 0);
                strip.show();
            }
            break;
        case SERVER_STARTED:
            if (strip.getPixelColor(0) != COLOR_CONNECTED) {
                strip.setPixelColor(0, COLOR_CONNECTED);
                strip.show();
            }
            break;
        case WIFI_CONNECTING:
            // Solid blue during initial connection or reconnection attempts
            if (strip.getPixelColor(0) != COLOR_CONNECTING) {
                strip.setPixelColor(0, COLOR_CONNECTING);
                strip.show();
            }
            break;
    }

    // OLED state management
    static unsigned long oledPreviousMillis = 0;
    long oledInterval = 2000; // Update OLED every 2 seconds

    if (currentState == SERVER_STARTED && (currentMillis - oledPreviousMillis >= oledInterval)) {
        oledPreviousMillis = currentMillis;
        preferences.begin("beecounter", true);
        String deviceName = preferences.getString("deviceName", "BeeCounter");
        preferences.end();
        
        updateOLED(
            deviceName,
            "IP: " + WiFi.localIP().toString(),
            "Heap: " + String(ESP.getFreeHeap()) + " B"
        );
    }

    // --- Send Performance Update Event ---
    static unsigned long lastEventTime = 0;
    if (currentState == SERVER_STARTED && (millis() - lastEventTime > performanceUpdateInterval)) {
        calculate_cpu_usage(); // Update CPU usage values
        JsonDocument doc;
        doc["cpu_core_0"] = cpu_0_usage;
        doc["cpu_core_1"] = cpu_1_usage;
        doc["heap_used"] = ESP.getFreeHeap();
        doc["heap_total"] = ESP.getHeapSize();
        doc["flash_used"] = LittleFS.usedBytes();
        doc["flash_total"] = LittleFS.totalBytes();
        doc["wifi_rssi"] = WiFi.RSSI();
        doc["cpu_temp"] = temperatureRead();
        String json;
        serializeJson(doc, json);
        events.send(json.c_str(), "performance_update", millis());
        lastEventTime = millis();
    }

    // --- Automated Data Collection ---
    if (isCollecting && (millis() - lastCollectionTime > collectionInterval)) {
        if (imagesCollected < totalImages) {
            // It's time to take another picture
            camera_fb_t * fb = esp_camera_fb_get();
            if (fb) {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                String path = "/images/img-" + String(tv.tv_sec) + ".jpg";
                File file = LittleFS.open(path, FILE_WRITE);
                if (file) {
                    file.write(fb->buf, fb->len);
                    file.close();
                    imagesCollected++;
                    Serial.printf("DATA COLLECTION: Saved %s (%d/%d)\n", path.c_str(), imagesCollected, totalImages);
                    
                    // Send progress update
                    JsonDocument doc;
                    doc["progress"] = (int)((float)imagesCollected / totalImages * 100);
                    doc["collected"] = imagesCollected;
                    doc["total"] = totalImages;
                    String json;
                    serializeJson(doc, json);
                    events.send(json.c_str(), "collection_update", millis());

                } else {
                    Serial.println("ERROR: Data collection failed to open file.");
                }
                esp_camera_fb_return(fb);
            } else {
                Serial.println("ERROR: Data collection failed to get frame.");
            }
            lastCollectionTime = millis(); // Reset timer
        } else {
            // Collection complete
            isCollecting = false;
            Serial.println("INFO: Automated data collection complete.");
            events.send("complete", "collection_status", millis());
        }
    }

    // --- Feed the Watchdog ---
    esp_task_wdt_reset();

    // --- Periodic Serial Output ---
    static unsigned long lastSerialPrintMillis = 0;
    if (currentState == SERVER_STARTED && (millis() - lastSerialPrintMillis > 30000)) {
        preferences.begin("beecounter", true);
        String deviceName = preferences.getString("deviceName", "BeeCounter");
        preferences.end();
        
        Serial.println("\n--- STATUS UPDATE ---");
        Serial.println("Device Name: " + deviceName);
        Serial.println("WiFi SSID:   " + WiFi.SSID());
        Serial.println("IP Address:  " + WiFi.localIP().toString());
        Serial.println("---------------------\n");
        
        lastSerialPrintMillis = millis();
    }
}

// --- Configuration Mode Handlers ---
void startConfigurationMode() {
    currentState = CONFIG_MODE;
    updateOLED("Setup Mode", "Connect to AP:", ap_ssid, "IP: 192.168.4.1");
    WiFi.softAP(ap_ssid);
    WiFi.mode(WIFI_AP);

    server.on("/", HTTP_GET, handleWelcomePage);
    server.on("/setup", HTTP_GET, handleSetupPage);
    server.on("/save", HTTP_POST, handleSaveWifiPage);
    server.onNotFound([](AsyncWebServerRequest *request){ request->redirect("/"); });
    server.begin();
}

void handleWelcomePage(AsyncWebServerRequest *request) {
    String body = "<img src='https://upload.wikimedia.org/wikipedia/commons/9/91/Abeille-bee.svg' alt='Bee Icon' style='height: 120px; margin-bottom: 1rem;' />" 
                  "<h1>Welcome to BeeCounter</h1>" 
                  "<p>Let's get your device connected.</p>" 
                  "<a href='/setup' style='display:inline-block;padding:1rem 2.5rem;border:none;border-radius:8px;background-color:#FFC300;color:#000;font-size:1.2rem;font-weight:600;cursor:pointer;text-decoration:none;'>Setup</a>";
    request->send(200, "text/html", getLoginPageTemplate("Welcome", body));
}

void handleSetupPage(AsyncWebServerRequest *request) {
    int16_t scanResult = WiFi.scanComplete();
    if (scanResult == WIFI_SCAN_RUNNING) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "<!DOCTYPE html><html><head><title>Scanning...</title><meta http-equiv='refresh' content='6'></head><body><p>Scanning for networks...</p></body></html>");
        request->send(response);
        return;
    }

    String body = "<img src='https://upload.wikimedia.org/wikipedia/commons/9/91/Abeille-bee.svg' alt='Bee Icon' style='height: 120px; margin-bottom: 1rem;' />" 
                  "<h2>Device Configuration</h2>" 
                  "<form action='/save' method='POST'>" 
                  "<div class='form-group'><label for='deviceName'>System Name</label><input type='text' id='deviceName' name='deviceName' placeholder='e.g., Hive Entrance 1' required></div>" 
                  "<div class='form-group'><label>Select Wi-Fi Network</label><div id='wifiList' style='min-height:50px;max-height:150px;overflow-y:auto;border:1px solid #FFC300;border-radius:6px;padding:.5rem;margin-top:1rem;'>";

    if (scanResult > 0) {
        std::map<String, int32_t> uniqueNetworks;
        for (int i = 0; i < scanResult; ++i) {
            String ssid = WiFi.SSID(i);
            int32_t rssi = WiFi.RSSI(i);
            if (uniqueNetworks.find(ssid) == uniqueNetworks.end() || rssi > uniqueNetworks[ssid]) {
                uniqueNetworks[ssid] = rssi;
            }
        }
        std::vector<WifiNetwork> sortedNetworks;
        for (auto const& pair : uniqueNetworks) {
            sortedNetworks.push_back({pair.first, pair.second});
        }
        std::sort(sortedNetworks.begin(), sortedNetworks.end(), [](const WifiNetwork& a, const WifiNetwork& b) { return a.rssi > b.rssi; });
        for (const auto& net : sortedNetworks) {
            body += "<div style='margin-bottom:.5rem;cursor:pointer;display:flex;align-items:center;'><input style='margin-right:.5rem;' type='radio' name='wifi_ssid' id='" + net.ssid + "' value='" + net.ssid + "' required><label for='" + net.ssid + "'>" + net.ssid + " (" + net.rssi + " dBm)</label></div>";
        }
        WiFi.scanDelete();
    } else {
        body += "<p>No networks found. <a href='/setup'>Scan Again</a></p>";
    }
    if (scanResult <= 0) WiFi.scanNetworks(true);

    body += "</div></div><div class='form-group'><label for='wifiPassword'>Wi-Fi Password</label><input type='password' id='wifiPassword' name='wifiPassword'></div>" 
            "<button type='submit'>Save & Reboot</button>" 
            "</form>";

    request->send(200, "text/html", getLoginPageTemplate("Setup", body));
}

void handleSaveWifiPage(AsyncWebServerRequest *request) {
    if (request->hasParam("deviceName", true) && request->hasParam("wifi_ssid", true)) {
        currentState = WIFI_CONNECTING;
        strip.setPixelColor(0, COLOR_SAVING);
        strip.show();

        preferences.begin("beecounter", false);
        preferences.putString("deviceName", request->getParam("deviceName", true)->value());
        preferences.putString("ssid", request->getParam("wifi_ssid", true)->value());
        preferences.putString("password", request->getParam("wifiPassword", true)->value());
        preferences.putString("adminPass", ""); // Initialize empty password
        preferences.putBool("isDefaultPass", true); // Set flag
        preferences.end();

        AsyncResponseStream *response = request->beginResponseStream("text/html");
        response->print(F("<!DOCTYPE html><html><head><title>Rebooting...</title><style>body{font-family:sans-serif;background-color:#1a1a1a;color:#FFC300;display:flex;justify-content:center;align-items:center;height:100vh;text-align:center;}</style></head><body><div><h1>Configuration Saved!</h1><p>Device is rebooting...</p></div></body></html>"));
        request->send(response);
        delay(2000);
        ESP.restart();
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}

// --- Authentication & Page Handlers ---
bool isAuthenticated(AsyncWebServerRequest *request) {
    if (currentSessionId.length() == 0) return false;
    if (request->hasHeader("Cookie")) {
        return request->header("Cookie").indexOf(SESSION_COOKIE_NAME "=" + currentSessionId) != -1;
    }
    return false;
}

void handleLoginPage(AsyncWebServerRequest *request) {
    String body = "<img src='https://upload.wikimedia.org/wikipedia/commons/9/91/Abeille-bee.svg' alt='Bee Icon' style='height: 120px; margin-bottom: 1rem;' />" 
                  "<form action='/login' method='POST'>" 
                  "<h2>Login</h2>" 
                  "<div class='form-group'><label>Username</label><input type='text' name='username' value='admin' required></div>" 
                  "<div class='form-group'><label>Password</label><input type='password' name='password' required></div>" 
                  "<button type='submit'>Login</button>" 
                  "</form>";
    request->send(200, "text/html", getLoginPageTemplate("Login", body));
}

void handleDoLogin(AsyncWebServerRequest *request) {
    const int maxLoginAttempts = 5;
    const unsigned long lockoutDuration = 5 * 60 * 1000; // 5 minutes

    preferences.begin("beecounter", false);
    int failCount = preferences.getInt("loginFails", 0);
    unsigned long lastFail = preferences.getULong("lastFailTime", 0);

    if (failCount >= maxLoginAttempts && (millis() - lastFail < lockoutDuration)) {
        preferences.end();
        String body = "<p class='error'>Too many failed login attempts. Please try again in 5 minutes.</p><a href='/login'>Back to Login</a>";
        request->send(429, "text/html", getLoginPageTemplate("Account Locked", body));
        return;
    }

    if (request->hasParam("username", true) && request->hasParam("password", true)) {
        String username = request->getParam("username", true)->value();
        String password = request->getParam("password", true)->value();

        String storedHash = preferences.getString("adminPass", "");
        bool isDefault = preferences.getBool("isDefaultPass", true);

        if (username == "admin") {
            bool loginSuccess = false;
            if (isDefault && password == "admin") {
                loginSuccess = true;
            } else if (!isDefault && sha256(password) == storedHash) {
                loginSuccess = true;
            }

            if (loginSuccess) {
                preferences.putInt("loginFails", 0);
                preferences.putULong("lastFailTime", 0);
                preferences.end();

                currentSessionId = String(random(0, 1000000));
                AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting");
                response->addHeader("Location", isDefault ? "/changepass" : "/");
                response->addHeader("Set-Cookie", String(SESSION_COOKIE_NAME) + "=" + currentSessionId + "; Path=/");
                request->send(response);
                return;
            }
        }
    }

    // If we reach here, the login failed.
    failCount++;
    preferences.putInt("loginFails", failCount);
    preferences.putULong("lastFailTime", millis());
    preferences.end();

    String body = "<img src='https://upload.wikimedia.org/wikipedia/commons/9/91/Abeille-bee.svg' alt='Bee Icon' style='height: 120px; margin-bottom: 1rem;' />" 
                  "<p class='error'>Invalid credentials.</p>" 
                  "<form action='/login' method='POST'>" 
                  "<h2>Login</h2>" 
                  "<div class='form-group'><label>Username</label><input type='text' name='username' value='admin' required></div>" 
                  "<div class='form-group'><label>Password</label><input type='password' name='password' required></div>" 
                  "<button type='submit'>Login</button>" 
                  "</form>";
    request->send(401, "text/html", getLoginPageTemplate("Login Failed", body));
}

String getChangePassForm(String errorMessage = "") {
    String form = "";
    if (errorMessage != "") {
        form += "<p class='error'>" + errorMessage + "</p>";
    }
    form += "<form action='/changepass' method='POST'>" 
            "<h2>Change Admin Password</h2>" 
            "<p>Requirements: 8+ characters, one uppercase, one number, one symbol.</p>" 
            "<div class='form-group'><label>New Password</label><input type='password' id='new_pass' name='new_password' pattern='(?=.*\\d)(?=.*[a-z])(?=.*[A-Z])(?=.*[^A-Za-z0-9]).{8,}' title='Must contain at least one number, one uppercase, one lowercase, one special character, and at least 8 or more characters' required></div>" 
            "<div class='form-group-inline'><input type='checkbox' onclick='togglePassword(\"new_pass\")'> Show Password</div>" 
            "<div class='form-group'><label>Confirm Password</label><input type='password' id='confirm_pass' name='confirm_password' required></div>" 
            "<div class='form-group-inline'><input type='checkbox' onclick='togglePassword(\"confirm_pass\")'> Show Password</div>" 
            "<button type='submit'>Save Password</button>" 
            "</form>" 
            "<script>" 
            "function togglePassword(id) {" 
            "  var x = document.getElementById(id);" 
            "  if (x.type === 'password') { x.type = 'text'; } else { x.type = 'password'; }" 
            "}" 
            "</script>";
    return form;
}

void handleChangePassPage(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }
    request->send(200, "text/html", getPageTemplate("Change Password", getChangePassForm()));
}

void handleDoChangePass(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }
    if (request->hasParam("new_password", true) && request->hasParam("confirm_password", true)) {
        String newPass = request->getParam("new_password", true)->value();
        if (newPass == request->getParam("confirm_password", true)->value()) {
            bool hasUpper = false, hasLower = false, hasDigit = false, hasSymbol = false;
            for (char c : newPass) {
                if (isupper(c)) hasUpper = true;
                if (islower(c)) hasLower = true;
                if (isdigit(c)) hasDigit = true;
                if (!isalnum(c)) hasSymbol = true;
            }
            if (newPass.length() >= 8 && hasUpper && hasLower && hasDigit && hasSymbol) {
                preferences.begin("beecounter", false);
                preferences.putString("adminPass", sha256(newPass));
                preferences.putBool("isDefaultPass", false); // <-- The fix
                preferences.end();
                request->redirect("/");
                return;
            }
        }
    }
    request->send(400, "text/html", getPageTemplate("Password Change Failed", getChangePassForm("Passwords do not match or meet complexity rules.")));
}

void handleMainPage(AsyncWebServerRequest *request) {
    preferences.begin("beecounter", true);
    bool isDefault = preferences.getBool("isDefaultPass", true);
    preferences.end();
    if (isDefault) { request->redirect("/login"); return; }
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }
    
    String message = "";
    if (request->hasParam("success")) {
        message = "<p class='success'>Camera settings saved!</p>";
    }

    String body = R"rawliteral(
        <div class='card' style='padding: 0; text-align: center;'>
            <div id='camera-feed' style='width: 100%; height: 480px; background-color: #000; color: #fff; display: flex; align-items: center; justify-content: center; border-radius: 8px 8px 0 0;'>
                <p>Camera feed not available.</p>
            </div>
        </div>
        <div class='card' style='margin-top: 2rem;'>
            <h2>Camera Controls</h2>
            )rawliteral" + message + R"rawliteral(
            <div style='display: flex; justify-content: space-between; gap: 2rem;'>
                <div class='form-group' style='flex: 1;'>
                    <label for='resolution'>Resolution</label>
                    <select id='resolution' name='resolution'>
                        <option value='UXGA'>UXGA (1600x1200)</option>
                        <option value='SXGA'>SXGA (1280x1024)</option>
                        <option value='XGA'>XGA (1024x768)</option>
                        <option value='SVGA'>SVGA (800x600)</option>
                        <option value='VGA' selected>VGA (640x480)</option>
                        <option value='CIF'>CIF (400x296)</option>
                        <option value='QVGA'>QVGA (320x240)</option>
                    </select>
                </div>
                <div class='form-group' style='flex: 1;'>
                    <label for='quality'>JPEG Quality (<span id='quality-val'>10</span>)</label>
                    <input type='range' id='quality' name='quality' min='10' max='63' value='10' oninput="document.getElementById('quality-val').innerText = this.value">
                </div>
            </div>
            <button onclick='saveCameraSettings()'>Save Settings</button>
        </div>
        <script>
            function saveCameraSettings() {
                const resolution = document.getElementById('resolution').value;
                const quality = document.getElementById('quality').value;
                fetch('/api/camera-settings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `resolution=${resolution}&quality=${quality}`
                }).then(response => {
                    if (response.ok) {
                        window.location.href = '/?success=1';
                    } else {
                        alert('Failed to save camera settings.');
                    }
                });
            }
        </script>
    )rawliteral";
    request->send(200, "text/html", getPageTemplate("Monitor", body, true));
}

void handleTrainPage(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }

    preferences.begin("beecounter", true);
    String eiApiKey = preferences.getString("eiApiKey", "");
    preferences.end();

    String message = "";
    if (request->hasParam("success")) {
        message = "<p class='success'>Edge Impulse credentials saved!</p>";
    }
    
    String body = R"rawliteral(
        <style>
            #image-gallery { display: grid; grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)); gap: 1rem; }
            .img-container { position: relative; }
            .img-container img { width: 100%; height: auto; border-radius: 8px; }
            .img-container .delete-btn { position: absolute; top: 5px; right: 5px; background: rgba(0,0,0,0.7); color: white; border: none; border-radius: 50%; cursor: pointer; width: 25px; height: 25px; font-size: 16px; line-height: 25px; text-align: center;}
            #progress-bar-container { width: 100%; background-color: #555; border-radius: 8px; margin-bottom: 1rem; }
            #progress-bar { width: 0%; height: 30px; background-color: #4CAF50; border-radius: 8px; text-align: center; line-height: 30px; color: white; }
            .grid-container { display: grid; grid-template-columns: 1fr 1fr; gap: 2rem; }
        </style>
        <h2>Training Workflow</h2>
        )rawliteral" + message + R"rawliteral(
        <div class="grid-container">
            <div class='card'>
                <h3>Step 1: Data Collection</h3>
                <p>Automatically capture 50 images over 24 hours, or take manual snapshots.</p>
                <div style='display: flex; justify-content: space-around; margin-bottom: 1rem;'>
                    <button onclick='startCollection()'>Start Auto</button>
                    <button onclick='stopCollection()'>Stop Auto</button>
                    <button onclick='takePhoto()'>Manual Photo</button>
                </div>
                <div id="progress-bar-container">
                    <div id="progress-bar">0%</div>
                </div>
                <p id='collection-status'>Status: Idle</p>
            </div>

            <div class='card'>
                <h3>Step 2: Upload to Edge Impulse</h3>
                <form id="ei-form">
                    <div class='form-group'>
                        <label for='ei-api-key'>API Key</label>
                        <input type='password' id='ei-api-key' name='ei-api-key' value=')rawliteral" + eiApiKey + R"rawliteral(' required>
                    </div>
                    <div class='form-group'>
                        <label for='ei-label'>Image Label</label>
                        <input type='text' id='ei-label' name='ei-label' value='bee' required>
                    </div>
                    <button type='button' onclick='saveEdgeImpulseSettings()'>Save Settings</button>
                    <button type='button' onclick='uploadToEdgeImpulse()' style='margin-top: 1rem;'>Upload All Images</button>
                </form>
                <p id='upload-status' style='margin-top: 1rem;'>Status: Idle</p>
            </div>
        </div>

        <div class='card' style='margin-top: 2rem;'>
            <h3>Collected Images (<span id="img-count">0</span>)</h3>
            <div id='image-gallery'></div>
        </div>

        <script>
            function startCollection() { fetch('/api/capture/start', { method: 'POST' }).then(() => alert('Collection Started!')); }
            function stopCollection() { fetch('/api/capture/stop', { method: 'POST' }).then(() => alert('Collection Stopped!')); }
            function takePhoto() { fetch('/api/capture/photo', { method: 'POST' }).then(() => setTimeout(loadImageGallery, 500)); }
            function saveEdgeImpulseSettings() {
                const apiKey = document.getElementById('ei-api-key').value;
                fetch('/api/edgeimpulse/settings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `ei-api-key=${apiKey}`
                }).then(response => {
                    if (response.ok) { window.location.href = '/train?success=1'; } 
                    else { alert('Failed to save settings.'); }
                });
            }
            function uploadToEdgeImpulse() {
                const label = document.getElementById('ei-label').value;
                if (!label) { alert('Please provide a label for the images.'); return; }
                document.getElementById('upload-status').innerText = 'Status: Starting upload...';
                fetch('/api/edgeimpulse/upload', { 
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `ei-label=${label}`
                });
            }

            function loadImageGallery() {
                fetch('/api/images')
                    .then(response => response.json())
                    .then(images => {
                        const gallery = document.getElementById('image-gallery');
                        gallery.innerHTML = '';
                        document.getElementById('img-count').innerText = images.length;
                        images.forEach(image => {
                            const container = document.createElement('div');
                            container.className = 'img-container';
                            const img = document.createElement('img');
                            img.src = '/images?serve=' + image.substring(8);
                            const btn = document.createElement('button');
                            btn.className = 'delete-btn';
                            btn.innerHTML = '&times;';
                            btn.onclick = () => deleteImage(image.substring(8));
                            container.appendChild(img);
                            container.appendChild(btn);
                            gallery.appendChild(container);
                        });
                    });
            }

            function deleteImage(filename) {
                if (confirm('Are you sure you want to delete ' + filename + '?')) {
                    fetch('/images?delete=' + filename).then(() => loadImageGallery());
                }
            }

            document.addEventListener('DOMContentLoaded', function() {
                loadImageGallery();
                const source = new EventSource('/events');
                source.addEventListener('collection_update', function(e) {
                    const data = JSON.parse(e.data);
                    const progressBar = document.getElementById('progress-bar');
                    progressBar.style.width = data.progress + '%';
                    progressBar.innerText = data.progress + '%';
                    document.getElementById('collection-status').innerText = `Status: Collecting (${data.collected}/${data.total})`;
                    if (data.progress === 100) {
                         document.getElementById('collection-status').innerText = 'Status: Collection Complete!';
                         setTimeout(loadImageGallery, 500);
                    }
                });
                source.addEventListener('ei_upload_status', function(e) {
                    document.getElementById('upload-status').innerText = `Status: ${e.data}`;
                });
            });
        </script>
    )rawliteral";
    request->send(200, "text/html", getPageTemplate("Train", body, true));
}

void handleEdgeImpulseSettings(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->send(401, "text/plain", "Unauthorized"); return; }
    if (request->hasParam("ei-api-key", true)) {
        preferences.begin("beecounter", false);
        preferences.putString("eiApiKey", request->getParam("ei-api-key", true)->value());
        preferences.end();
        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}

void handleEdgeImpulseUpload(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->send(401, "text/plain", "Unauthorized"); return; }
    if (!request->hasParam("ei-label", true)) {
        request->send(400, "text/plain", "Missing label");
        return;
    }

    String label = request->getParam("ei-label", true)->value();
    
    preferences.begin("beecounter", true);
    String apiKey = preferences.getString("eiApiKey", "");
    preferences.end();

    if (apiKey.length() == 0) {
        request->send(400, "text/plain", "API Key not set");
        return;
    }

    request->send(200, "text/plain", "Upload process started.");

    // --- This needs to run asynchronously ---
    xTaskCreate([](void* pvParameters){
        String label = *(String*)pvParameters;
        delete (String*)pvParameters;

        preferences.begin("beecounter", true);
        String apiKey = preferences.getString("eiApiKey", "");
        preferences.end();

        File root = LittleFS.open("/images");
        if (!root) {
            events.send("Failed to open /images directory", "ei_upload_status", millis());
            vTaskDelete(NULL);
            return;
        }

        File file = root.openNextFile();
        int count = 0;
        while(file){
            count++;
            file = root.openNextFile();
        }
        root.close(); // Close and reopen to reset iterator

        root = LittleFS.open("/images");
        file = root.openNextFile();
        int uploaded = 0;
        while(file){
            String filename = String(file.name());
            String statusMsg = "Uploading " + filename + " (" + String(uploaded + 1) + "/" + String(count) + ")";
            events.send(statusMsg.c_str(), "ei_upload_status", millis());

            HTTPClient http;
            http.begin("https://ingestion.edgeimpulse.com/api/training/data");
            http.addHeader("x-api-key", apiKey);
            http.addHeader("x-file-name", filename);
            http.addHeader("x-label", label);
            
            int httpCode = http.sendRequest("POST", &file, file.size());
            http.end();

            if (httpCode == 200) {
                Serial.printf("SUCCESS: Uploaded %s\n", filename.c_str());
            } else {
                Serial.printf("ERROR: Failed to upload %s, HTTP code: %d\n", filename.c_str(), httpCode);
                String errorMsg = "Upload failed for " + filename + ". See serial monitor for details.";
                events.send(errorMsg.c_str(), "ei_upload_status", millis());
                vTaskDelete(NULL);
                return;
            }
            
            file = root.openNextFile();
            uploaded++;
            vTaskDelay(pdMS_TO_TICKS(100)); // Small delay
        }
        events.send("Upload complete!", "ei_upload_status", millis());
        vTaskDelete(NULL);
    }, "eiUploadTask", 8192, new String(label), 1, NULL);
}

void handleSetCameraSettings(AsyncWebServerRequest *request) {
    if (request->hasParam("resolution", true) && request->hasParam("quality", true)) {
        String resolution = request->getParam("resolution", true)->value();
        int quality = request->getParam("quality", true)->value().toInt();

        preferences.begin("beecounter", false);
        preferences.putString("cam_res", resolution);
        preferences.putInt("cam_qlty", quality);
        preferences.end();

        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}

void handleSetRefreshRate(AsyncWebServerRequest *request) {
    if (request->hasParam("rate", true)) {
        performanceUpdateInterval = request->getParam("rate", true)->value().toInt();
        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}

void handleObservabilityPage(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }

    String body = R"rawliteral(
        <h2>Observability</h2>
        <div class='form-group' style='max-width: 200px;'>
            <label for='refresh-rate'>Refresh Rate</label>
            <select id='refresh-rate'>
                <option value='5000' selected>5 Seconds</option>
                <option value='10000'>10 Seconds</option>
                <option value='60000'>60 Seconds</option>
            </select>
        </div>
        <div class='card'><canvas id='cpu-chart'></canvas></div>
        <div class='card'><canvas id='temp-chart'></canvas></div>
        <div class='card'><canvas id='wifi-chart'></canvas></div>
        <div style='display: flex; justify-content: space-between; gap: 2rem;'>
            <div class='card' style='width: 50%;'><canvas id='memory-chart'></canvas></div>
            <div class='card' style='width: 50%;'><canvas id='storage-chart'></canvas></div>
        </div>
        <script src='https://cdn.jsdelivr.net/npm/chart.js'></script>
        <script src='https://cdn.jsdelivr.net/npm/chartjs-plugin-annotation@3.0.1/dist/chartjs-plugin-annotation.min.js'></script>
        <script>
            document.addEventListener('DOMContentLoaded', function() {
                console.log("Observability page JavaScript loaded and DOMContentLoaded fired.");
                // --- Chart.js Configuration ---
                const chartOptions = (title, threshold) => ({
                    responsive: true,
                    maintainAspectRatio: false,
                    scales: {
                        x: { ticks: { color: '#FFC300' }, grid: { color: '#444' } },
                        y: { ticks: { color: '#FFC300' }, grid: { color: '#444' } }
                    },
                    plugins: {
                        legend: { labels: { color: '#FFC300' } },
                        annotation: {
                            annotations: {
                                line1: {
                                    type: 'line',
                                    yMin: threshold,
                                    yMax: threshold,
                                    borderColor: 'rgb(211, 47, 47, 0.8)',
                                    borderWidth: 2,
                                    borderDash: [6, 6],
                                    label: {
                                        content: title,
                                        position: 'end',
                                        backgroundColor: 'rgba(211, 47, 47, 0.8)',
                                        color: '#fff',
                                        font: { style: 'normal' }
                                    }
                                }
                            }
                        }
                    }
                });

                 const doughnutChartOptions = {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: { legend: { position: 'top', labels: { color: '#FFC300' } } }
                };

                // --- CPU Chart ---
                const cpuCtx = document.getElementById('cpu-chart').getContext('2d');
                const cpuChart = new Chart(cpuCtx, {
                    type: 'line',
                    data: {
                        labels: [],
                        datasets: [{ label: 'CPU Core 0 (%)', data: [], borderColor: '#FFC300', backgroundColor: 'rgba(255, 195, 0, 0.2)', fill: true }, {
                            label: 'CPU Core 1 (%)', data: [], borderColor: '#00A8E8', backgroundColor: 'rgba(0, 168, 232, 0.2)', fill: true
                        }]
                    },
                    options: chartOptions('High Load', 70)
                });

                // --- Temperature Chart ---
                const tempCtx = document.getElementById('temp-chart').getContext('2d');
                const tempChart = new Chart(tempCtx, {
                    type: 'line',
                    data: {
                        labels: [],
                        datasets: [{ label: 'CPU Temp (°C)', data: [], borderColor: '#FF5722', backgroundColor: 'rgba(255, 87, 34, 0.2)', fill: true }]
                    },
                    options: chartOptions('High Temp', 60)
                });

                // --- Wi-Fi Chart ---
                const wifiCtx = document.getElementById('wifi-chart').getContext('2d');
                const wifiChart = new Chart(wifiCtx, {
                    type: 'line',
                    data: {
                        labels: [],
                        datasets: [{ label: 'WiFi RSSI (dBm)', data: [], borderColor: '#4CAF50', backgroundColor: 'rgba(76, 175, 80, 0.2)', fill: true }]
                    },
                    options: chartOptions('Poor Signal', -70)
                });

                // --- Memory Chart ---
                const memoryCtx = document.getElementById('memory-chart').getContext('2d');
                const memoryChart = new Chart(memoryCtx, {
                    type: 'doughnut',
                    data: {
                        labels: ['Used Heap (bytes)', 'Free Heap (bytes)'],
                        datasets: [{ data: [0, 0], backgroundColor: ['#FFC300', '#444'] }]
                    },
                    options: doughnutChartOptions
                });

                // --- Storage Chart ---
                const storageCtx = document.getElementById('storage-chart').getContext('2d');
                const storageChart = new Chart(storageCtx, {
                    type: 'doughnut',
                    data: {
                        labels: ['Used Flash (bytes)', 'Free Flash (bytes)'],
                        datasets: [{ data: [0, 0], backgroundColor: ['#00A8E8', '#444'] }]
                    },
                    options: doughnutChartOptions
                });

                // --- Server-Sent Events ---
                if (!!window.EventSource) {
                    var source = new EventSource('/events');

                    source.onopen = function(e) { console.log("Events Connected"); };
                    source.onerror = function(e) { 
                        console.error("EventSource error:", e);
                        if (e.target.readyState != EventSource.OPEN) { console.log("Events Disconnected"); } 
                    };

                    source.addEventListener('performance_update', function(e) {
                        console.log("Performance update event received:", e.data);
                        const data = JSON.parse(e.data);
                        
                        const now = new Date();
                        const timestamp = `${now.getHours()}:${now.getMinutes()}:${now.getSeconds()}`;

                        // Update CPU Chart
                        cpuChart.data.labels.push(timestamp);
                        cpuChart.data.datasets[0].data.push(data.cpu_core_0);
                        cpuChart.data.datasets[1].data.push(data.cpu_core_1);
                        if (cpuChart.data.labels.length > 20) {
                            cpuChart.data.labels.shift();
                            cpuChart.data.datasets[0].data.shift();
                            cpuChart.data.datasets[1].data.shift();
                        }
                        cpuChart.update();

                        // Update Temperature Chart
                        tempChart.data.labels.push(timestamp);
                        tempChart.data.datasets[0].data.push(data.cpu_temp);
                        if (tempChart.data.labels.length > 20) {
                            tempChart.data.labels.shift();
                            tempChart.data.datasets[0].data.shift();
                        }
                        tempChart.update();

                        // Update WiFi Chart
                        wifiChart.data.labels.push(timestamp);
                        wifiChart.data.datasets[0].data.push(data.wifi_rssi);
                        if (wifiChart.data.labels.length > 20) {
                            wifiChart.data.labels.shift();
                            wifiChart.data.datasets[0].data.shift();
                        }
                        wifiChart.update();

                        // Update Memory Chart
                        const heapUsedPercent = (data.heap_used / data.heap_total) * 100;
                        memoryChart.data.datasets[0].backgroundColor[0] = heapUsedPercent > 70 ? '#D32F2F' : '#FFC300';
                        memoryChart.data.datasets[0].data[0] = data.heap_used;
                        memoryChart.data.datasets[0].data[1] = data.heap_total - data.heap_used;
                        memoryChart.update();

                        // Update Storage Chart
                        const flashUsedPercent = (data.flash_used / data.flash_total) * 100;
                        storageChart.data.datasets[0].backgroundColor[0] = flashUsedPercent > 70 ? '#D32F2F' : '#00A8E8';
                        storageChart.data.datasets[0].data[0] = data.flash_used;
                        storageChart.data.datasets[0].data[1] = data.flash_total - data.flash_used;
                        storageChart.update();
                    }, false);
                }

                // --- Refresh Rate Control ---
                const refreshRateSelect = document.getElementById('refresh-rate');
                refreshRateSelect.addEventListener('change', function() {
                    fetch('/api/set-refresh-rate', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                        body: 'rate=' + this.value
                    });
                });
            });
        </script>
    )rawliteral";

    request->send(200, "text/html", getPageTemplate("Observability", body, true));
}

void handleAdminPage(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }

    String message = "";
    if (request->hasParam("success")) {
        message = "<p class='success'>Password updated successfully!</p>";
    }
    if (request->hasParam("error")) {
        message = "<p class='error'>" + request->getParam("error")->value() + "</p>";
    }
    if (request->hasParam("tz_success")) {
        message = "<p class='success'>Timezone found and saved! Please reboot for the change to take effect.</p>";
    }
    if (request->hasParam("tz_error")) {
        message = "<p class='error'>Failed to find timezone.</p>";
    }

    preferences.begin("beecounter", true);
    String currentTz = preferences.getString("timezone", "UTC");
    preferences.end();

    String body = "<h2>Administer</h2>" + message +
                  "<div style='display: flex; gap: 2rem;'>" +
                  "<div class='card' style='flex: 1;'>" +
                  "<h3>Change Password</h3>" +
                  "<form action='/admin/changepass' method='POST'>" +
                  "<div class='form-group'><label>Current Password</label><input type='password' name='current_password' required></div>" +
                  "<div class='form-group'><label>New Password</label><input type='password' name='new_password' pattern='(?=.*\\d)(?=.*[a-z])(?=.*[A-Z])(?=.*[^A-Za-z0-9]).{8,}' title='Must contain at least one number, one uppercase, one lowercase, one special character, and at least 8 or more characters' required></div>" +
                  "<div class='form-group'><label>Confirm New Password</label><input type='password' name='confirm_password' required></div>" +
                  "<button type='submit'>Update Password</button>" +
                  "</form>" +
                  "</div>" +
                  "<div class='card' style='flex: 1;'>" +
                  "<h3>System</h3>" +
                  "<p>The currently set timezone is: <strong>" + currentTz + "</strong></p>" +
                  "<form action='/admin/find-timezone' method='POST' style='margin-bottom:1rem;'>" +
                  "<button type='submit'>Auto-Detect Timezone</button>" +
                  "</form>" +
                  "<form action='/admin/reboot' method='POST' onsubmit='return confirm(\"Are you sure you want to reboot?\" );' style='margin-bottom:1rem;'>" +
                  "<button type='submit'>Reboot Device</button>" +
                  "</form>" +
                  "<form action='/factory-reset' method='POST' onsubmit='return confirm(\"Are you sure? This erases all settings.\" );'>" +
                  "<button type='submit' class='danger'>Factory Reset</button>" +
                  "</form>" +
                  "</div>" +
                  "</div>" +
                  "<div class='card' style='margin-top: 2rem;'>" +
                  "<h3>Firmware Update (OTA)</h3>" +
                  "<p>Upload a new firmware binary. The device will update and reboot automatically. Ensure the file is a valid `.bin` file for this hardware.</p>" +
                  "<form method='POST' action='/update' enctype='multipart/form-data'>" +
                  "<div class='form-group'><input type='file' name='update' accept='.bin'></div>" +
                  "<button type='submit'>Upload and Update</button>" +
                  "</form>" +
                  "<p><small>Note: For this to work, you must upload the firmware via the PlatformIO OTA command: `pio run --target upload --upload-port [DEVICE_IP]`</small></p>" +
                  "</div>";

    request->send(200, "text/html", getPageTemplate("Administer", body, true));
}

void handleAboutPage(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }
    
    String os_string = String(BUILD_OS);
    int debian_index = os_string.indexOf("Debian");
    if (debian_index != -1) {
        os_string = os_string.substring(debian_index);
    }

    String body = "<div class='card'><h2>About BeeCounter</h2>" 
                  "<p>This device is designed to monitor bee activity at a hive entrance.</p>" 
                  "<ul>" 
                  "<li><strong>Firmware Version:</strong> " + String(APP_VERSION) + "</li>" 
                  "<li><strong>Build Date:</strong> " + String(BUILD_DATE) + "</li>" 
                  "<li><strong>Build Host:</strong> " + String(BUILD_HOST) + "</li>" 
                  "<li><strong>Build Platform:</strong> " + String(BUILD_PLATFORM) + "</li>" 
                  "<li><strong>Build Operating System:</strong> " + os_string + "</li>" 
                  "<li><strong>Local IP Address:</strong> " + WiFi.localIP().toString() + "</li>" 
                  "<li><strong>Public IP Address:</strong> " + getPublicIP() + "</li>" 
                  "</ul>" 
                  "<p><a href='/changelog'>View Full Changelog</a></p></div>";

    request->send(200, "text/html", getPageTemplate("About", body, true));
}

void handleChangelogPage(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }
    
    File file = LittleFS.open("/CHANGELOG.md", "r");
    if(!file){
        request->send(404, "text/plain", "Changelog not found.");
        return;
    }
    String changelogContent = file.readString();
    file.close();

    String body = "<div class='card'><h2>Changelog</h2><pre style='background-color:#333;padding:1rem;border-radius:8px;white-space:pre-wrap;word-wrap:break-word;color:#fff;'>" + changelogContent + "</pre></div>";
    
    request->send(200, "text/html", getPageTemplate("Changelog", body, true));
}

void handleFindTimezone(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }
    if (autoSetTimezone()) {
        request->redirect("/admin?tz_success=1");
    } else {
        request->redirect("/admin?tz_error=1");
    }
}

void handleReboot(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }
    String body = "<h1>Rebooting...</h1><meta http-equiv='refresh' content='5;url=/' />";
    request->send(200, "text/html", getPageTemplate("Rebooting...", body));
    delay(2000);
    ESP.restart();
}

void handleDoAdminChangePass(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }

    if (request->hasParam("current_password", true) && request->hasParam("new_password", true) && request->hasParam("confirm_password", true)) {
        String currentPass = request->getParam("current_password", true)->value();
        String newPass = request->getParam("new_password", true)->value();
        String confirmPass = request->getParam("confirm_password", true)->value();

        preferences.begin("beecounter", true);
        String storedHash = preferences.getString("adminPass", "");
        preferences.end();

        if (sha256(currentPass) != storedHash) {
            request->redirect("/admin?error=Incorrect+current+password");
            return;
        }

        if (newPass != confirmPass) {
            request->redirect("/admin?error=New+passwords+do+not+match");
            return;
        }

        bool hasUpper = false, hasLower = false, hasDigit = false, hasSymbol = false;
        for (char c : newPass) {
            if (isupper(c)) hasUpper = true;
            if (islower(c)) hasLower = true;
            if (isdigit(c)) hasDigit = true;
            if (!isalnum(c)) hasSymbol = true;
        }

        if (newPass.length() >= 8 && hasUpper && hasLower && hasDigit && hasSymbol) {
            preferences.begin("beecounter", false);
            preferences.putString("adminPass", sha256(newPass));
            preferences.end();
            request->redirect("/admin?success=1");
            return;
        } else {
            request->redirect("/admin?error=Password+does+not+meet+complexity+rules");
            return;
        }
    }
    request->redirect("/admin?error=Missing+fields");
}

void handleFactoryReset(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->redirect("/login"); return; }
    preferences.begin("beecounter", false);
    preferences.clear();
    preferences.end();
    
    String body = "<h1>Factory Reset Successful</h1><p>Device is rebooting into setup mode...</p>";
    request->send(200, "text/html", getPageTemplate("Resetting...", body));
    delay(2000);
    ESP.restart();
}

void handleLogout(AsyncWebServerRequest *request) {
    currentSessionId = "";
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting");
    response->addHeader("Location", "/login");
    response->addHeader("Set-Cookie", String(SESSION_COOKIE_NAME) + "=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
    request->send(response);
}

// --- HTML Template ---
String getFooter() {
    char timeStr[20];
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) { // 5s timeout
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", &timeinfo);
    } else {
        strcpy(timeStr, "Time not synced");
    }

    preferences.begin("beecounter", true);
    String deviceName = preferences.getString("deviceName", "BeeCounter");
    preferences.end();

    String footer = "<footer><p>";
    footer += deviceName + " | v" + String(APP_VERSION) + " | <span id='footer-time'>" + String(timeStr) + "</span>";
    footer += "</p></footer>";
    return footer;
}

// --- Error Logging ---
void logError(const String& message) {
    const char* logFile = "/error.log";
    const size_t maxLogSize = 10240; // 10 KB

    if (LittleFS.exists(logFile) && LittleFS.open(logFile, "r").size() > maxLogSize) {
        LittleFS.rename(logFile, "/error.log.bak");
    }

    File file = LittleFS.open(logFile, "a");
    if (!file) {
        Serial.println("ERROR: Failed to open error.log for writing.");
        return;
    }

    char timeStr[30];
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    } else {
        strcpy(timeStr, "Time not synced");
    }

    String logMessage = String(timeStr) + " - " + message + "\n";
    file.print(logMessage);
    file.close();
    Serial.print("LOGGED ERROR: " + message);
}


String getPageTemplate(const String& title, const String& body, bool includeTabs) {
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>BeeCounter - " + title + "</title>";
    html += "<style>" 
            "html, body {height: 100%; margin: 0;}"
            "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;background-color:#444;color:#FFC300;display:flex;flex-direction:column;}"
            "main{flex:1 0 auto;}"
            "header{background-color:#555;color:#FFC300;padding:1rem 2rem;display:flex;justify-content:space-between;align-items:center;border-bottom:2px solid #FFC300;}"
            "header h1{margin:0;font-size:1.5rem;} header a{color:#FFC300;text-decoration:none;}"
            ".tabs{background-color:#555;padding:0 2rem;border-bottom:1px solid #444;}"
            ".tabs nav{display:flex;}"
            ".tabs a{padding:1rem 1.5rem;color:#fff;text-decoration:none;border-bottom:3px solid transparent;}"
            ".tabs a.active, .tabs a:hover{color:#FFC300;border-bottom:3px solid #FFC300;}"
            ".container{padding:2rem;}"
            ".container a{color:#FFC300;}"
            "form{background-color:#222;padding:2rem;border-radius:8px;max-width:400px;margin:2rem auto;border:1px solid #FFC300;}"
            "form h2, form h3{margin-top:0;text-align:center;}"
            ".form-group{margin-bottom:1rem;}"
            ".form-group-inline{margin-bottom:1rem; display:flex; align-items:center;}"
            ".form-group-inline input{width:auto; margin-right:0.5rem;}"
            "label{display:block;margin-bottom:.5rem;}"
            "input, select{width:100%;padding:.75rem;box-sizing:border-box;border-radius:4px;border:1px solid #FFC300;background:#333;color:#fff;}"
            "button{width:100%;padding:.75rem;border:none;border-radius:4px;background-color:#FFC300;color:#000;font-size:1.1rem;cursor:pointer;}"
            "button.danger{background-color:#D32F2F;color:#fff;}"
            ".error{color:#F44336;text-align:center;margin-bottom:1rem;}"
            ".success{color:#4CAF50;text-align:center;margin-bottom:1rem;}"
            "footer{background-color:#555; flex-shrink: 0; text-align:center;padding:1rem;color:#ddd;font-size:0.9rem;border-top:1px solid #444;}"
            "</style></head><body>";
    
    if (includeTabs) {
        html += "<header><h1>BeeCounter</h1><img src='https://upload.wikimedia.org/wikipedia/commons/9/91/Abeille-bee.svg' alt='Bee Icon' style='height: 40px; margin: auto;' /><div><a href='/about'>About</a><span style='margin:0 10px;'>|</span><a href='/logout'>Logout</a></div></header>";
        html += "<div class='tabs'><nav>";
        html += String("<a href='/' class='") + (title == "Monitor" ? "active" : "") + "'>Monitor</a>";
        html += String("<a href='/train' class='") + (title == "Train" ? "active" : "") + "'>Train</a>";
        html += String("<a href='/observability' class='") + (title == "Observability" ? "active" : "") + "'>Observability</a>";
        html += String("<a href='/admin' class='") + (title == "Administer" ? "active" : "") + "'>Administer</a>";
        html += "</nav></div>";
    }

    html += "<main class='container'>" + body + "</main>";
    html += getFooter();
    html += "<script>" 
            "function updateTime() {" 
            "  const timeElement = document.getElementById('footer-time');" 
            "  if (timeElement) {" 
            "    const now = new Date();" 
            "    const year = now.getFullYear();" 
            "    const month = String(now.getMonth() + 1).padStart(2, '0');" 
            "    const day = String(now.getDate()).padStart(2, '0');" 
            "    const hours = String(now.getHours()).padStart(2, '0');" 
            "    const minutes = String(now.getMinutes()).padStart(2, '0');" 
            "    timeElement.textContent = `${year}-${month}-${day} ${hours}:${minutes}`;" 
            "  }" 
            "}" 
            "updateTime();" // Initial call to set time immediately
            "setInterval(updateTime, 60000);" // Update every 60 seconds
            "</script>";
    html += "</body></html>";
    return html;
}

void handleCaptureStart(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->send(401, "text/plain", "Unauthorized"); return; }
    if (!isCollecting) {
        isCollecting = true;
        imagesCollected = 0;
        lastCollectionTime = millis(); // Start the timer now
        Serial.println("INFO: Starting automated data collection.");
    }
    request->send(200, "text/plain", "Data collection started.");
}

void handleCaptureStop(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->send(401, "text/plain", "Unauthorized"); return; }
    if (isCollecting) {
        isCollecting = false;
        Serial.println("INFO: Stopping automated data collection.");
    }
    request->send(200, "text/plain", "Data collection stopped.");
}

void handleCapturePhoto(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->send(401, "text/plain", "Unauthorized"); return; }

    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("ERROR: Camera capture failed");
        request->send(500, "text/plain", "Camera capture failed");
        return;
    }

    // Create a unique filename using the current timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    String path = "/images/img-" + String(tv.tv_sec) + ".jpg";
    
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("ERROR: Failed to open file for writing");
        esp_camera_fb_return(fb);
        request->send(500, "text/plain", "Failed to open file for writing");
        return;
    }

    file.write(fb->buf, fb->len);
    file.close();
    Serial.printf("SUCCESS: Image saved to %s (%d bytes)\n", path.c_str(), fb->len);
    
    esp_camera_fb_return(fb);
    
    request->send(200, "text/plain", "Photo captured and saved as " + path);
}



void handleEdgeImpulseDownloadModel(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) { request->send(401, "text/plain", "Unauthorized"); return; }
    // In a real implementation, you would parse the API key, then download the model from Edge Impulse
    request->send(200, "text/plain", "Model downloaded (placeholder)");
}