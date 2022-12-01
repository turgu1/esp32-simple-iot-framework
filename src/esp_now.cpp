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

RTC_NOINIT_ATTR uint32_t gateway_access_error_count;
RTC_NOINIT_ATTR bool     ap_failed;

bool              ESPNow::abort             = false;
QueueHandle_t     ESPNow::send_queue_handle = nullptr;
ESPNow::SendEvent ESPNow::send_event;

esp_err_t ESPNow::init()
{
  esp_err_t status;

  esp_log_level_set(TAG, CONFIG_IOT_LOG_LEVEL);

  send_queue_handle = xQueueCreate(5, sizeof(send_event));
  if (send_queue_handle == nullptr) {
    ESP_LOGE(TAG, "Unable to create send queue.");
    return ESP_FAIL;
  }

  // Retrieve AP info from nvs and check reset type

  nvs_mgr.get_nvs_data();
  esp_reset_reason_t reason = esp_reset_reason();

  if (reason != ESP_RST_DEEPSLEEP) {
    gateway_access_error_count = 0;
    ap_failed = false;
  }

  if (!((reason == ESP_RST_DEEPSLEEP) && nvs_mgr.is_data_valid() && !ap_failed)) {
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
  ESP_ERROR_CHECK(status = esp_now_set_pmk((const uint8_t *) CONFIG_IOT_ESPNOW_PMK));

  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(esp_now_peer_info_t));

  memcpy(peer.peer_addr, ap_mac_addr, 6);

  peer.channel   = CONFIG_IOT_CHANNEL;
  peer.ifidx     = (wifi_interface_t) ESP_IF_WIFI_STA;

  #ifdef CONFIG_IOT_ENCRYPT
    static_assert(sizeof(CONFIG_IOT_ESPNOW_LMK) == 17, "The Exerciser's LMK must be 16 characters long.");

    peer.encrypt   = true;
    memcpy(peer.lmk, CONFIG_IOT_ESPNOW_LMK, ESP_NOW_KEY_LEN);
  #else
    peer.encrypt   = false;
  #endif

  ESP_LOGD(TAG, "AP Peer MAC address: " MACSTR, MAC2STR(peer.peer_addr));

  ESP_ERROR_CHECK(status = esp_now_add_peer(&peer));
  ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_IOT_CHANNEL, WIFI_SECOND_CHAN_NONE));

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
  static struct {
    uint16_t crc;
    char data[CONFIG_IOT_ESPNOW_MAX_PKT_SIZE];
  } __attribute__((packed)) pkt;

  if (len > CONFIG_IOT_ESPNOW_MAX_PKT_SIZE) {
    ESP_LOGE(TAG, "Cannot send data of length %d, too long. Max is %d.", len, CONFIG_IOT_ESPNOW_MAX_PKT_SIZE);
    status = ESP_FAIL;
  }
  else {
    memcpy(pkt.data, data, len);
    pkt.crc = esp_crc16_le(UINT16_MAX, (uint8_t *)(pkt.data), len);

    if ((status = esp_now_send(ap_mac_addr, (const uint8_t *) &pkt, len+2)) != ESP_OK) {
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

  ESP_LOGD(TAG, "Scanning AP list to find SSID starting with [%s]...", CONFIG_IOT_APSSID_PREFIX);

  memset(&config, 0, sizeof(wifi_scan_config_t));
  config.channel = CONFIG_IOT_CHANNEL;
  config.scan_type = WIFI_SCAN_TYPE_ACTIVE;

  ESP_ERROR_CHECK(esp_wifi_scan_start(&config, true));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&count));

  ap_records = (wifi_ap_record_t *) malloc(sizeof(wifi_ap_record_t) * count);
  if (ap_records == nullptr) {
    ESP_LOGE(TAG, "Unable to allocate memory for ap_records.");
    return ESP_FAIL;
  }

  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&count, ap_records));

  int len = strlen(CONFIG_IOT_APSSID_PREFIX);

  for (int i = 0; i < count; i++) {
    if (strncmp((const char *) ap_records[i].ssid, CONFIG_IOT_APSSID_PREFIX, len) == 0) {
      memcpy(&ap_mac_addr, ap_records[i].bssid, sizeof(MacAddr)); 
      wifi.set_rssi(ap_records[i].rssi);
      ESP_LOGD(TAG, "Found AP SSID %s:" MACSTR, ap_records[i].ssid, MAC2STR(ap_mac_addr));
      ap_failed = false;
      gateway_access_error_count = 0;
      return ESP_OK;
    }
  }

  gateway_access_error_count++;
  ap_failed = true;
  int wait_time = pow(gateway_access_error_count, 4) * 10;
  if (wait_time > 86400) wait_time = 86400; // Don't wait for more than one day.
  ESP_LOGE(TAG, "Unable to find Gateway Access Point. Waiting for %d seconds...", wait_time);

  esp_deep_sleep(wait_time * 1e6);

  return ESP_FAIL;
}

void ESPNow::prepare_for_deep_sleep()
{
  esp_now_unregister_send_cb();
  esp_now_deinit();
}

#endif