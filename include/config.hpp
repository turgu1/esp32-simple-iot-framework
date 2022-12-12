#pragma once

#include <lwip/sockets.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_now.h>

#if defined(CONFIG_IOT_ENABLE_UDP) && defined(CONFIG_IOT_ENABLE_ESP_NOW)
  #error "You must define only one of CONFIG_IOT_ENABLE_UDP or CONFIG_IOT_ENABLE_ESP_NOW"
#endif

#if !defined(CONFIG_IOT_ENABLE_UDP) && !defined(CONFIG_IOT_ENABLE_ESP_NOW)
  #error "You must define one of CONFIG_IOT_ENABLE_UDP or CONFIG_IOT_ENABLE_ESP_NOW"
#endif

typedef uint8_t MacAddr[6];
typedef char    MacAddrStr[18];

// Define the log level for all classes
// One of (ESP_LOG_<suffix>: NONE, ERROR, WARN, INFO, DEBUG, VERBOSE
//
// To take effect the CONFIG_LOG_MAXIMUM_LEVEL configuration parameter must
// be as high as the targeted log level. This is set in menuconfig:
//     Component config > Log output > Maximum log verbosity
constexpr const esp_log_level_t LOG_LEVEL = ESP_LOG_VERBOSE;

// Log Level

#if defined(CONFIG_IOT_LOG_NONE)
  #define CONFIG_IOT_LOG_LEVEL ESP_LOG_NONE
#elif defined(CONFIG_IOT_LOG_ERROR)
  #define CONFIG_IOT_LOG_LEVEL ESP_LOG_ERROR
#elif defined(CONFIG_IOT_LOG_WARN)
  #define CONFIG_IOT_LOG_LEVEL ESP_LOG_WARN
#elif defined(CONFIG_IOT_LOG_INFO)
  #define CONFIG_IOT_LOG_LEVEL ESP_LOG_INFO
#elif defined(CONFIG_IOT_LOG_DEBUG)
  #define CONFIG_IOT_LOG_LEVEL ESP_LOG_DEBUG
#elif defined(CONFIG_IOT_LOG_VERBOSE)
  #define CONFIG_IOT_LOG_LEVEL ESP_LOG_VERBOSE
#endif

// Router authorization mode

#if defined(CONFIG_IOT_WIFI_STA_WPA3)
  #define WIFI_STA_AUTH_MODE WIFI_AUTH_WPA3_PSK
#elif defined(CONFIG_IOT_WIFI_STA_WPA2)
  #define WIFI_STA_AUTH_MODE WIFI_AUTH_WPA2_PSK
#elif defined(CONFIG_IOT_WIFI_STA_WPA)
  #define WIFI_STA_AUTH_MODE WIFI_AUTH_WPA_PSK
#elif defined(CONFIG_IOT_WIFI_STA_WEP)
  #define WIFI_STA_AUTH_MODE WIFI_AUTH_WEP_PSK
#endif

struct CFG {
  long            watchdog_interval;   // CONFIG_IOT_WATCHDOG_INTERVAL
  char            device_name[33];     // CONFIG_IOT_DEVICE_NAME
  char            topic_name[33];      // CONFIG_IOT_TOPIC_NAME
  esp_log_level_t log_level;           // CONFIG_IOT_LOG_...

  #ifdef CONFIG_IOT_ENABLE_UDP
    struct UDP {
      uint16_t port;                   // CONFIG_IOT_UDP_PORT
      uint16_t max_pkt_size;           // CONFIG_IOT_UDP_MAX_PKT_SIZE
      char     gateway_address[129];   // CONFIG_IOT_GATEWAY_ADDRESS
      char     wifi_ssid[33];          // CONFIG_IOT_WIFI_UDP_STA_SSID
      char     wifi_psw[33];           // CONFIG_IOT_WIFI_UDP_STA_PASS
    } __attribute__((packed)) udp;
  #endif

  #ifdef CONFIG_IOT_ENABLE_ESP_NOW
    struct ESP_NOW {
      char     primary_master_key[17]; // CONFIG_IOT_ESPNOW_PMK
      char     local_master_key[17];   // CONFIG_IOT_ESPNOW_LMK
      char     gateway_ssid_prefix[17];// CONFIG_IOT_GATEWAY_SSID_PREFIX
      uint8_t  channel;                // CONFIG_IOT_CHANNEL
      uint16_t max_pkt_size;           // CONFIG_IOT_ESPNOW_MAX_PKT_SIZE
      bool     encryption_enabled;     // CONFIG_IOT_ENCRYPT
      bool     enable_long_range;      // CONFIG_IOT_ESPNOW_ENABLE_LONG_RANGE
    } __attribute__((packed)) esp_now;
  #endif

  uint16_t crc;
} __attribute__((packed));

class Config
{
  private:
    static constexpr char const * TAG = "Config Class";

    long           get_val(const char * sub, const char * name, long default_value, long min, long max);
    void           get_str(char * loc, const char * sub, const char * name, const char * default_value, int max_length);
    esp_err_t retrieve_cfg();

  public:
    esp_err_t         init(bool reset);
};