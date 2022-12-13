#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "app.hpp"
#include "global.hpp"

const char * TAG = "Exerciser";

App app;

extern "C" {
  void app_main() {
    esp_log_level_set(TAG, CONFIG_IOT_LOG_LEVEL);

    if (app.init() != ESP_OK) {
      ESP_LOGE(TAG, "Main App Initialization failed...");
    }

    // Wait for 24 hours before trying again. This is to minimize battery consumption

    // iot.prepare_for_deep_sleep();
    esp_deep_sleep(24*60*60 * 1e6);
  }
}