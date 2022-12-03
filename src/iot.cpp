#include <esp_sleep.h>
#include <time.h>

#include "iot.hpp"

#if CONFIG_IOT_ESPNOW_ENABLE_LONG_RANGE
  #pragma message "----> INFO: IOT WIFI LONG RANGE ENABLED <----"
#else
  #pragma message "----> INFO: IOT WIFI LONG RANGE DISABLED <----"
#endif

#if CONFIG_IOT_BATTERY_LEVEL
  #pragma message "----> INFO: IOT BATTERY LEVEL ENABLED <----"
#else
  #pragma message "----> INFO: IOT BATTERY LEVEL DISABLED <----"
#endif

RTC_NOINIT_ATTR IoT::State state;
RTC_NOINIT_ATTR IoT::State return_state;
RTC_NOINIT_ATTR time_t     next_watchdog_time;

esp_err_t IoT::init(ProcessHandler * handler)
{
  esp_log_level_set(TAG, CONFIG_IOT_LOG_LEVEL);

  process_handler = handler;

  esp_reset_reason_t reason = esp_reset_reason();

  if (reason != ESP_RST_DEEPSLEEP) {
    time_t now;
    state = return_state = STARTUP;
    next_watchdog_time = time(&now) + (60*60*24);
  }

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

IoT::State IoT::check_if_24_hours_time(State the_state)
{
  time_t now;

  time(&now);
  if (now > next_watchdog_time) return State::HOURS_24;
  return the_state;
}

void IoT::send_msg(const char * msg_type, const char * other_field)
{
  static char pkt[200];

  sprintf(pkt, 
    "%s;{type:%s,mac:\"%s\",rssi:%d,state:%d,return_state:%d,heap:%d%s%s"
    #ifdef CONFIG_IOT_BATTERY_LEVEL
      ",vbat:%f"
    #endif
    #ifdef CONFIG_IOT_ENABLE_UDP
      ",ip:\"%s\""
    #endif
    "}",
    CONFIG_IOT_TOPIC_NAME,
    msg_type,
    wifi.get_mac_cstr(),
    wifi.get_rssi(),
    state, return_state,
    esp_get_free_heap_size(),
    other_field == nullptr ? "" : ",",
    other_field == nullptr ? "" : other_field
    #ifdef CONFIG_IOT_BATTERY_LEVEL
      , battery.read_voltage_level()
    #endif
    #ifdef CONFIG_IOT_ENABLE_UDP
      , wifi.get_ip_cstr()
    #endif
  );

  #ifdef CONFIG_IOT_ENABLE_UDP
    udp.send((uint8_t *) pkt, strlen(pkt));
  #endif
  #ifdef CONFIG_IOT_ENABLE_ESP_NOW
    esp_now.send((uint8_t *) pkt, strlen(pkt));
  #endif
}

void IoT::process()
{
  UserResult user_result = UserResult::COMPLETED;
  if (process_handler != nullptr) user_result = process_handler(state);
  
  State new_state = state;
  State new_return_state = return_state;

  switch (state) {
    case State::STARTUP:
      send_msg("STARTUP");
      if (user_result != NOT_COMPLETED) {
        new_state        = WAIT_FOR_EVENT;
        new_return_state = WAIT_FOR_EVENT;
      }
      break;

    case State::HOURS_24: {
        send_msg("WATCHDOG");
        time_t now;
        next_watchdog_time = time(&now) + (60*60*24);
        new_state = new_return_state;
      }
      break;

    case State::WAIT_FOR_EVENT:
      if (user_result == NEW_EVENT) {
        new_state        = PROCESS_EVENT;
        new_return_state = PROCESS_EVENT;
      }
      else {
        new_return_state = WAIT_FOR_EVENT;
        new_state        = check_if_24_hours_time(WAIT_FOR_EVENT);
      }
      break;

    case State::PROCESS_EVENT:
     if (user_result == ABORTED) {
        new_state = new_return_state = WAIT_FOR_EVENT;
      }
      else if (user_result != NOT_COMPLETED) {
        new_state = new_return_state = WAIT_END_EVENT;
      }
      else {
        new_return_state = PROCESS_EVENT;
        new_state        = check_if_24_hours_time(PROCESS_EVENT);
      }
       break;

    case State::WAIT_END_EVENT:
      if (user_result == RETRY) {
        new_state = new_return_state = PROCESS_EVENT;
      }
      else if (user_result != NOT_COMPLETED) {
        new_state = new_return_state = END_EVENT;
      }
      else {
        new_return_state = WAIT_END_EVENT;
        new_state        = check_if_24_hours_time(WAIT_END_EVENT);
      }
      break;

    case State::END_EVENT:
      if (user_result != NOT_COMPLETED) {
        new_state = new_return_state = WAIT_FOR_EVENT;
      }
      break;
  }

  state        = new_state;
  return_state = new_return_state;
}