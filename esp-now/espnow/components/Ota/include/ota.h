#ifndef OTA_H
#define OTA_H

#include "global.h"
#include "wifi.h"
#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif
#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif
#if CONFIG_BT_BLE_ENABLED || CONFIG_BT_NIMBLE_ENABLED
#include "ble_api.h"
#endif

/**
 * @brief Enumeration representing error information for OTA operations.
 *
 * This enumeration defines different error types that can occur during OTA operations:
 * - OTA_BEGIN_FAILED: Indicates that the OTA update process failed to begin.
 * - VERIFICATION_FAILED: Indicates that verification of the new firmware image header failed.
 * - INCOMPLETE_DATA: Indicates that incomplete data was received during the OTA update process.
 */
typedef enum
{
    eOTA_BEGIN_FAILED    = 0,
    eVERIFICATION_FAILED = 1,
    eINCOMPLETE_DATA     = 2,
    eWIFI_NOT_CONNECTED  = 3
                        
} ERROR_INFOR;

/**
 * @brief Enumeration representing the state of firmware version comparison.
 *
 * This enumeration defines two states:
 * - NEW_VERSION: Indicates that a new version of firmware is available.
 * - NOT_CHANGE: Indicates that there is no new version available; the current version is up to date.
 */
typedef enum
{
    eNEW_VERSION  = 0,
    eNOT_CHANGE   = 1
} STATE_VERSION;

/**
 * @brief Typedef for error handling function pointer.
 *
 * This typedef defines a function pointer type `ErrorHandle` that points to a function
 * capable of handling errors related to OTA operations.
 *
 * @param error_code The error code indicating the type of error occurred (ERROR_INFOR enum).
 * @param data       Additional data associated with the error, typically used for error context or details.
 */
typedef void(*ErrorHandle)(ERROR_INFOR,void *);

/**
 * @brief Typedef for function pointer to check firmware version.
 *
 * This typedef defines a function pointer type `Check_Version` that points to a function
 * capable of checking the current firmware version and determining if a new version is available.
 *
 * @return State_version Enum value indicating the result of version comparison:
 *         - NEW_VERSION: Indicates a new firmware version is available.
 *         - NOT_CHANGE: Indicates no new firmware version is available; current version is up to date.
 */
typedef STATE_VERSION(*CheckVersion)(void);

extern  char g_http_response_buffer[BUFFER_OTA_SIZE];
extern int g_http_response_buffer_index;
extern const char *OTA_TAG;
extern esp_http_client_config_t *g_client_config;
extern int version_number; //Just for testing
extern CheckVersion g_check_ptr;

/**
 * @brief Perform an Over-The-Air (OTA) update.
 *
 * This function checks for a new firmware version and initiates an OTA update process if a new version is available.
 * It uses HTTPS to securely download the new firmware image and writes it to the appropriate partition.
 * If the update is successful, the device will restart with the new firmware.
 *
 * @param pvParameter Pointer to a user-defined error handling function.
 */
void ota_perform(void *pvParameter);

void ota_init(esp_http_client_config_t *config, CheckVersion check);

/**
 * @brief Check the firmware version from a remote server.
 *
 * This function performs an HTTP GET request to a specified URL to retrieve the
 * firmware version from a remote server. The version is compared with the current
 * firmware version, and the function returns the appropriate status indicating
 * whether a new version is available or not.
 * Warning: It's just for testing
 * @return state_version Indicating the version status:
 *         - NEW_VERSION: If the remote version is greater than the current version.
 *         - NOT_CHANGE: If the remote version is not greater than the current version.
 */
STATE_VERSION default_check_version(void);

/**
 * @brief HTTP event handler for the ESP HTTP client.
 *
 * This function handles various events that occur during an HTTP request.
 * It logs messages indicating the progress and status of the HTTP request.
 * Additionally, it processes the data received in the HTTP response.
 *
 * @param evt Pointer to the HTTP client event structure containing event-specific data.
 * @return ESP_OK on success.
 */
esp_err_t http_event_handler(esp_http_client_event_t *evt);

/**
 * @brief Event handler for HTTPS OTA events.
 *
 * This function handles various events during the HTTPS OTA update process.
 * It logs messages indicating the progress and status of the OTA update.
 *
 * @param arg User-defined argument passed to the event handler.
 * @param event_base Event base that identifies the event source.
 * @param event_id Event ID indicating the specific event that occurred.
 * @param event_data Pointer to event-specific data.
 */
void https_ota_event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);

/**
 * @brief Check the firmware version in the new application image header.
 *
 * This function compares the firmware version of the new application image
 * with the running firmware version. If the new firmware version is not greater
 * than the current version, the update is aborted (depending on the configuration).
 *
 * @param new_app_info Pointer to the application description structure of the new firmware.
 * @return
 *     - ESP_OK: If the version check is successful and the new firmware version is valid.
 *     - ESP_ERR_INVALID_ARG: If the new_app_info parameter is NULL.
 *     - ESP_FAIL: If the new firmware version is not greater than the running firmware version (when version check is enabled).
 */
esp_err_t check_header_image(esp_app_desc_t *new_app_info);

/**
 * @brief Confirm the validity of the current OTA application and cancel rollback if pending.
 *
 * This function checks the state of the running OTA application. If the state indicates
 * that the application is pending verification, it marks the application as valid and
 * cancels any pending rollback. It logs the success or failure of these operations.
 *
 * @return
 *     - ESP_OK if the application is confirmed valid and rollback cancellation succeeds.
 *     - ESP_FAIL if there was an error confirming the application or cancelling rollback.
 */
esp_err_t confirm_app(void);

/**
 * @brief Cancel the current application and initiate rollback if possible.
 *
 * This function checks if rollback is possible using `esp_ota_check_rollback_is_possible()`.
 * If rollback is possible, it marks the current application as invalid, initiates rollback,
 * and reboots the device. It logs the success or failure of these operations.
 *
 * @return
 *     - ESP_OK if rollback is initiated successfully.
 *     - ESP_ERR_OTA_ROLLBACK_FAILED if rollback is not possible or fails.
 */
esp_err_t cancel_app(void);
#endif





