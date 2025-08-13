idf_component_register(SRCS "beemonitor.cpp" "person_detect_model_data.cc" "display.cpp" "config.c" "provisioning.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp32-camera esp_http_server esp_wifi esp_netif nvs_flash esp_event esp-tflite-micro esp_http_client u8g2-hal-esp-idf)

target_add_binary_data(${COMPONENT_TARGET} "web/style.css" TEXT)


target_add_binary_data(${COMPONENT_TARGET} "web/style.css" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "web/bee1.jpg" BINARY)
target_add_binary_data(${COMPONENT_TARGET} "web/bee2.jpg" BINARY)




