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
#include "esp_spiffs.h"
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "provisioning.h"

static const char *TAG = "provisioning";

static httpd_handle_t server = NULL;

#define SPIFFS_BASE_PATH "/spiffs"

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

static esp_err_t serve_file(httpd_req_t *req, const char* filepath, const char* content_type)
{
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", SPIFFS_BASE_PATH, filepath);

    struct stat st;
    if (stat(full_path, &st) == -1) {
        ESP_LOGE(TAG, "File not found: %s", full_path);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    int fd = open(full_path, O_RDONLY);
    if (fd == -1) {
        ESP_LOGE(TAG, "Failed to open file: %s", full_path);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, content_type);

    char buffer[1024];
    int bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
            close(fd);
            ESP_LOGE(TAG, "File sending failed!");
            return ESP_FAIL;
        }
    }
    close(fd);
    httpd_resp_send_chunk(req, NULL, 0); // Finalize the chunked response
    return ESP_OK;
}

static esp_err_t main_page_handler(httpd_req_t *req) { return serve_file(req, "/index.html", "text/html"); }
static esp_err_t style_css_handler(httpd_req_t *req) { return serve_file(req, "/style.css", "text/css"); }
static esp_err_t bee1_jpg_handler(httpd_req_t *req) { return serve_file(req, "/bee1.jpg", "image/jpeg"); }
static esp_err_t bee2_jpg_handler(httpd_req_t *req) { return serve_file(req, "/bee2.jpg", "image/jpeg"); }
static esp_err_t hardware_page_handler(httpd_req_t *req) { return serve_file(req, "/hardware.html", "text/html"); }


static esp_err_t scan_json_handler(httpd_req_t *req)
{
    // ... (scan logic remains the same)
    return ESP_OK;
}

static const httpd_uri_t main_uri = { .uri = "/", .method = HTTP_GET, .handler = main_page_handler };
static const httpd_uri_t style_css_uri = { .uri = "/style.css", .method = HTTP_GET, .handler = style_css_handler };
static const httpd_uri_t bee1_jpg_uri = { .uri = "/bee1.jpg", .method = HTTP_GET, .handler = bee1_jpg_handler };
static const httpd_uri_t bee2_jpg_uri = { .uri = "/bee2.jpg", .method = HTTP_GET, .handler = bee2_jpg_handler };
static const httpd_uri_t hardware_uri = { .uri = "/hardware.html", .method = HTTP_GET, .handler = hardware_page_handler };
static const httpd_uri_t scan_json_uri = { .uri = "/scan.json", .method = HTTP_GET, .handler = scan_json_handler };


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
        httpd_register_uri_handler(server, &style_css_uri);
        httpd_register_uri_handler(server, &bee1_jpg_uri);
        httpd_register_uri_handler(server, &bee2_jpg_uri);
        httpd_register_uri_handler(server, &hardware_uri);
        httpd_register_uri_handler(server, &scan_json_uri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = SPIFFS_BASE_PATH,
      .partition_label = "spiffs",
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
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
    init_spiffs();
    wifi_init_softap();
    server = start_webserver();
    if (server) {
        return ESP_OK;
    }
    return ESP_FAIL;
}