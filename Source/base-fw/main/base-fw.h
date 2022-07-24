#ifndef _BASEFW_H_
#define _BASEFW_H_

#include "SGUBRComm.h"

extern SGUBRCOMM_SHandle g_sSGUBRCOMMHandle;

void BASEFW_GetWiFiSTAIP(esp_netif_ip_info_t* pIPInfo);

void BASEFW_GetWiFiSoftAPIP(esp_netif_ip_info_t* pIPInfo);

void BASEFW_RequestReboot();

#endif