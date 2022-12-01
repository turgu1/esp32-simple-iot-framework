#pragma once

#include "config.hpp"

#ifdef CONFIG_IOT_ENABLE_ESP_NOW

#include <freertos/FreeRTOS.h>
#include <esp_now.h>

class ESPNow
{
  public:
    struct SendEvent {
      esp_err_t status;
      MacAddr mac_addr;
    };

  private:
    static constexpr char const * TAG = "ESPNow Class";

    static bool          abort;
    static QueueHandle_t send_queue_handle;
    static SendEvent     send_event;
    static void send_handler(const uint8_t * mac_addr, esp_now_send_status_t status);

    MacAddr ap_mac_addr;

    esp_err_t search_ap();

  public:
    esp_err_t                          init();
    esp_err_t                          send(const uint8_t * data, int len);
    QueueHandle_t     get_sent_queue_handle() { return send_queue_handle; }
    void             prepare_for_deep_sleep();
};

#endif