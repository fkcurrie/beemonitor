#include <stdio.h>
#include <string.h>
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_sleep.h"
#include "display.h"
#include "version.h"

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "person_detect_model_data.h"
#include "config.h"
#include "provisioning.h"

// Camera Pins for ESP32-S3-DevKitC-1 with Adafruit OV5640
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 5 // I2C SDA
#define CAM_PIN_SIOC 4 // I2C SCL

#define CAM_PIN_D7 16
#define CAM_PIN_D6 17
#define CAM_PIN_D5 18
#define CAM_PIN_D4 12
#define CAM_PIN_D3 11
#define CAM_PIN_D2 10
#define CAM_PIN_D1 9
#define CAM_PIN_D0 13
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HSYNC 7
#define CAM_PIN_PCLK 14

static const char *TAG = "main";

#define SERVER_URL     "http://your-server-ip/api/bee-counts"

// RTC memory to store counts between deep sleep cycles
static int bee_in_count = 0;
static int bee_out_count = 0;


static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HSYNC,
    .pin_pclk = CAM_PIN_PCLK,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_GRAYSCALE,
    .frame_size = FRAMESIZE_96X96,
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    .sccb_i2c_port = 0,
};

namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
constexpr int kTensorArenaSize = 73 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
}  // namespace

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static bool wifi_connect_sta(const app_config_t *config)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, config->wifi_ssid);
    strcpy((char*)wifi_config.sta.password, config->wifi_password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(15000)); // 15 second timeout

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s", config->wifi_ssid);
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s", config->wifi_ssid);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    return false;
}


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    return ESP_OK;
}

static void send_data_to_server(int in, int out)
{
    char post_data[100];
    sprintf(post_data, "{\"in\": %d, \"out\": %d}", in, out);

    esp_http_client_config_t config = {};
    config.url = SERVER_URL;
    config.event_handler = _http_event_handler;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}


void setup_tflite() {
    model = tflite::GetModel(g_person_detect_model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Model version does not match Schema");
        return;
    }

    static tflite::MicroMutableOpResolver<8> resolver;
    resolver.AddAveragePool2D();
    resolver.AddConv2D();
    resolver.AddDepthwiseConv2D();
    resolver.AddReshape();
    resolver.AddSoftmax();
    resolver.AddPad();
    resolver.AddMean();
    resolver.AddFullyConnected();

    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors() failed");
        return;
    }
    input = interpreter->input(0);
}

extern "C" void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Display
    display_init();
    display_show_version(FIRMWARE_VERSION);
    display_show_copyright();

    app_config_t app_config;
    esp_err_t err = config_load(&app_config);

    if (err != ESP_OK || strlen(app_config.wifi_ssid) == 0) {
        ESP_LOGI(TAG, "No configuration found, starting provisioning server.");
        start_provisioning_server();
        // The provisioning server is a blocking call and will restart the device when done.
        // We can just wait here.
        while(1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
    }

    display_wifi_connecting(app_config.wifi_ssid);

    if (!wifi_connect_sta(&app_config)) {
        ESP_LOGI(TAG, "Failed to connect to saved WiFi, starting provisioning server.");
        start_provisioning_server();
        while(1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
    }

    // Initialize Camera
    ESP_ERROR_CHECK(esp_camera_init(&camera_config));

    // Setup TensorFlow Lite
    setup_tflite();

    // Main application loop
    while(1) {
        // Capture and process one image
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
        } else {
            if (input->bytes == fb->len) {
                memcpy(input->data.uint8, fb->buf, fb->len);

                if (interpreter->Invoke() != kTfLiteOk) {
                    ESP_LOGE(TAG, "Invoke failed");
                }

                TfLiteTensor* output = interpreter->output(0);
                int person_score = output->data.uint8[1];
                int no_person_score = output->data.uint8[0];

                ESP_LOGI(TAG, "Person score: %d, No person score: %d", person_score, no_person_score);
                
                // This is where you would implement your tripwire logic.
                // For now, we'll just increment the counters as a test.
                if (person_score > 200) { // Threshold for person detection
                    bee_in_count++;
                    bee_out_count++;
                }

            } else {
                ESP_LOGE(TAG, "Input tensor size (%d) does not match frame buffer size (%d)", input->bytes, fb->len);
            }
            esp_camera_fb_return(fb);
        }
        
        // Send data to server
        send_data_to_server(bee_in_count, bee_out_count);

        // Update the display
        display_update_counts(bee_in_count, bee_out_count);

        // Wait for 30 seconds before the next cycle
        ESP_LOGI(TAG, "Waiting for 30 seconds");
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}