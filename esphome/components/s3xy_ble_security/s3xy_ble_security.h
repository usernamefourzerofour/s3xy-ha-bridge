#pragma once

#include <esp_gap_ble_api.h>

#include "esphome/components/esp32_ble_server/ble_server.h"
#include "esphome/core/log.h"

namespace esphome::s3xy_ble_security {

class S3XYBLESecurity {
 public:
  explicit S3XYBLESecurity(esp32_ble_server::BLEServer *server) : server_(server) {}

  void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (event != ESP_GATTS_CONNECT_EVT || gatts_if != this->server_->get_gatts_if() || param->connect.link_role != 1)
      return;

    // Start security as the peripheral, as the original Arduino implementation does.
    const esp_err_t result = esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT);
    if (result == ESP_OK) {
      ESP_LOGI("s3xy", "Requested BLE encryption");
    } else {
      ESP_LOGW("s3xy", "BLE encryption request failed: %s", esp_err_to_name(result));
    }
  }

 protected:
  esp32_ble_server::BLEServer *server_;
};

}  // namespace esphome::s3xy_ble_security
