#ifdef CONFIG_IOT_BATTERY_LEVEL

#include "battery.hpp"

esp_err_t Battery::init()
{
  gpio_set_direction(VOLTAGE_ENABLE, GPIO_MODE_OUTPUT);
  gpio_set_level(VOLTAGE_ENABLE, 0);

  return ESP_OK;
}

double Battery::read_level()
{
  esp_adc_cal_characteristics_t adc1_chars;

  gpio_set_level(VOLTAGE_ENABLE, 1);

  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
    ADC_UNIT_1, 
    ADC_ATTEN_DB_11, 
    ADC_WIDTH_BIT_12, 
    DEFAULT_VREF, 
    &adc1_chars);

    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
      ESP_LOGD(TAG, "ADC Calib Type: eFuse Vref");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
      ESP_LOGD(TAG, "ADC Calib Type: Two Point");
    } else {
      ESP_LOGD(TAG, "ADC Calib Type: Default");
    }

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC, ADC_ATTEN_DB_11);

  int voltage = esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC), &adc1_chars);
  
  gpio_set_level(VOLTAGE_ENABLE, 0);

  return double(voltage) / 1000.0;
}

#endif
