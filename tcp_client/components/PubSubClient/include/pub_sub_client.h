#ifndef PUB_SUBCLIENT_H
#define PUB_SUBCLIENT_H
// #include "global.h"

#include "mqtt_client.h"
#include "cJSON.h"
#include "string.h"
#include "esp_log.h"


void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void parse_and_add_to_json(const char *input_string, cJSON *json_obj);
void data_to_mqtt(char *data, char *topic,int delay_time_ms,int qos);
void mqtt_init(char *broker_uri, char *username, char *client_id);
void subcribe_to_topic(char *topic,int qos);
void get_data_subcribe_topic( uint16_t *data);
void mqtt_subcriber(esp_mqtt_event_handle_t event);
#endif // PUB_SUBCLIENT
