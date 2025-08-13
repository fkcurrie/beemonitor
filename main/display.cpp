#include "display.h"
extern "C" {
#include "u8g2_esp32_hal.h"
}
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display";

// Default I2C pins for ESP32-S3-DevKitC-1 are GPIO21 (SDA) and GPIO22 (SCL)
// but many OLED modules use other pins. Let's define them here.
#define PIN_SDA 5
#define PIN_SCL 4

static u8g2_t u8g2; // a structure which will contain all the data for one display

void display_init(void) {
    // u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    // u8g2_esp32_hal.bus.i2c.sda = (gpio_num_t)PIN_SDA;
    // u8g2_esp32_hal.bus.i2c.scl = (gpio_num_t)PIN_SCL;
    // u8g2_esp32_hal_init(u8g2_esp32_hal);

    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8g2_esp32_i2c_byte_cb,
        u8g2_esp32_gpio_and_delay_cb);

    // The I2C address for SSD1306 displays is usually 0x3C.
    // u8x8_SetI2CAddress expects the 8-bit address, so we shift 0x3C left by one.
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x3C << 1);

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
    u8g2_DrawStr(&u8g2, 0, 24, "Starting...");
    u8g2_SendBuffer(&u8g2);
    ESP_LOGI(TAG, "Display initialized");
}

void display_show_version(const char *version) {
    char version_buf[40];
    sprintf(version_buf, "Version: %s", version);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
    u8g2_DrawStr(&u8g2, 0, 24, "Bee Monitor");
    u8g2_DrawStr(&u8g2, 0, 52, version_buf);
    u8g2_SendBuffer(&u8g2);
    ESP_LOGI(TAG, "Displaying version: %s", version);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}

void display_show_copyright(void) {
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2, 0, 24, "Copyright by");
    u8g2_DrawStr(&u8g2, 0, 48, "Frank Currie (2025)");
    u8g2_SendBuffer(&u8g2);
    ESP_LOGI(TAG, "Displaying copyright");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
}

void display_wifi_connecting(const char *ssid) {
    char buf[64];
    sprintf(buf, "Connecting to:");
    
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2, 0, 24, buf);
    u8g2_DrawStr(&u8g2, 0, 48, ssid);
    u8g2_SendBuffer(&u8g2);
    ESP_LOGI(TAG, "Displaying WiFi connecting message for SSID: %s", ssid);
}

void display_update_counts(int in_count, int out_count) {
    char in_buf[20];
    char out_buf[20];

    sprintf(in_buf, "IN: %d", in_count);
    sprintf(out_buf, "OUT: %d", out_count);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&u8g2, 0, 24, in_buf);
    u8g2_DrawStr(&u8g2, 0, 52, out_buf);
    u8g2_SendBuffer(&u8g2);
    ESP_LOGI(TAG, "Display updated: IN=%d, OUT=%d", in_count, out_count);
}
