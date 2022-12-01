#include <nvs_flash.h>

#include "nvs_mgr.hpp"

esp_err_t NVSMgr::init()
{
  // Initialize NVS
  esp_err_t status = nvs_flash_init();
  if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    if ((status = nvs_flash_erase()) == ESP_OK) status = nvs_flash_init();
  }
  data_is_valid = false;
  return status;
}

esp_err_t NVSMgr::get_nvs_data()
{
  esp_err_t status;

  if ((status = nvs_open(NAMESPACE, NVS_READONLY, &nvs_handle)) == ESP_OK) {
    size_t len = sizeof(NVSData);
    status = nvs_get_blob(nvs_handle, "DATA", &nvs_data, &len);
    data_is_valid = (status == ESP_OK) && (len == sizeof(NVSData));
    nvs_close(nvs_handle);
  }

  return data_is_valid ? ESP_OK : ESP_FAIL;
}

esp_err_t NVSMgr::set_nvs_data(NVSData * data)
{
  esp_err_t status;

  memcpy(&nvs_data, data, sizeof(NVSData));
  data_is_valid = true;

  if ((status = nvs_open(NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
    status = nvs_set_blob(nvs_handle, "DATA", &nvs_data, sizeof(NVSData));
    nvs_close(nvs_handle);
  }

  return status;
}
