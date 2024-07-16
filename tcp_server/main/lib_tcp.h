
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
#include "protocol_examples_common.h"

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
static void byte_to_hex_str();
#define MAX_CONNECTIONS 5



// static void do_retransmit(const int sock)
// {
//     int len;
//     char rx_buffer[128];
//     // struct msghdr messa;
//     char hex_str[256];
//     do {
//         len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
//         //  len = recvmsg(sock, messa, 0);
//         int err = send(sock, payload, strlen(payload), 0);

//         if (len < 0) {
//             ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
//         } else if (len == 0) {
//             ESP_LOGW(TAG, "Connection closed");
//         } else {
//             rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
//             ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
//             byte_to_hex_str((uint8_t *)rx_buffer, len, hex_str, sizeof(hex_str));
//             ESP_LOGI(TAG, "HEX: %s", hex_str);

//             // int to_write = len;
//             // ESP_LOGI(TAG, "socket %d ", sock);
//         }
//     } while (len > 0);
// }


// #define SOCK_STREAM 5


void handle_connection(void *pvParameters) {
    sock = (int)pvParameters;
    char rx_buffer[128];
    while (1) {
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        send(sock, payload, strlen(payload), 0);
                        ESP_LOGI(TAG, "Socket: %d", sock);
        // int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        // if (len < 0) {
        //     ESP_LOGE(TAG, "recv failed: errno %d", errno);
        //     break;
        // } else if (len == 0) {
        //     ESP_LOGI(TAG, "Connection closed");
        //     break;
        // } else {
        //     rx_buffer[len] = 0;
        //     ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
        //     // Send response

        //     // send(sock, "Message from  Server", len, 0);
            

        //     // printf(sock);
        // }
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
    ESP_LOGI(TAG, "listen Socket");
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create  socket: errno %d", errno);
        vTaskDelete(NULL);
    }
    
    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        ESP_LOGI(TAG, "bind Socket");

    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(listen_sock); 
        vTaskDelete(NULL);
    }
    err = listen(listen_sock, MAX_CONNECTIONS);
        ESP_LOGI(TAG, "listen ");

    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }

    while (1) {
        struct sockaddr_in source_addr;
        uint addr_len = sizeof(source_addr);
        sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        ESP_LOGI("accept", "Socket: %d", sock);
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




static QueueHandle_t uart0_queue;
#define EX_UART_NUM UART_NUM_0
#define PATTERN_CHR_NUM    (3)        
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)


void uart_configg(){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
     //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);
    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //Set uart pattern detect function.
    // uart_enable_pattern_det_baud_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    // uart_pattern_queue_reset(EX_UART_NUM, 20);
    //Create a task to handler UART event from ISR

}


static void uart_event_task(void *pvParameters)
{   
    // int sock = (int)pvParameters;
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    while (true){
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            switch (event.type) {
                case UART_DATA:
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                                            ESP_LOGI(TAG, "Socket: %d", sock);

                    ESP_LOGI(TAG, "Send %d bytes : %s",event.size,dtmp);
                    send(sock, dtmp,event.size, 0);
                    send_sock(dtmp);
                    // send(sock, payload, strlen(payload), 0);
                    break;
                default:
                    ESP_LOGE(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
    // free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}


void byte_to_hex_str(const uint8_t* bytes, size_t len, char* hex_str, size_t hex_str_len) {
    for (size_t i = 0; i < len; ++i) {
         snprintf(hex_str + i * 3, hex_str_len - i * 3, " %02x ", bytes[i]);
        // snprintf(hex_str + i * 2, hex_str_len - i * 2, " %02x ", bytes[i]);
    }
}
