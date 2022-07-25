#ifndef _FWCONFIG_H_
#define _FWCONFIG_H_

#define FWCONFIG_SOFTAP_WIFI_SSID_BASE ("SGU-Ring-%02X%02X%02X")
#define FWCONFIG_SOFTAP_WIFI_PASS      ""
#define FWCONFIG_SOFTAP_WIFI_CHANNEL   0
#define FWCONFIG_SOFTAP_MAX_CONN       10

#define FWCONFIG_MASTERBASE_SSID "SGU-Base-F800C9"
#define FWCONFIG_MASTERBASE_PASS ""

#define FWCONFIG_HOLDPOWER_PIN 26
#define FWCONFIG_HOLDPOWER_DELAY_MS (300*1000)

#define FWCONFIG_SWITCH_PIN 21
#define FWCONFIG_SWITCH_HOLDDELAY_MS (1500)

// WS1228 as sanity led
#define FWCONFIG_WS1228B_ISACTIVE (1)
#define FWCONFIG_WS1228B_PIN 18
#define FWCONFIG_WS1228B_RMT_TX_CHANNEL RMT_CHANNEL_0
#define FWCONFIG_WS1228B_LEDCOUNT (45)

#endif