#include "battery.hpp"

#ifdef CONFIG_IOT_BATTERY_LEVEL

#define __BATTERY__
#include "global.hpp"
#undef __BATTERY__

esp_err_t Battery::init()
{
  esp_log_level_set(TAG, cfg.log_level);

  gpio_set_direction(VOLTAGE_ENABLE, GPIO_MODE_OUTPUT);
  gpio_set_level(VOLTAGE_ENABLE, 0);

  return ESP_OK;
}

double Battery::read_voltage_level()
{
  esp_adc_cal_characteristics_t adc1_chars;

  gpio_set_level(VOLTAGE_ENABLE, 1);

  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
    ADC_UNIT_1, 
    ADC_ATTEN_DB_11, 
    ADC_WIDTH_BIT_9, 
    ESP_ADC_CAL_VAL_DEFAULT_VREF, 
    &adc1_chars);

  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    ESP_LOGD(TAG, "ADC Calib Type: eFuse Vref");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    ESP_LOGD(TAG, "ADC Calib Type: Two Point");
  } else {
    ESP_LOGD(TAG, "ADC Calib Type: Default");
  }

  adc1_config_width(ADC_WIDTH_BIT_9);
  adc1_config_channel_atten(ADC, ADC_ATTEN_DB_11);

  int voltage = esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC), &adc1_chars);
  
  gpio_set_level(VOLTAGE_ENABLE, 0);

  return double(voltage) / 500.0;
}

esp_err_t Battery::prepare_for_deep_sleep()
{
  return ESP_OK;
}

#endif
