#define __GLOBAL__ 1
#include "config.hpp"

#include "global.hpp"

RTC_NOINIT_ATTR CFG cfg;

Config config;
IoT    iot;
Wifi   wifi;
NVSMgr nvs_mgr;

#ifdef CONFIG_IOT_BATTERY_LEVEL
  Battery battery;
#endif

#ifdef CONFIG_IOT_ENABLE_UDP
  UDP udp;
#endif

#ifdef CONFIG_IOT_ENABLE_ESP_NOW
  ESPNow esp_now;
#endif

RTC_NOINIT_ATTR uint32_t sequence_number;
