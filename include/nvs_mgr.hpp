#pragma once

#include <nvs_flash.h>

#include "config.hpp"

class NVSMgr
{
  public:
    struct NVSData {
      MacAddr gateway_mac_addr;
      int8_t  rssi;
    };

  private:
    static constexpr char const * TAG            = "NVSMgr Class";
    static constexpr char const * NAMESPACE      = "Exerciser";
    static constexpr char const * PARTITION_NAME = "nvs";

    nvs_handle_t nvs_handle;
    NVSData nvs_data;
    bool data_is_valid;

  public:
    esp_err_t init();
    esp_err_t get_nvs_data();
    esp_err_t set_nvs_data(NVSData * data);

    inline const NVSData *     get_data() { return &nvs_data; }
    inline bool           is_data_valid() { return data_is_valid; }
};