#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "config.h"
#include <string.h>

static const char *TAG = "config";

esp_err_t config_load(app_config_t *config) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    size_t required_size = sizeof(config->wifi_ssid);
    err = nvs_get_str(nvs_handle, "wifi_ssid", config->wifi_ssid, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get wifi_ssid: %s", esp_err_to_name(err));
        // This is not a fatal error if the value is not set yet.
    }

    required_size = sizeof(config->wifi_password);
    err = nvs_get_str(nvs_handle, "wifi_password", config->wifi_password, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get wifi_password: %s", esp_err_to_name(err));
    }

    required_size = sizeof(config->system_name);
    err = nvs_get_str(nvs_handle, "system_name", config->system_name, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get system_name: %s", esp_err_to_name(err));
        // Default to a name if not set
        strcpy(config->system_name, "BeeMonitor");
    }

    required_size = sizeof(config->username);
    err = nvs_get_str(nvs_handle, "username", config->username, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get username: %s", esp_err_to_name(err));
    }

    required_size = sizeof(config->password);
    err = nvs_get_str(nvs_handle, "password", config->password, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get password: %s", esp_err_to_name(err));
    }

    int64_t lat_val, lon_val;
    err = nvs_get_i64(nvs_handle, "latitude", &lat_val);
    if (err == ESP_OK) {
        config->latitude = (double)lat_val / 1000000.0;
    } else {
        ESP_LOGW(TAG, "Failed to get latitude: %s", esp_err_to_name(err));
        config->latitude = 0; // Default value
    }

    err = nvs_get_i64(nvs_handle, "longitude", &lon_val);
    if (err == ESP_OK) {
        config->longitude = (double)lon_val / 1000000.0;
    } else {
        ESP_LOGW(TAG, "Failed to get longitude: %s", esp_err_to_name(err));
        config->longitude = 0; // Default value
    }

    required_size = sizeof(config->ei_api_key);
    err = nvs_get_str(nvs_handle, "ei_api_key", config->ei_api_key, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get ei_api_key: %s", esp_err_to_name(err));
    }

    required_size = sizeof(config->ei_hmac_key);
    err = nvs_get_str(nvs_handle, "ei_hmac_key", config->ei_hmac_key, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get ei_hmac_key: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t config_save(const app_config_t *config) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, "wifi_ssid", config->wifi_ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set wifi_ssid: %s", esp_err_to_name(err));
    }

    err = nvs_set_str(nvs_handle, "wifi_password", config->wifi_password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set wifi_password: %s", esp_err_to_name(err));
    }

    err = nvs_set_str(nvs_handle, "system_name", config->system_name);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set system_name: %s", esp_err_to_name(err));
    }

    err = nvs_set_str(nvs_handle, "username", config->username);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set username: %s", esp_err_to_name(err));
    }

    err = nvs_set_str(nvs_handle, "password", config->password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set password: %s", esp_err_to_name(err));
    }

    int64_t lat_val = (int64_t)(config->latitude * 1000000.0);
    err = nvs_set_i64(nvs_handle, "latitude", lat_val);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set latitude: %s", esp_err_to_name(err));
    }

    int64_t lon_val = (int64_t)(config->longitude * 1000000.0);
    err = nvs_set_i64(nvs_handle, "longitude", lon_val);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set longitude: %s", esp_err_to_name(err));
    }

    err = nvs_set_str(nvs_handle, "ei_api_key", config->ei_api_key);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set ei_api_key: %s", esp_err_to_name(err));
    }

    err = nvs_set_str(nvs_handle, "ei_hmac_key", config->ei_hmac_key);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set ei_hmac_key: %s", esp_err_to_name(err));
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}
