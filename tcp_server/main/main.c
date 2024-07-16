#include "lib_tcp.h"
#include "connect_wifi.h"
#include "pub_sub_client.h"
// #include "esp_system.h"
// #include "esp_crt_bundle.h"
// #include "esp_log.h"

// #include "esp_http_client.h"

// #define WIFI_SSID "Aruba_Wifi"
// #define WIFI_PASS "123456789"
#define WIFI_SSID "Hoai Nam"
#define WIFI_PASS "123456789"
#define PING_URL "http://35.240.204.122"
// #define PING_URL "https://github.com"
#define WIFI_SSIDD "Hoai Nam 123"

void app_main(void)
{
    uart_configg();
    wifi_init_softap(WIFI_SSID,WIFI_PASS);
    // wifi_init_sta(WIFI_SSID,WIFI_PASS); 

    // https_ping(PING_URL);
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
//     xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET6, 5, NULL);
    xTaskCreate(uart_event_task, "uart_event_task", 8192, NULL, 12, NULL);
//     while (1)
//     {
//             vTaskDelay(pdMS_TO_TICKS(5000));
// int state_internet=check_internet();
// if (state_internet){
//     ESP_LOGI(TAG,"have internet");
// }
// else{
//     ESP_LOGE(TAG,"No Internet");
// }
//     }
}

