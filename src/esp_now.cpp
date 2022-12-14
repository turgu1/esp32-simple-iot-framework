#include <esp_sleep.h>

#include "config.hpp"

#ifdef CONFIG_IOT_ENABLE_ESP_NOW

#include <cstring>
#include <cmath>
#include <esp_crc.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <assert.h>

#include "utils.hpp"
#include "esp_now.hpp"

#define __ESP_NOW__
#include "global.hpp"

#undef __ESP_NOW__

RTC_NOINIT_ATTR bool     ap_failed;
RTC_NOINIT_ATTR uint32_t gateway_access_error_count;

bool              ESPNow::abort             = false;
QueueHandle_t     ESPNow::send_queue_handle = nullptr;
ESPNow::SendEvent ESPNow::send_event;

esp_err_t ESPNow::init()
{
  esp_err_t status;

  esp_log_level_set(TAG, cfg.log_level);

  send_queue_handle = xQueueCreate(5, sizeof(send_event));
  if (send_queue_handle == nullptr) {
    ESP_LOGE(TAG, "Unable to create send queue.");
    return ESP_FAIL;
  }

  // Retrieve AP info from nvs and check reset type

  nvs_mgr.get_nvs_data();

  if (iot.was_reset()) {
    gateway_access_error_count = 0;
    ap_failed = false;
  }

  if (!(nvs_mgr.is_data_valid() && !ap_failed && !iot.was_reset())) {
    ESP_ERROR_CHECK(search_ap());
    NVSMgr::NVSData nvs_data;
    memcpy(&nvs_data.gateway_mac_addr, &ap_mac_addr, 6);
    nvs_data.rssi = wifi.get_rssi();
    ESP_ERROR_CHECK(nvs_mgr.set_nvs_data(&nvs_data));
  }
  else {
    wifi.set_rssi(nvs_mgr.get_data()->rssi);
    memcpy(&ap_mac_addr, nvs_mgr.get_data()->gateway_mac_addr, 6);
  }

  static_assert(sizeof(CONFIG_IOT_ESPNOW_PMK) == 17, "The Exerciser's PMK must be 16 characters long.");

  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_send_cb(send_handler));
  ESP_ERROR_CHECK(status = esp_now_set_pmk((const uint8_t *) cfg.esp_now.primary_master_key));

  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(esp_now_peer_info_t));

  memcpy(peer.peer_addr, ap_mac_addr, 6);

  peer.channel   = cfg.esp_now.channel;
  peer.ifidx     = (wifi_interface_t) ESP_IF_WIFI_STA;

  if (cfg.esp_now.encryption_enabled) {
    static_assert(sizeof(CONFIG_IOT_ESPNOW_LMK) == 17, "The Exerciser's LMK must be 16 characters long.");

    peer.encrypt   = true;
    memcpy(peer.lmk, cfg.esp_now.local_master_key, ESP_NOW_KEY_LEN);
  }
  else {
    peer.encrypt   = false;
  }

  ESP_LOGD(TAG, "AP Peer MAC address: " MACSTR, MAC2STR(peer.peer_addr));

  ESP_ERROR_CHECK(status = esp_now_add_peer(&peer));
  ESP_ERROR_CHECK(esp_wifi_set_channel(cfg.esp_now.channel, WIFI_SECOND_CHAN_NONE));

  return status;
}

void ESPNow::send_handler(const uint8_t * mac_addr, esp_now_send_status_t status)
{
  ESP_LOGD(TAG, "Send Event for " MACSTR ": %s.", MAC2STR(mac_addr), status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAILED");

  if (send_queue_handle != nullptr) {
    memcpy(send_event.mac_addr, mac_addr, 6);
    send_event.status = (status == ESP_NOW_SEND_SUCCESS) ? ESP_OK : ESP_FAIL;
    if (xQueueSend(send_queue_handle, &send_event, 0) != pdTRUE) {
      ESP_LOGW(TAG, "Message Queue is full, message is lost.");
    }
  }
}

esp_err_t ESPNow::send(const uint8_t * data, int len)
{
  esp_err_t status;
  static struct PKT {
    uint16_t crc;
    char data[0];
  } __attribute__((packed)) * pkt;

  if (len > cfg.esp_now.max_pkt_size) {
    ESP_LOGE(TAG, "Cannot send data of length %d, too long. Max is %d.", len, cfg.esp_now.max_pkt_size);
    status = ESP_FAIL;
  }
  else {
    pkt = (PKT *) malloc(len + 2);
    if (pkt == nullptr) {
      ESP_LOGE(TAG, "Unable to allocate memory for PKT struct.");
      return ESP_FAIL;
    }
    memcpy(pkt->data, data, len);
    pkt->crc = esp_crc16_le(UINT16_MAX, (uint8_t *)(pkt->data), len);
    status = esp_now_send(ap_mac_addr, (const uint8_t *) pkt, len+2);
    free(pkt);
    
    if (status != ESP_OK) {
      ESP_LOGE(TAG, "Unable to send ESP-NOW packet: %s.", esp_err_to_name(status));
      uint8_t primary_channel;
      wifi_second_chan_t secondary_channel;
      esp_wifi_get_channel(&primary_channel, &secondary_channel);
      ESP_LOGE(TAG, "Wifi channels: %d %d.", primary_channel, secondary_channel);
    }
  }

  return status;
}

esp_err_t ESPNow::search_ap()
{
  wifi_scan_config_t config;
  wifi_ap_record_t * ap_records;
  uint16_t count;

  ESP_LOGD(TAG, "Scanning AP list to find SSID starting with [%s]...", cfg.esp_now.gateway_ssid_prefix);

  memset(&config, 0, sizeof(wifi_scan_config_t));
  config.channel   = cfg.esp_now.channel;
  config.scan_type = WIFI_SCAN_TYPE_ACTIVE;

  ESP_ERROR_CHECK(esp_wifi_scan_start(&config, true));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&count));

  ESP_LOGD(TAG, "Number of SSID found: %d", count);
  
  ap_records = (wifi_ap_record_t *) malloc(sizeof(wifi_ap_record_t) * count);
  if (ap_records == nullptr) {
    ESP_LOGE(TAG, "Unable to allocate memory for ap_records.");
    return ESP_FAIL;
  }

  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&count, ap_records));

  int len = strlen(cfg.esp_now.gateway_ssid_prefix);

  for (int i = 0; i < count; i++) {
    ESP_LOGD(TAG, "SSID -> %s ...", ap_records[i].ssid);
    if (strncmp((const char *) ap_records[i].ssid, cfg.esp_now.gateway_ssid_prefix, len) == 0) {
      memcpy(&ap_mac_addr, ap_records[i].bssid, sizeof(MacAddr)); 
      wifi.set_rssi(ap_records[i].rssi);
      ESP_LOGD(TAG, "Found AP SSID %s:" MACSTR, ap_records[i].ssid, MAC2STR(ap_mac_addr));
      ap_failed = false;
      gateway_access_error_count = 0;
      return ESP_OK;
    }
  }

  gateway_access_error_count++;
  iot.increment_error_count();
  ap_failed = true;
  int wait_time = pow(gateway_access_error_count, 4) * 10;
  if (wait_time > 86400) wait_time = 86400; // Don't wait for more than one day.
  ESP_LOGE(TAG, "Unable to find Gateway Access Point. Waiting for %d seconds...", wait_time);

  iot.prepare_for_deep_sleep();
  esp_deep_sleep(wait_time * 1e6);

  return ESP_FAIL;
}

void ESPNow::prepare_for_deep_sleep()
{
  esp_now_unregister_send_cb();
  esp_now_deinit();
}

#endif