
// #include <string.h>
// #include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
#include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_netif.h"
// #include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <stdio.h>
#include "freertos/queue.h"
#include "driver/uart.h"


#include "freertos/event_groups.h"
#include "sdkconfig.h"

#define PORT                        CONFIG_EXAMPLE_PORT
#define KEEPALIVE_IDLE              CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_EXAMPLE_KEEPALIVE_COUNT

static const char *TAG = "Sockets Server";
static const char *payload = "Message from  Server ";
int sock;

#define MAX_CONNECTIONS 5
void handle_connection(void *pvParameters) {
    sock = (int)pvParameters;
    char rx_buffer[128];
    while (1) {
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        } else if (len == 0) {
            ESP_LOGI(TAG, "Connection closed");
            break;
        } else {
            rx_buffer[len] = 0;
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
            // Send response

            // send(sock, "Message from  Server", len, 0);
            send(sock, payload, strlen(payload), 0);
                        ESP_LOGI(TAG, "Socket: %d", sock);

            // printf(sock);
        }
    }
    close(sock);
    vTaskDelete(NULL);
}

void send_sock(const void *payload){
    send(sock, payload, strlen(payload), 0);
}
void tcp_server_task(void *pvParameters) {
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    
    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
    }
    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }
    err = listen(listen_sock, MAX_CONNECTIONS);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }

    while (1) {
        struct sockaddr_in source_addr;
        uint addr_len = sizeof(source_addr);
        sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }
        // Handle connection in separate task
        xTaskCreate(handle_connection, "handle_connection", 4096, (void*)sock, 5, NULL);
        // xTaskCreate(uart_event_task, "uart_event_task", 8192, (void*)sock, 12, NULL);
    }
    close(listen_sock);
    vTaskDelete(NULL);
}