
static esp_err_t config_handler(httpd_req_t *req)
{
    char buf[256];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        app_config_t cfg;
        char param[64];

        if (httpd_query_key_value(buf, "sysname", param, sizeof(param)) == ESP_OK) {
            strcpy(cfg.system_name, param);
        }
        if (httpd_query_key_value(buf, "ssid", param, sizeof(param)) == ESP_OK) {
            strcpy(cfg.wifi_ssid, param);
        }
        if (httpd_query_key_value(buf, "password", param, sizeof(param)) == ESP_OK) {
            strcpy(cfg.wifi_password, param);
        }
        if (httpd_query_key_value(buf, "username", param, sizeof(param)) == ESP_OK) {
            strcpy(cfg.username, param);
        }
        if (httpd_query_key_value(buf, "user_password", param, sizeof(param)) == ESP_OK) {
            strcpy(cfg.password, param);
        }
        if (httpd_query_key_value(buf, "lat", param, sizeof(param)) == ESP_OK) {
            cfg.latitude = atof(param);
        }
        if (httpd_query_key_value(buf, "lon", param, sizeof(param)) == ESP_OK) {
            cfg.longitude = atof(param);
        }

        config_save(&cfg);

        httpd_resp_send(req, "Configuration saved. Rebooting...", HTTPD_RESP_USE_STRLEN);
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    } else {
        // No query string, serve the main page
        return serve_file(req, "/index.html", "text/html");
    }
    return ESP_OK;
}
