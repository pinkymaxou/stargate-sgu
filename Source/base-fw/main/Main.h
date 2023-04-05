#ifndef _MAIN_H_
#define _MAIN_H_

#include "SGUBRComm.h"

extern SGUBRCOMM_SHandle g_sSGUBRCOMMHandle;

void MAIN_GetWiFiSTAIP(esp_netif_ip_info_t* pIPInfo);

void MAIN_GetWiFiSoftAPIP(esp_netif_ip_info_t* pIPInfo);

void MAIN_RequestReboot();

#endif