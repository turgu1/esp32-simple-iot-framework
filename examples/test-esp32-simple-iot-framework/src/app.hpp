#pragma once

#include "config.hpp"

class App
{
  private:
    static constexpr char const * TAG = "App Class";

    static xTaskHandle task_handle;
    static void task(void * param);

  public:
    esp_err_t init();
};