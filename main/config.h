#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_NAMESPACE "beemonitor"

typedef struct {
    char wifi_ssid[32];
    char wifi_password[64];
    char system_name[32];
    char username[32];
    char password[32];
    double latitude;
    double longitude;
} app_config_t;

esp_err_t config_load(app_config_t *config);
esp_err_t config_save(const app_config_t *config);

#ifdef __cplusplus
}
#endif

#endif // _CONFIG_H_
