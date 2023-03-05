#ifndef _WEBSERVER_H
#define _WEBSERVER_H

#include <esp_http_server.h>

void WEBSERVER_Init();

bool WEBSERVER_GetIsReceivingOTA();

#endif
