#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H
#include <stdio.h>

void wifi_init(void);

bool wifi_is_connected(void);
void wifi_open_socket(void);
void wifi_close_socket(void);

void wifi_send_data(uint8_t* pBuffer, uint32_t len);

 #endif