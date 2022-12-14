#include <cstdio>
#include <cJSON.h>
#include <sys/stat.h>
#include <esp_crc.h>
#include <esp_littlefs.h>

#include "config.hpp"
#include "global.hpp"

static cJSON * root;

long Config::get_val(const char * sub, const char * name, long default_value, long min, long max) 
{
  long val = default_value;

  if (sub[0]) {
    cJSON * sub_obj = cJSON_GetObjectItem(root, sub);
    if (sub_obj != nullptr) {
      cJSON * elem = cJSON_GetObjectItem(sub_obj, name);
      if ((elem != nullptr) && (elem->type == cJSON_Number)) {
        long v = elem->valuedouble;
        if ((v >= min) && (v <= max)) {
          val = v;
        }
        else {
          ESP_LOGW(TAG, "===> get_val value %ld is not inside limits [%ld, %ld]: %s %s", v, min, max, sub, name);
        }
      }
      else {
        ESP_LOGW(TAG, "===> get_val item %s %s not found.", sub, name);
      }
    }
  }
  else {
    cJSON * elem = cJSON_GetObjectItem(root, name);
    if ((elem != nullptr) && (elem->type == cJSON_Number)) {
      long v = elem->valuedouble;
      if ((v >= min) && (v <= max)) {
        val = v;
      }
      else {
        ESP_LOGW(TAG, "===> get_val value %ld is not inside limits [%ld, %ld]: %s %s", v, min, max, sub, name);
      }
    }
    else {
      ESP_LOGW(TAG, "===> get_val item %s %s not found.", sub, name);
    }
  }
  
  ESP_LOGD(TAG, "===> get_val: %s %s (%ld): %ld", sub, name, default_value, val);

  return val;
}

void Config::get_str(char * loc, const char * sub, const char * name, const char * default_value, int max_length)
{
  loc[0] = 0;

  if (sub[0]) {
    cJSON * sub_obj = cJSON_GetObjectItem(root, sub);
    if (sub_obj != nullptr) {
      cJSON * elem = cJSON_GetObjectItem(sub_obj, name);
      if ((elem != nullptr) && (elem->type == cJSON_String)) {
        int len = strlen(elem->valuestring);
        if (len <= max_length) {
          strncpy(loc, elem->valuestring, len);
          loc[max_length] = 0;
        }
      }
    }
  }
  else {
    cJSON * elem = cJSON_GetObjectItem(root, name);
    if ((elem != nullptr) && (elem->type == cJSON_String)) {
      int len = strlen(elem->valuestring);
      if (len <= max_length) {
        strncpy(loc, elem->valuestring, len);
        loc[max_length] = 0;
      }
    }
  }

  if (loc[0] == 0) {
    ESP_LOGW(TAG, "===> get_str item %s %s not found.", sub, name);
    int len = strlen(default_value);
    if (len > max_length) len = max_length;
    strncpy(loc, default_value, len);
    loc[max_length] = 0;
  }

  ESP_LOGD(TAG, "===> get_str: %s %s (%s): %s", sub, name, default_value, loc);
}

esp_err_t Config::retrieve_cfg()
{
  esp_vfs_littlefs_conf_t conf = {
    .base_path = "/littlefs",
    .partition_label = "littlefs",
    .format_if_mount_failed = false,
    .dont_mount = false
  };

  esp_err_t ret = esp_vfs_littlefs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount LittleFS filesystem or wrong format.");
    } 
    else {
      ESP_LOGE(TAG, "Failed to find LittleFS partition.");
    } 
    return ESP_FAIL;
  }

  struct stat st;

  if (stat("/littlefs/config.json", &st) == 0) {

    ESP_LOGD(TAG, "Config file size: %d", (int) st.st_size);
    
    FILE * f = fopen("/littlefs/config.json", "r");
    if (f == nullptr) {
      ESP_LOGE(TAG, "Failed to open /littlefs/config.json file for reading");
      esp_vfs_littlefs_unregister("littlefs");
      return ESP_FAIL;
    }

    char * cfg_file_content = (char *) malloc((size_t)(st.st_size + 1));

    if (cfg_file_content == nullptr) {
      ESP_LOGE(TAG, "Unable to allocate space for config file.");
      fclose(f);
      esp_vfs_littlefs_unregister("littlefs");
      return ESP_FAIL;
    }

    int size = fread(cfg_file_content, 1, (size_t) st.st_size, f);
    if (size != ((size_t) st.st_size)) {
      ESP_LOGE(TAG, "Unable to read config file content (%d).", size);
      fclose(f);
      esp_vfs_littlefs_unregister("littlefs");
      free(cfg_file_content);
      return ESP_FAIL;
    }

    fclose(f);
    esp_vfs_littlefs_unregister("littlefs");

    cfg_file_content[st.st_size] = 0;
    root = cJSON_Parse(cfg_file_content);

    if (root == nullptr) {
      free(cfg_file_content);
      ESP_LOGE(TAG, "Unable to parse config file.");
      return ESP_FAIL;
    }

    memset(&cfg, 0, sizeof(CFG));

    cfg.log_level         = (esp_log_level_t) get_val("", "log_level",   CONFIG_IOT_LOG_LEVEL,          0,     5);
    esp_log_level_set(TAG, cfg.log_level);
    cfg.watchdog_interval = get_val(                 "", "watchdog_interval", CONFIG_IOT_WATCHDOG_INTERVAL, 60, 84600);
                            get_str(cfg.device_name, "", "device_name", CONFIG_IOT_DEVICE_NAME,              32);
                            get_str(cfg.topic_name,  "", "topic_name",  CONFIG_IOT_TOPIC_NAME,               32);

    #ifdef CONFIG_IOT_ENABLE_UDP
      cfg.udp.port            = get_val(                        "udp", "port",            CONFIG_IOT_UDP_PORT,         1, 65535);
      cfg.udp.max_pkt_size    = get_val(                        "udp", "max_pkt_size",    CONFIG_IOT_UDP_MAX_PKT_SIZE, 2,  1450);
                                get_str(cfg.udp.gateway_address "udp", "gateway_address", CONFIG_IOT_GATEWAY_ADDRESS ,      128);
                                get_str(cfg.udp.wifi_ssid,      "udp", "wifi_ssid",       CONFIG_IOT_WIFI_UDP_STA_SSID,      32);
                                get_str(cfg.udp.wifi_psw,       "udp", "wifi_psw",        CONFIG_IOT_WIFI_UDP_STA_PASS,      32);
    #endif

    #ifdef CONFIG_IOT_ENABLE_ESP_NOW
      #ifdef CONFIG_IOT_ENCRYPT
        #define ENCRYPT 1
      #else
        #define ENCRYPT 0
      #endif
      #ifdef CONFIG_IOT_ESPNOW_ENABLE_LONG_RANGE
        #define LONG_RANGE 1
      #else
        #define LONG_RANGE 0
      #endif
      cfg.esp_now.encryption_enabled = get_val(                                 "esp_now", "encryption_enabled",  ENCRYPT,                        0,   1);
      cfg.esp_now.channel            = get_val(                                 "esp_now", "channel",             CONFIG_IOT_CHANNEL,             0,  11);
      cfg.esp_now.max_pkt_size       = get_val(                                 "esp_now", "max_pkt_size",        CONFIG_IOT_ESPNOW_MAX_PKT_SIZE, 1, 248);
      cfg.esp_now.enable_long_range  = get_val(                                 "esp_now", "enable_long_range",   LONG_RANGE,                     0,   1);
                                       get_str(cfg.esp_now.primary_master_key,  "esp_now", "primary_master_key",  CONFIG_IOT_ESPNOW_PMK,              16);
                                       get_str(cfg.esp_now.local_master_key,    "esp_now", "local_master_key",    CONFIG_IOT_ESPNOW_LMK,              16);
                                       get_str(cfg.esp_now.gateway_ssid_prefix, "esp_now", "gateway_ssid_prefix", CONFIG_IOT_GATEWAY_SSID_PREFIX,     16);
    #endif

    cfg.crc = esp_crc16_le(UINT16_MAX, (const uint8_t *)(&cfg), sizeof(CFG) - 2);
    cJSON_Delete(root);
    free(cfg_file_content);
  }
  else {
    ESP_LOGE(TAG, "Config file /littlefs/config.json not found!");
    esp_vfs_littlefs_unregister("littlefs");
    return ESP_FAIL;
  }
    
  return ESP_OK;
}

esp_err_t Config::init(bool reset)
{
  esp_log_level_set(TAG, CONFIG_IOT_LOG_LEVEL);

  uint16_t crc = esp_crc16_le(UINT16_MAX, (const uint8_t *)(&cfg), sizeof(CFG) - 2);
  if (reset || (crc != cfg.crc)) {
    if (reset) {
      ESP_LOGI(TAG, "JSON Config file retrieval.");
    }
    else {
      ESP_LOGW(TAG, "Config CRC is wrong! JSON file retrieval!");
    }
    return retrieve_cfg();
  }

  esp_log_level_set(TAG, cfg.log_level);
  return ESP_OK;
}