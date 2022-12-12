#include <time.h>
#include <esp_timer.h>

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
RTC_NOINIT_ATTR uint32_t   error_count;
RTC_NOINIT_ATTR uint32_t   send_seq_nbr;
RTC_NOINIT_ATTR uint32_t   last_duration;

esp_err_t IoT::init(ProcessHandler * handler)
{
  process_handler           = handler;
  deep_sleep_duration       = 0;
  esp_reset_reason_t reason = esp_reset_reason();

  if (reason != ESP_RST_DEEPSLEEP) {
    if (!config.init(true)) return ESP_FAIL;
    restart_reason       = RestartReason::RESET;
    time_t now           = time(&now);
    state = return_state = STARTUP;
    next_watchdog_time   = now + CONFIG_IOT_WATCHDOG_INTERVAL;
    error_count          = 0;
    send_seq_nbr         = 0;
    last_duration        = 0;
  }
  else {
    if (!config.init(false)) return ESP_FAIL;
    restart_reason = RestartReason::DEEP_SLEEP_AWAKE;
    deep_sleep_wakeup_reason = esp_sleep_get_wakeup_cause();
  }

  esp_log_level_set(TAG, cfg.log_level);

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
    send_queue_handle = esp_now.get_send_queue_handle();
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
  if (now > next_watchdog_time) return State::WATCHDOG;
  return the_state;
}

/// Send a message to the gateway. May not return if the message cannot
/// be sent (gateway is dead for any reason). The underneath protocol
/// (udp or esp_now) will start a deep_sleep if not able to send the message.
/// At next boot, the current state will be done again to try to send again
/// something if there is still something to be sent.
void IoT::send_msg(const char * msg_type, const char * other_field)
{
  static char pkt[248];

  snprintf(pkt, 247,
    "%s;{name:%s,type:%s,seq:%d,dur:%d,mac:\"%s\",err:%d,rssi:%d,st:%d,rst:%d,heap:%d%s%s"
    #ifdef CONFIG_IOT_BATTERY_LEVEL
      ",vbat:%4.2f"
    #endif
    #ifdef CONFIG_IOT_ENABLE_UDP
      ",ip:\"%s\""
    #endif
    "}",
    CONFIG_IOT_TOPIC_NAME,
    CONFIG_IOT_DEVICE_NAME,
    msg_type,
    send_seq_nbr,
    last_duration,
    wifi.get_mac_cstr(),
    error_count,
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
    ESPNow::SendEvent evt;
    if (send_queue_handle != nullptr) {
      if (xQueueReceive(send_queue_handle, &evt, pdMS_TO_TICKS(200)) != pdTRUE) {
        ESP_LOGE(TAG, "No answer after packet sent.");
        error_count++;
      }
      else {
        if (evt.status == ESP_OK) {
          ESP_LOGD(TAG, "Packet transmitted:");
        }
      }
    }
  #endif

  send_seq_nbr++;
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

    case State::WATCHDOG: {
        send_msg("WATCHDOG");
        time_t now;
        next_watchdog_time = time(&now) + CONFIG_IOT_WATCHDOG_INTERVAL;
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

  if ((state & (PROCESS_EVENT|END_EVENT|WATCHDOG)) == 0) {
    if (deep_sleep_duration >= 0) {
      if (deep_sleep_duration == 0) {
        time_t now = time(&now);
        if ((next_watchdog_time - now) > 0) {
          prepare_for_deep_sleep();
          last_duration = (int)(esp_timer_get_time() / 1000);
          esp_deep_sleep(((uint64_t)(next_watchdog_time - now)) * 1e6);
        }
      }
      else {
        prepare_for_deep_sleep();
        last_duration = (int)(esp_timer_get_time() / 1000);
        esp_deep_sleep(((uint64_t) deep_sleep_duration) * 1e6);
      }
    }
  }
}