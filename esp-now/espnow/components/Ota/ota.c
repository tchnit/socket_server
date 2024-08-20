#include "ota.h"

const char *OTA_TAG = "HTTPS_OTA";
esp_http_client_config_t *g_client_config; //CONFIG
int version_number=1; //Just for testing
CheckVersion g_check_ptr;
char g_http_response_buffer[BUFFER_OTA_SIZE];
int g_http_response_buffer_index = 0;

void https_ota_event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == ESP_HTTPS_OTA_EVENT) 
    {
        switch (event_id) {
            case ESP_HTTPS_OTA_START:
                ESP_LOGI(OTA_TAG, "OTA started");
                break;
            case ESP_HTTPS_OTA_CONNECTED:
                ESP_LOGI(OTA_TAG, "Connected to server");
                break;
            case ESP_HTTPS_OTA_GET_IMG_DESC:
                ESP_LOGI(OTA_TAG, "Reading Image Description");
                break;
            case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
                ESP_LOGI(OTA_TAG, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
                break;
            case ESP_HTTPS_OTA_DECRYPT_CB:
                ESP_LOGI(OTA_TAG, "Callback to decrypt function");
                break;
            case ESP_HTTPS_OTA_WRITE_FLASH:
                ESP_LOGD(OTA_TAG, "Writing to flash: %d written", *(int *)event_data);
                break;
            case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
                ESP_LOGI(OTA_TAG, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
                break;
            case ESP_HTTPS_OTA_FINISH:
                ESP_LOGI(OTA_TAG, "OTA finish");
                break;
            case ESP_HTTPS_OTA_ABORT:
                ESP_LOGI(OTA_TAG, "OTA abort");
                break;
        }
    }
}

esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) 
    {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(OTA_TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(OTA_TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(OTA_TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(OTA_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(OTA_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (g_http_response_buffer_index + evt->data_len < BUFFER_OTA_SIZE) 
            {
                printf("version:%.*s",evt->data_len, (char *)evt->data);
                memcpy(g_http_response_buffer + g_http_response_buffer_index, evt->data, evt->data_len);
                g_http_response_buffer_index += evt->data_len;
            } else 
            {
                ESP_LOGE(OTA_TAG, "Buffer overflow");
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(OTA_TAG, "HTTP_EVENT_ON_FINISH");
             g_http_response_buffer[g_http_response_buffer_index] = '\0';
            printf("%.*s",g_http_response_buffer_index, (char *)g_http_response_buffer); 
            g_http_response_buffer_index=0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(OTA_TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}

STATE_VERSION default_check_version(void)
{
    //Just for testing
    esp_http_client_config_t ver_config= 
    {
        .url = "https://raw.githubusercontent.com/thong1a/OTA/main/test.txt", // URL của file README trên GitHub
        .event_handler = http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach, // Sử dụng bộ chứng chỉ
    }; 
    esp_http_client_handle_t client = esp_http_client_init(&ver_config);

    // Thực hiện yêu cầu GET
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) 
    {
        ESP_LOGI(OTA_TAG, "HTTP GET SUCCESSFULLY");
    } else 
    {
        ESP_LOGE(OTA_TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    g_http_response_buffer_index=0;
    if(atoi(g_http_response_buffer)>=version_number)
    {
        for(int i=0;i<g_http_response_buffer_index;i++)
            g_http_response_buffer[i]=' ';
        return eNEW_VERSION;    
    }
    for(int i=0;i<g_http_response_buffer_index;i++)
        g_http_response_buffer[i]=' ';
    return eNOT_CHANGE;
    // Đóng HTTP client
}

esp_err_t check_header_image(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) 
    {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) 
    {
        ESP_LOGI(OTA_TAG, "Running firmware version: %s", running_app_info.version);
    }
    ESP_LOGI(OTA_TAG, "New firmware version: %s", new_app_info->version);

#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
    if(strcmp(new_app_info->version,running_app_info.version)<=0) 
    {
        ESP_LOGW(OTA_TAG, "It's a lower version. We will not continue the update.");
        return ESP_FAIL;
    }
#endif
    return ESP_OK;
}

void ota_perform(void *pvParameter)
{
    while(g_check_ptr()!=eNEW_VERSION)
    {
        ESP_LOGI(OTA_TAG,"Waiting for new version");
        return;
    };
    ESP_LOGI(OTA_TAG, "Starting  OTA:");
    ErrorHandle error_func =(ErrorHandle) pvParameter;
    esp_err_t ota_finish_err = ESP_OK;
#ifdef CONFIG_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = 
    {
        .http_config = g_client_config,
#ifdef CONFIG_ENABLE_PARTIAL_HTTP_DOWNLOAD
        .partial_http_download = true,
        .max_http_request_size = CONFIG_EXAMPLE_HTTP_REQUEST_SIZE,
#endif
    };
    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(OTA_TAG, "ESP HTTPS OTA Begin failed");
        if(error_func!=NULL)
        {
            error_func(eOTA_BEGIN_FAILED,(int*)&err);
        }
        return;
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) 
    {
        ESP_LOGE(OTA_TAG, "esp_https_ota_read_img_desc failed");
        goto ota_end;
    }
    err = check_header_image(&app_desc);
    if (err != ESP_OK) 
    {
        ESP_LOGE(OTA_TAG, "image header verification failed");
        if(error_func!=NULL)
        {
            error_func(eVERIFICATION_FAILED,(int*)&err);
        }
        goto ota_end;
    }

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) 
        {
            break;
        }
        if(checkwifi_state()!=eWIFI_CONNECTED)
        {
            esp_wifi_connect();
            if(checkwifi_state()!=eWIFI_CONNECTED)
            {
                ESP_LOGE(OTA_TAG, "Trying to connect to wifi");
                if(error_func!=NULL)
                {
                    error_func(eWIFI_NOT_CONNECTED,NULL);
                }
                 return;
            }

        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        ESP_LOGE(OTA_TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) 
    {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(OTA_TAG, "Complete data was not received.");
        if(error_func!=NULL)
            error_func(eINCOMPLETE_DATA,NULL);
    } 
    else 
    {
        ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) 
        {
            ESP_LOGI(OTA_TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        } 
        else 
        {
            if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) 
            {
                ESP_LOGE(OTA_TAG, "Image validation failed, image is corrupted");
            }
            ESP_LOGE(OTA_TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
            return;
        }
    }

ota_end:
    esp_https_ota_abort(https_ota_handle);
    ESP_LOGE(OTA_TAG, "ESP_HTTPS_OTA upgrade failed");
    return;
}

void ota_init(esp_http_client_config_t *config, CheckVersion check)
{
    g_client_config=config;
        ESP_LOGI(OTA_TAG, "OTA example app_main start");
    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &https_ota_event_handler, NULL));
    if(check!=NULL)
        g_check_ptr=check;
    else
        g_check_ptr=default_check_version;    
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
#if CONFIG_CONNECT_WIFI
#if !CONFIG_BT_ENABLED
    /* Ensure to disable any WiFi power save mode, this allows best speed of data transform
     * and reducing download time of OTA
     */
    esp_wifi_set_ps(WIFI_PS_NONE);
#else
    /* WIFI_PS_MIN_MODEM is the default mode for WiFi Power saving. When both
     * WiFi and Bluetooth are running, WiFI modem has to go down, hence we
     * need WIFI_PS_MIN_MODEM. And as WiFi modem goes down, OTA download time
     * increases.
     */
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
#endif // CONFIG_BT_ENABLED
#endif // CONFIG_EXAMPLE_CONNECT_WIFI

#if CONFIG_BT_CONTROLLER_ENABLED && (CONFIG_BT_BLE_ENABLED || CONFIG_BT_NIMBLE_ENABLED)
    ESP_ERROR_CHECK(esp_ble_helper_init()); // Init function of bluetooth function
#endif
}

esp_err_t confirm_app(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
                ESP_LOGI(OTA_TAG, "App is valid, rollback cancelled successfully");
                    return ESP_OK;
            } else {
                ESP_LOGE(OTA_TAG, "Failed to cancel rollback");
                return ESP_FAIL;
            }
        }
    }
    return ESP_FAIL;
}

esp_err_t cancel_app(void)
{
    esp_err_t err = esp_ota_check_rollback_is_possible();
    if (err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "Rollback is not possible: %s", esp_err_to_name(err));
        return err;
    }
    else
    {
        esp_ota_mark_app_invalid_rollback_and_reboot();
        return ESP_OK;
    }
}