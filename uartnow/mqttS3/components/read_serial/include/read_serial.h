#ifndef READ_SERIAL_H
#define READ_SERIAL_H

#define STILL_CONNECTED_MSG         "slave_KEEP_connect"
#define STILL_CONNECTED_MSG_SIZE    (sizeof(STILL_CONNECTED_MSG) - 1)

void uart_config(void);
void uart_event_task(void);
void add_json(void);
void dump_uart(const char *message);



#endif