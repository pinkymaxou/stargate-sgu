#ifndef _MAIN_H_
#define _MAIN_H_

#include "SGUBRComm.h"

extern SGUBRCOMM_SHandle g_sSGUBRCOMMHandle;

void MAIN_GetWiFiSTAIP(esp_netif_ip_info_t* pIPInfo);

int32_t MAIN_GetWiFiSTAIPv6(esp_ip6_addr_t if_ip6[CONFIG_LWIP_IPV6_NUM_ADDRESSES]);

void MAIN_GetWiFiSoftAPIP(esp_netif_ip_info_t* pIPInfo);

void MAIN_RequestReboot();

#endif