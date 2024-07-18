#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_netif.h"
#include "connect_wifi.h"
#define PORT 1234
#define SERVER_IP "192.168.4.1"  // Replace with the server's IP address
#define SSID "minz"
#define PASS "12345678"
static const char *TAG = "udp_client";
int count=0;




void get_ip_address(char *ip_address, size_t ip_len)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_get_ip_info(netif, &ip_info);
        snprintf(ip_address, ip_len, IPSTR, IP2STR(&ip_info.ip));
    } else {
        snprintf(ip_address, ip_len, "0.0.0.0");
    }
}
void udp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char tx_buffer[128];
    char addr_str[20];
    char ip_address[16];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        ESP_LOGE(TAG, "Unable to set broadcast option: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);
    int len=0;
    while (1) {
        // ESP_LOGI(TAG, "Waiting for broadcast data");
        struct sockaddr_in6 source_addr;
        socklen_t socklen = sizeof(source_addr);
        while (1){
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        if (len>0){
            break;
        }
        }
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            break;
        } else {
            // rx_buffer[len] = 0;?
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            ESP_LOGI(TAG, "Received %d bytes from %s: %s", len, addr_str, rx_buffer);
            // ESP_LOGI(TAG, "%s", rx_buffer);
            // Send response back to server
            count++;


            // get_ip_address(ip_address, sizeof(ip_address));

            snprintf(tx_buffer, sizeof(tx_buffer), "Response from %s  : %d","esp033333", count);
            err = sendto(sock, tx_buffer, strlen(tx_buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }
            ESP_LOGW(TAG, "Response sent to server %d", count);
        }
    }

    if (sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(sock, 0);
        close(sock);
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
     wifi_init_sta(SSID,PASS);

    // ESP_ERROR_CHECK(nvs_flash_init());
    xTaskCreate(udp_client_task, "udp_client_task", 4096, NULL, 5, NULL);
}
