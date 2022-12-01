#pragma once

#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

#include "config.hpp"

#ifdef CONFIG_IOT_BATTERY_LEVEL

class Battery
{
  private:
    static constexpr char const * TAG = "Battery Class";

    constexpr const gpio_num_t     VOLTAGE_ENABLE = GPIO_NUM_17;
    constexpr const adc1_channel_t ADC            = ADC1_CHANNEL_0;
    
  public:
    esp_err_t                   init();
    double                read_level();
    esp_err_t prepare_for_deep_sleep();
};

#endif