#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "config.h"
#include <string.h>
#include <sys/param.h>

static const char *TAG = "provisioning";

static httpd_handle_t server = NULL;

extern const uint8_t style_css_start[] asm("_binary_web_style_css_start");
extern const uint8_t style_css_end[]   asm("_binary_web_style_css_end");
extern const uint8_t bee1_jpg_start[] asm("_binary_web_bee1_jpg_start");
extern const uint8_t bee1_jpg_end[]   asm("_binary_web_bee1_jpg_end");
extern const uint8_t bee2_jpg_start[] asm("_binary_web_bee2_jpg_start");
extern const uint8_t bee2_jpg_end[]   asm("_binary_web_bee2_jpg_end");

/* Event handler for catching system events */
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

static esp_err_t style_css_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_css_start, style_css_end - style_css_start);
    return ESP_OK;
}

static const httpd_uri_t style_css_uri = {
    .uri       = "/style.css",
    .method    = HTTP_GET,
    .handler   = style_css_handler,
    .user_ctx  = NULL
};

static esp_err_t bee1_jpg_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_send(req, (const char *)bee1_jpg_start, bee1_jpg_end - bee1_jpg_start);
    return ESP_OK;
}

static const httpd_uri_t bee1_jpg_uri = {
    .uri       = "/bee1.jpg",
    .method    = HTTP_GET,
    .handler   = bee1_jpg_handler,
    .user_ctx  = NULL
};

static esp_err_t bee2_jpg_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_send(req, (const char *)bee2_jpg_start, bee2_jpg_end - bee2_jpg_start);
    return ESP_OK;
}

static const httpd_uri_t bee2_jpg_uri = {
    .uri       = "/bee2.jpg",
    .method    = HTTP_GET,
    .handler   = bee2_jpg_handler,
    .user_ctx  = NULL
};

static esp_err_t main_page_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            app_config_t config;
            char ssid[32] = {0};
            char password[64] = {0};
            char sysname[32] = {0};
            char username[32] = {0};
            char user_password[32] = {0};

            if (httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid)) == ESP_OK &&
                httpd_query_key_value(buf, "password", password, sizeof(password)) == ESP_OK &&
                httpd_query_key_value(buf, "sysname", sysname, sizeof(sysname)) == ESP_OK &&
                httpd_query_key_value(buf, "username", username, sizeof(username)) == ESP_OK &&
                httpd_query_key_value(buf, "user_password", user_password, sizeof(user_password)) == ESP_OK) {

                ESP_LOGI(TAG, "Saving configuration...");
                strcpy(config.wifi_ssid, ssid);
                strcpy(config.wifi_password, password);
                strcpy(config.system_name, sysname);
                strcpy(config.username, username);
                strcpy(config.password, user_password);
                config_save(&config);

                httpd_resp_send(req, "<h1>Configuration Saved!</h1><p>The device will now restart.</p>", HTTPD_RESP_USE_STRLEN);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                esp_restart();
                return ESP_OK;
            }
        }
        free(buf);
    }

    const char* resp_str = R"html(
<!DOCTYPE html>
<html>
<head>
<title>Apiary Guardian Setup</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" type="text/css" href="/style.css">
</head>
<body>
<div class="container">
<h1>Apiary Guardian</h1>
<p style="text-align: center;">Welcome! Please configure your bee monitoring device.</p>
<form action="/" method="get">
  <label for="sysname">System Name</label>
  <input type="text" id="sysname" name="sysname" value="BeeMonitor-1">

  <label for="ssid">WiFi Network (SSID)</label>
  <select id="ssid" name="ssid"></select>
  <button type="button" onclick="scanWifi()">Scan for Networks</button>

  <label for="password">WiFi Password</label>
  <input type="password" id="password" name="password">

  <hr>
  <h3>Device Access</h3>
  <label for="username">Username</label>
  <input type="text" id="username" name="username" value="admin">
  <label for="user_password">Password</label>
  <input type="password" id="user_password" name="user_password">

  <input type="submit" value="Save and Reboot">
</form>
<a href="/hardware" class="hardware-link">View Hardware Info</a>
<div class="footer">
    <img src="/bee1.jpg" class="bee-img">
    <img src="/bee2.jpg" class="bee-img">
    <p>Copyright by Frank Currie (2025)</p>
</div>
</div>
<script>
function scanWifi() {
  var ssidSelect = document.getElementById("ssid");
  ssidSelect.innerHTML = "<option>Scanning...</option>";
  fetch('/scan.json')
    .then(response => response.json())
    .then(data => {
      ssidSelect.innerHTML = "";
      data.forEach(function(ssid) {
        var option = document.createElement("option");
        option.text = ssid;
        ssidSelect.add(option);
      });
    });
}
window.onload = scanWifi;
</script>
</body>
</html>
)html";

    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static const httpd_uri_t main_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = main_page_handler,
    .user_ctx  = NULL
};

static esp_err_t hardware_page_handler(httpd_req_t *req)
{
    const char* resp_str = R"html(
<!DOCTYPE html>
<html>
<head><title>Hardware Info</title></head>
<body>
<h1>Hardware Info</h1>
<ul>
  <li>Board: ESP32-S3-DevKitC-1</li>
  <li>Camera: OV5640 (DVP)</li>
  <li>Display: SSD1306 I2C OLED</li>
</ul>
<p><a href="/">Back to Setup</a></p>
</body>
</html>
)html";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t hardware_uri = {
    .uri       = "/hardware",
    .method    = HTTP_GET,
    .handler   = hardware_page_handler,
    .user_ctx  = NULL
};


static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &main_uri);
        httpd_register_uri_handler(server, &hardware_uri);
        httpd_register_uri_handler(server, &style_css_uri);
        httpd_register_uri_handler(server, &bee1_jpg_uri);
        httpd_register_uri_handler(server, &bee2_jpg_uri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.ap.ssid, "BeeMonitor-Setup");
    wifi_config.ap.ssid_len = strlen("BeeMonitor-Setup");
    wifi_config.ap.channel = 1;
    strcpy((char*)wifi_config.ap.password, "beesbees");
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    if (strlen((char*)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             "BeeMonitor-Setup", "beesbees", 1);
}

esp_err_t start_provisioning_server(void)
{
    wifi_init_softap();
    server = start_webserver();
    if (server) {
        return ESP_OK;
    }
    return ESP_FAIL;
}