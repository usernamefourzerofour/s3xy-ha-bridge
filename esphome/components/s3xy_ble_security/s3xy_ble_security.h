#pragma once

#include <esp_gap_ble_api.h>

#include <array>
#include <optional>

#include "esphome/components/esp32_ble_server/ble_characteristic.h"
#include "esphome/components/esp32_ble_server/ble_server.h"
#include "esphome/components/esp32_ble_server/ble_service.h"
#include "esphome/core/log.h"

namespace esphome::s3xy_ble_security {

class S3XYBLESecurity {
 public:
  S3XYBLESecurity(esp32_ble_server::BLEServer *server, esp32_ble_server::BLECharacteristic *notify)
      : server_(server), notify_(notify) {}

  void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (gatts_if != this->server_->get_gatts_if())
      return;

    switch (event) {
      case ESP_GATTS_CONNECT_EVT:
        if (param->connect.link_role == 1) {
          this->start_encryption_(param->connect.remote_bda);
        }
        break;
      case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
        const auto &descriptor = param->add_char_descr;
        auto *service = this->notify_->get_service();
        if (descriptor.status == ESP_GATT_OK && service != nullptr &&
            descriptor.service_handle == service->get_handle() && descriptor.descr_uuid.len == ESP_UUID_LEN_16 &&
            descriptor.descr_uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
          this->cccd_handle_ = descriptor.attr_handle;
        }
        break;
      }
      case ESP_GATTS_WRITE_EVT: {
        const auto &write = param->write;
        if (write.handle == this->cccd_handle_ && write.len == 2 && !write.is_prep) {
          this->subscribed_ = (write.value[0] & 0x01) != 0;
          ESP_LOGI("s3xy", "Commander notifications %s", this->subscribed_ ? "enabled" : "disabled");
          this->flush_replies_();
        }
        break;
      }
      case ESP_GATTS_DISCONNECT_EVT:
        this->subscribed_ = false;
        this->pending_init_ = false;
        this->pending_rename_.reset();
        break;
      default:
        break;
    }
  }

  bool subscribed() const { return this->subscribed_; }

  bool ready_for_press() const {
    if (this->subscribed_) {
      return true;
    }
    ESP_LOGW("s3xy", "Button press skipped: Commander has not enabled notifications");
    return false;
  }

  void reply_to_init() {
    this->pending_init_ = true;
    this->flush_replies_();
  }

  void reply_to_rename(uint8_t first, uint8_t second) {
    this->pending_rename_ = std::array<uint8_t, 4>{0xA4, 0x00, first, second};
    this->flush_replies_();
  }

 private:
  void flush_replies_() {
    // ESPHome's GATT server processes the CCCD write before this handler, so notify() can now deliver replies.
    if (!this->subscribed_)
      return;

    if (this->pending_init_) {
      this->notify_->set_value({0xC7, 0x00, 0x01});
      this->notify_->notify();
      this->pending_init_ = false;
      ESP_LOGI("s3xy", "Sent Commander initialization reply");
    }
    if (this->pending_rename_) {
      this->notify_->set_value(this->pending_rename_->data(), this->pending_rename_->size());
      this->notify_->notify();
      this->pending_rename_.reset();
      ESP_LOGI("s3xy", "Sent Commander rename reply");
    }
  }

  void start_encryption_(esp_bd_addr_t peer) {
    // Start security as the peripheral, as the original Arduino implementation does.
    const esp_err_t result = esp_ble_set_encryption(peer, ESP_BLE_SEC_ENCRYPT);
    if (result == ESP_OK) {
      ESP_LOGI("s3xy", "Requested BLE encryption");
    } else {
      ESP_LOGW("s3xy", "BLE encryption request failed: %s", esp_err_to_name(result));
    }
  }

  esp32_ble_server::BLEServer *server_;
  esp32_ble_server::BLECharacteristic *notify_;
  uint16_t cccd_handle_{0xFFFF};
  bool subscribed_{false};
  bool pending_init_{false};
  std::optional<std::array<uint8_t, 4>> pending_rename_;
};

}  // namespace esphome::s3xy_ble_security
