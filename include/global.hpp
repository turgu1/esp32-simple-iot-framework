#pragma once

#include "config.hpp"

#include "wifi.hpp"
#include "nvs_mgr.hpp"
#include "iot.hpp"

#ifdef CONFIG_IOT_BATTERY_LEVEL
  #include "battery.hpp"
#endif

#ifdef CONFIG_IOT_ENABLE_UDP
  #include "udp_sender.hpp"
#endif

#ifdef CONFIG_IOT_ENABLE_ESP_NOW
  #include "esp_now.hpp"
#endif

#ifndef __GLOBAL__
  extern CFG cfg;
  extern Config config;
  
  #ifndef __IOT__
    extern IoT iot;
  #endif
  #ifndef __WIFI__
    extern Wifi   wifi;
  #endif
  #ifndef __NVS_MGR__
    extern NVSMgr nvs_mgr;
  #endif

  #ifdef CONFIG_IOT_ENABLE_UDP
    #ifndef __UDP__
      extern UDP udp;
    #endif
  #endif

  #ifdef CONFIG_IOT_BATTERY_LEVEL
    #ifndef __BATTERY__
      extern Battery battery;
    #endif
  #endif

  #ifdef CONFIG_IOT_ENABLE_ESP_NOW
    #ifndef __ESP_NOW__
      extern ESPNow esp_now;
    #endif
  #endif
#endif

extern uint32_t sequence_number;
extern uint32_t error_count;