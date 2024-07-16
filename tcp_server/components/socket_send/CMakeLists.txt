idf_component_register(
    SRCS "src/socket_send.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event nvs_flash esp_http_client protocol_examples_common mbedtls esp_timer driver

)


