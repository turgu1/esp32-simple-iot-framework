#include <driver/gpio.h>

#include "app.hpp"

#include "global.hpp"

xTaskHandle App::task_handle = nullptr;

const gpio_num_t gpio                   = GPIO_NUM_15;
const int        event_timeout_duration = 10*60;

RTC_NOINIT_ATTR int transmit_count;

static IoT::UserResult iot_handler(IoT::State state)
{
  IoT::UserResult result = IoT::UserResult::COMPLETED;

  int level = 1;

  switch (state) {
    case IoT::State::STARTUP: {
        gpio_config_t io_conf = {};
        
        io_conf.mode         = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = 1ULL << gpio;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;

        gpio_config(&io_conf);

        result = IoT::UserResult::COMPLETED;
      }
      break;

    case IoT::State::WAIT_FOR_EVENT:
      if (gpio_get_level(gpio) == 1) {
        result = IoT::UserResult::NEW_EVENT;
      }
      else {
        result = IoT::UserResult::NOT_COMPLETED;
      }
      break;

    case IoT::State::PROCESS_EVENT:
      if (gpio_get_level(gpio) == 1) {
        level = 0;
        transmit_count = 1;
        iot.set_deep_sleep_duration(event_timeout_duration);
        iot.send_msg("STATE", "state:HIGH");
        result = IoT::UserResult::COMPLETED;
      }
      else {
        result = IoT::UserResult::ABORTED;
      }
      break;

    case IoT::State::WAIT_END_EVENT:
      if (gpio_get_level(gpio) == 0) {
        result = IoT::UserResult::COMPLETED;
      }
      else {
        level = 0;
        if (transmit_count < 5) {
          iot.send_msg("STATE", "state:HIGH");
          transmit_count++;
          if (transmit_count < 5) iot.set_deep_sleep_duration(event_timeout_duration);
        }
        result = IoT::UserResult::NOT_COMPLETED;
      }
      break;

    case IoT::State::END_EVENT:
      iot.send_msg("STATE", "state:LOW");
      result = IoT::UserResult::COMPLETED;
      break;

    default:
      break;
  }

  esp_sleep_enable_ext0_wakeup(gpio, level);

  return result;
}

void App::task(void * param)
{
  while (true) iot.process();
}

esp_err_t App::init()
{
  if (iot.init(&iot_handler) != ESP_OK) {
    ESP_LOGE(TAG, "Unable to start the IOT Framework properly.");
    return ESP_FAIL;
  }

  if (xTaskCreate(task, "main_task", 4*4096, nullptr, 5, &task_handle) != pdPASS) {
    ESP_LOGE(TAG, "Unable to create main_task.");
    return ESP_FAIL;
  }

  return ESP_OK;
}