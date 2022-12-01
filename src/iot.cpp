#include "iot.hpp"

RTC_NOINIT_ATTR IoT::State state;

esp_err_t IoT::init(ProcessHandler * handler)
{
  esp_log_level_set(TAG, CONFIG_IOT_LOG_LEVEL);

  process_handler = handler;

  ESP_ERROR_CHECK(nvs_mgr.init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(wifi.init());

  #ifdef CONFIG_IOT_BATTERY_LEVEL
    battery.init();
  #endif

  #ifdef CONFIG_IOT_ENABLE_UDP
    wifi.show_state();

    Wifi::State state = wifi.get_state();
    while (!((state == Wifi::State::CONNECTED) || (state == Wifi::State::ERROR)) || (state == Wifi::State::WAITING_FOR_IP)) {
      vTaskDelay(pdMS_TO_TICKS(500));
      state = wifi.get_state();
    }

    if (state == Wifi::State::ERROR) return ESP_FAIL;
  #endif

  #ifdef CONFIG_IOT_ENABLE_UDP
    // UDP initialization
    ESP_ERROR_CHECK(udp.init());
  #endif

  #ifdef CONFIG_IOT_ENABLE_ESP_NOW
    // EspNow initialization
    ESP_ERROR_CHECK(esp_now.init());
  #endif

  return ESP_OK;
}

esp_err_t IoT::prepare_for_deep_sleep()
{
  #ifdef CONFIG_IOT_BATTERY_LEVEL
    battery.prepare_for_deep_sleep();
  #endif
  
  #ifdef CONFIG_IOT_ENABLE_UDP
    udp.prepare_for_deep_sleep();
  #endif

  #ifdef CONFIG_IOT_ENABLE_ESP_NOW
    esp_now.prepare_for_deep_sleep();
  #endif

  wifi.prepare_for_deep_sleep();

  return ESP_OK;
}

void IoT::process()
{
  esp_reset_reason_t reason = esp_reset_reason();

  if (reason == ESP_RST_DEEPSLEEP) {
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER) {
      if (!(state & (State::PROCESS_EVENT | State::WAIT_END_EVENT))) state = State::PROCESS_EVENT;
    }
  }
  else {
    state = STARTUP;
  }

  UserResult user_result;
  if (process_handler != nullptr) user_result = process_handler(state);
  
  switch state {
    case State::STARTUP:
      break;

    case State::HOURS_24:
      break;
  }
}