# CMakeLists.txt for spiffs_image component

idf_component_register(REQUIRES spiffs)

# Set the path to the web assets relative to the project root
set(WEB_ASSETS_PATH ${CMAKE_SOURCE_DIR}/main/web)

# Set the name for the generated SPIFFS image
set(SPIFFS_IMAGE_NAME spiffs)

# Register the command to build the SPIFFS image
spiffs_create_partition_image(
    ${SPIFFS_IMAGE_NAME}
    ${WEB_ASSETS_PATH}
    FLASH_IN_PROJECT
)
