#include <esp_netif.h>

#include "wifi.hpp"

// Class variables

int8_t        Wifi::rssi                    = 0;

#ifdef CONFIG_IOT_ENABLE_UDP
  std::mutex  Wifi::mutex                   = {};
  Wifi::State Wifi::state                   = State::NOT_INITIALIZED;
  uint32_t    Wifi::ip                      = 0;
  char        Wifi::ip_cstr[20]             = "0.0.0.0";
#endif

// Wifi Contructor

Wifi::Wifi(void)
{
  esp_log_level_set(TAG, CONFIG_IOT_LOG_LEVEL);

  #ifdef CONFIG_IOT_ENABLE_UDP
    mac_addr_cstr[0] = 0;
    wifi_init_cfg    = WIFI_INIT_CONFIG_DEFAULT();
    memset(&wifi_sta_cfg, 0, sizeof(wifi_config_t));
  #endif
}

#ifdef CONFIG_IOT_ENABLE_UDP
  void Wifi::wifi_event_handler(void             * arg, 
                                esp_event_base_t   event_base,
                                int32_t            event_id, 
                                void             * event_data)
  {
    if (event_base == WIFI_EVENT) {
      const wifi_event_t event_type = static_cast<wifi_event_t>(event_id);

      switch (event_type) {
        case WIFI_EVENT_STA_START: {
          std::lock_guard<std::mutex> state_guard(mutex);
          state = State::READY_TO_CONNECT;
          show_state();
          connect();
          break;
        }

        case WIFI_EVENT_STA_CONNECTED: {
          std::lock_guard<std::mutex> state_guard(mutex);
          state = State::WAITING_FOR_IP;
          show_state();
          break;
        }

        case WIFI_EVENT_STA_DISCONNECTED: {
          std::lock_guard<std::mutex> state_guard(mutex);
          state = State::DISCONNECTED;
          show_state();
          connect();
          break;
        }

        default:
          break;
      }
    }
  }

  void Wifi::ip_event_handler(void             * arg, 
                              esp_event_base_t   event_base,
                              int32_t            event_id, 
                              void             * event_data)
  {
    if (event_base == IP_EVENT) {
      const ip_event_t event_type = static_cast<ip_event_t>(event_id);

      switch (event_type) {
        case IP_EVENT_STA_GOT_IP: {
          std::lock_guard<std::mutex> state_guard(mutex);
          state = State::CONNECTED;

          wifi_ap_record_t ap;
          esp_err_t status = esp_wifi_sta_get_ap_info(&ap);
          if (status == ESP_OK) {
            rssi = ap.rssi;
            ESP_LOGD(TAG, "RSSI: %d.", rssi);
          }
          else {
            ESP_LOGE(TAG, "Unable to retrieve remote ap info: %s.", esp_err_to_name(status));
          }

          tcpip_adapter_ip_info_t ipInfo;        
          status = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);   
          if (status == ESP_OK) {
            ip = ipInfo.ip.addr;
            snprintf(ip_cstr, sizeof(ip_cstr), IPSTR, IP2STR(&ipInfo.ip));
            ESP_LOGD(TAG, "IP Address: %s.", ip_cstr);
          } 
          else {
            ESP_LOGE(TAG, "Unable to retrieve IP Address: %s.", esp_err_to_name(status));
          }    

          show_state();
          break;
        }

        case IP_EVENT_STA_LOST_IP: {
          std::lock_guard<std::mutex> state_guard(mutex);
          if (state != State::DISCONNECTED) {
            state = State::WAITING_FOR_IP;
            show_state();
          }
          break;
        }

        default:
          break;
      }
    }
  }

  esp_err_t Wifi::connect(void)
  {
    esp_err_t status = ESP_OK;

    switch (state) {
      case State::READY_TO_CONNECT:
      case State::DISCONNECTED:
        status = esp_wifi_connect();
        if (status == ESP_OK) {
          state = State::CONNECTING;
          show_state();
        }
        else {
          ESP_LOGE(TAG, "Unable to start wifi connection: %s.", esp_err_to_name(status));
        }
        break;
          
      case State::CONNECTING:
      case State::WAITING_FOR_IP:
      case State::CONNECTED:
        break;

      case State::NOT_INITIALIZED:
      case State::INITIALIZED:
      case State::ERROR:
        ESP_LOGE(TAG, "Connecting at a wrong time.");
        show_state();
        status = ESP_FAIL;
        break;
    }

    return status;
  }

  void Wifi::show_state()
  {
    const char * msg = nullptr;
    switch (state) {
      case State::READY_TO_CONNECT:
        msg = "READY_TO_CONNECT";
        break;
      case State::DISCONNECTED:
        msg = "DISCONNECTED";
        break;
      case State::CONNECTING:
        msg = "CONNECTING";
        break;
      case State::WAITING_FOR_IP:
        msg = "WAITING_FOR_IP";
        break;
      case State::ERROR:
        msg = "ERROR";
        break;
      case State::CONNECTED:
        msg = "CONNECTED";
        break;
      case State::NOT_INITIALIZED:
        msg = "NOT_INITIALIZED";
        break;
      case State::INITIALIZED:
        msg = "INITIALIZED";
        break;
      default:
        msg = "UNKNOWN!!!";
    }  

    ESP_LOGD(TAG, "State: %s", msg);
  }
#endif

esp_err_t Wifi::init()
{
  #ifdef CONFIG_IOT_ENABLE_ESP_NOW

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    #if CONFIG_IOT_ESPNOW_ENABLE_LONG_RANGE
      #pragma message "----> INFO: EXERCISER LONG RANGE ENABLED <----"
      ESP_ERROR_CHECK(esp_wifi_set_protocol(
        (wifi_interface_t) ESP_IF_WIFI_STA, 
        WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR));
    #else
      #pragma message "----> INFO: EXERCISER LONG RANGE DISABLED <----"
    #endif

    return ESP_OK;

  #else // CONFIG_IOT_ENABLE_UDP

    state = State::NOT_INITIALIZED;

    std::lock_guard<std::mutex> mutx_guard(mutex);

    if (mac_addr_cstr[0] == 0) {
      if (retrieve_mac() != ESP_OK) {
        ESP_LOGE(TAG, "Unable to retrieve Mac Adress.");
        return ESP_FAIL;
      }
    }

    ESP_ERROR_CHECK(esp_netif_init());

    if (esp_netif_create_default_wifi_sta() == nullptr) {
      ESP_LOGE(TAG, "Unable to create default wifi STA.");
      return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    memcpy(wifi_sta_cfg.sta.ssid,     CONFIG_IOT_WIFI_UDP_STA_SSID, std::min(strlen(CONFIG_IOT_WIFI_UDP_STA_SSID),     sizeof(wifi_sta_cfg.sta.ssid)));
    memcpy(wifi_sta_cfg.sta.password, CONFIG_IOT_WIFI_UDP_STA_PASS, std::min(strlen(CONFIG_IOT_WIFI_UDP_STA_PASS), sizeof(wifi_sta_cfg.sta.password)));
    wifi_sta_cfg.sta.threshold.authmode = WIFI_STA_AUTH_MODE;
    wifi_sta_cfg.sta.pmf_cfg.capable    = true;
    wifi_sta_cfg.sta.pmf_cfg.required   = false;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    state = State::INITIALIZED;

    return ESP_OK;
  #endif
}

// Get default MAC from API and convert to ASCII HEX
esp_err_t Wifi::retrieve_mac(void)
{

  const esp_err_t status = esp_read_mac((uint8_t *) mac_addr,  ESP_MAC_WIFI_STA);

  if (status == ESP_OK) {
    snprintf(mac_addr_cstr, sizeof(mac_addr_cstr), MACSTR, MAC2STR((char *) mac_addr));
  }

  return status;
}


void Wifi::prepare_for_deep_sleep()
{
  #ifdef CONFIG_IOT_ENABLE_UDP
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler);
  #endif
  
  esp_err_t status = esp_wifi_stop();
  if (status != ESP_OK) {
    ESP_LOGW(TAG, "Unable to stop Wifi Properly: %s.", esp_err_to_name(status));
  }
}

