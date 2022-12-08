#include "app.hpp"

#include "global.hpp"

xTaskHandle App::task_handle = nullptr;

static IoT::UserResult iot_handler(IoT::State state)
{
  IoT::UserResult result = IoT::UserResult::COMPLETED;

  switch (state) {
    case IoT::State::STARTUP:
      result = IoT::UserResult::COMPLETED;
      break;
    case IoT::State::WAIT_FOR_EVENT:
      result = IoT::UserResult::NOT_COMPLETED;
      break;
    case IoT::State::PROCESS_EVENT:
      result = IoT::UserResult::COMPLETED;
      break;
    case IoT::State::WAIT_END_EVENT:
      result = IoT::UserResult::COMPLETED;
      break;
    case IoT::State::END_EVENT:
      result = IoT::UserResult::COMPLETED;
      break;
    default:
      break;
  }

  return result;
}

void App::task(void * param)
{
  while (true) iot.process();
}

esp_err_t App::init()
{
  iot.init(&iot_handler);

  if (xTaskCreate(task, "main_task", 4*4096, nullptr, 5, &task_handle) != pdPASS) {
    ESP_LOGE(TAG, "Unable to create main_task.");
    return ESP_FAIL;
  }

  return ESP_OK;
}