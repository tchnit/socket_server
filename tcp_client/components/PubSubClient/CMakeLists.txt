idf_component_register(
    SRCS "src/pub_sub_client.c" 
    INCLUDE_DIRS "include" 
    REQUIRES nvs_flash esp_netif esp_event mqtt json
    
)
                    
# idf_component_register(
#     SRCS "src/connect_wifi.c"
#     INCLUDE_DIRS "include"
#     REQUIRES esp_wifi esp_event nvs_flash esp_http_client protocol_examples_common mbedtls esp_timer

# )
