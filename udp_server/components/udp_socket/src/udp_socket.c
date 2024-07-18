#include "udp_socket.h"
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "esp_timer.h"
static const char *TAG = "UDP_SOCKET";
int count=0;
#define EXAMPLE_MAX_STA_CONN 4
#define PORT 1234
#define BROADCAST_IP "255.255.255.255"

int start=0;
int stop=0;
int time_s=0;

void tranmister(void *pvParameters) {

    struct sockaddr_in broadcast_addr;
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    int sock = (int)pvParameters;

        char tx_buffer[128];
while(1){
          vTaskDelay(3000 / portTICK_PERIOD_MS);
            count++;
            snprintf(tx_buffer, sizeof(tx_buffer), "Send from server : %d", count);

            ESP_LOGW(TAG,"Send to broadcast: %d",count);
            // int err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
            start=esp_timer_get_time();
            int err = sendto(sock, tx_buffer,  sizeof(tx_buffer), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));

            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
             }}
}
static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];

    char addr_str[20];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

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

    struct sockaddr_storage source_addr;
    socklen_t socklen = sizeof(source_addr);




        xTaskCreate(tranmister, "tranmister", 4096, (void*)sock, 5, NULL);

    while (1) {
        // ESP_LOGI(TAG, "Waiting for data");
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            break;
        } else {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            rx_buffer[len] = 0;
            ESP_LOGI(TAG, "Received %d bytes from %s: %s", len, addr_str, rx_buffer);
            stop=esp_timer_get_time();
            time_s=stop-start;
            ESP_LOGI(TAG, "time= %d ms", time_s);
            // ESP_LOGI(TAG, "%s", rx_buffer);

        }
    }

    if (sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(sock, 0);
        close(sock);
    }

    vTaskDelete(NULL);
}

void create_task_udp_server(){
        xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);

}