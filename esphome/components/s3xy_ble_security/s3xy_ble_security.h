#pragma once

#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>

#include <array>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <vector>

#include "esphome/components/esp32_ble/ble.h"
#include "esphome/components/esp32_ble_server/ble_characteristic.h"
#include "esphome/components/esp32_ble_server/ble_server.h"
#include "esphome/components/esp32_ble_server/ble_service.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome::s3xy_ble_security {

class S3XYBLESecurity : public Component {
 public:
  S3XYBLESecurity(esp32_ble::ESP32BLE *ble, esp32_ble_server::BLEServer *server,
                  esp32_ble_server::BLECharacteristic *notify)
      : ble_(ble), server_(server), notify_(notify) {}

  void enable_hold_to_pair() {
    this->hold_to_pair_ = true;
    this->server_->set_advertising_required(false);
    this->server_->set_pairing_enabled(false);
  }

  bool pairing_held() const { return this->pairing_held_; }
  bool waiting_for_hold() const { return this->hold_to_pair_ && !this->advertising_requested_; }

  void set_pairing_held(bool held) {
    if (!this->hold_to_pair_ || this->pairing_held_ == held)
      return;
    this->pairing_held_ = held;
    ESP_LOGI("s3xy", "Pairing button %s", held ? "held" : "released");
    this->refresh_advertising_();
  }

  void unpair(uint16_t connection_id) {
    this->forget_peer_ = true;
    if (esp_ble_gatts_close(this->server_->get_gatts_if(), connection_id) != ESP_OK) {
      this->forget_peer_ = false;
      ESP_LOGW("s3xy", "Could not disconnect Commander after unpair request");
    }
  }

  void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (gatts_if != this->server_->get_gatts_if())
      return;

    switch (event) {
      case ESP_GATTS_CONNECT_EVT:
        if (param->connect.link_role == 1) {
          this->accept_connection_(param->connect.remote_bda, param->connect.conn_id);
        }
        break;
      case ESP_GATTS_START_EVT:
        this->refresh_advertising_();
        break;
      case ESP_GATTS_ADD_CHAR_EVT: {
        auto &characteristic = param->add_char;
        auto *service = this->notify_->get_service();
        if (characteristic.status == ESP_GATT_OK && service != nullptr &&
            characteristic.service_handle == service->get_handle() &&
            esp32_ble::ESPBTUUID::from_uuid(characteristic.char_uuid) == this->notify_->get_uuid()) {
          this->notify_handle_ = characteristic.attr_handle;
        }
        break;
      }
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
        if (this->link_allowed_ && write.handle == this->cccd_handle_ && write.len == 2 && !write.is_prep) {
          this->subscribed_ = (write.value[0] & 0x01) != 0;
          if (this->subscribed_) {
            this->notify_client_ = write.conn_id;
          } else {
            this->notify_client_.reset();
            this->front_button_down_ = false;
          }
          ESP_LOGI("s3xy", "Commander notifications %s", this->subscribed_ ? "enabled" : "disabled");
          this->flush_replies_();
        }
        break;
      }
      case ESP_GATTS_DISCONNECT_EVT:
        this->notify_client_.reset();
        this->link_allowed_ = false;
        this->subscribed_ = false;
        this->front_button_down_ = false;
        this->pending_init_ = false;
        this->pending_rename_.reset();
        if (this->forget_peer_)
          this->remove_bond_(param->disconnect.remote_bda);
        if (this->hold_to_pair_) {
          this->server_->set_pairing_enabled(false);
          this->refresh_advertising_();
        }
        break;
      default:
        break;
    }
  }

  bool subscribed() const { return this->subscribed_; }

  void set_front_button(bool down) {
    if (!this->subscribed_ || down == this->front_button_down_)
      return;
    this->front_button_down_ = down;
    this->notify_->set_value({static_cast<uint8_t>(down ? 0x01 : 0x00)});
    this->notify_->notify();
    ESP_LOGI("s3xy", "Front button %s", down ? "down" : "up");
  }

  void send_double_press_packet(std::initializer_list<uint8_t> packet) {
    if (!this->notify_client_ || !this->notify_handle_) {
      ESP_LOGW("s3xy", "Double press packet skipped: no subscribed client or notification handle");
      return;
    }

    this->notify_->set_value(packet);
    auto &value = this->notify_->get_value();
    // Send a GATT notification and capture the return status that ESPHome's notify() does not expose.
    const esp_err_t result =
        esp_ble_gatts_send_indicate(this->server_->get_gatts_if(), *this->notify_client_, *this->notify_handle_,
                                   static_cast<uint16_t>(value.size()), value.data(), false);
    if (value.size() == 1) {
      ESP_LOGI("s3xy", "Double press TX %02X: %s", static_cast<unsigned>(value[0]), esp_err_to_name(result));
    } else {
      ESP_LOGI("s3xy", "Double press TX %02X %02X: %s", static_cast<unsigned>(value[0]),
               static_cast<unsigned>(value[1]), esp_err_to_name(result));
    }
  }

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
  void remove_bond_(esp_bd_addr_t peer) {
    if (esp_ble_remove_bond_device(peer) != ESP_OK) {
      this->forget_peer_ = false;
      ESP_LOGW("s3xy", "Could not request removal of Commander's stored BLE bond");
      return;
    }
    // Bond removal runs asynchronously in the Bluetooth stack.
    std::array<uint8_t, ESP_BD_ADDR_LEN> address;
    std::memcpy(address.data(), peer, address.size());
    this->set_timeout(1000, [this, address]() {
      this->forget_peer_ = false;
      if (this->is_bonded_(address.data())) {
        ESP_LOGW("s3xy", "Commander's stored BLE bond is still present");
      }
      this->refresh_advertising_();
    });
  }

  void refresh_advertising_() {
    if (!this->hold_to_pair_)
      return;
    auto *service = this->notify_->get_service();
    const bool should_advertise = service != nullptr && service->is_running() && !this->forget_peer_ &&
                                  (this->pairing_held_ || esp_ble_get_bond_device_num() > 0);
    if (should_advertise == this->advertising_requested_)
      return;
    this->advertising_requested_ = should_advertise;
    if (should_advertise) {
      this->ble_->advertising_start();
    } else {
      this->ble_->advertising_stop();
    }
  }

  bool is_bonded_(const uint8_t *peer) const {
    int count = esp_ble_get_bond_device_num();
    if (count <= 0)
      return false;
    std::vector<esp_ble_bond_dev_t> bonds(count);
    if (esp_ble_get_bond_device_list(&count, bonds.data()) != ESP_OK)
      return false;
    for (int i = 0; i < count; ++i) {
      if (std::memcmp(bonds[i].bd_addr, peer, ESP_BD_ADDR_LEN) == 0)
        return true;
    }
    return false;
  }

  void accept_connection_(esp_bd_addr_t peer, uint16_t connection_id) {
    this->link_allowed_ = !this->hold_to_pair_ ||
                          (!this->forget_peer_ && (this->pairing_held_ || this->is_bonded_(peer)));
    if (this->hold_to_pair_) {
      this->server_->set_pairing_enabled(this->link_allowed_);
    }
    if (this->link_allowed_) {
      this->start_encryption_(peer);
    } else {
      ESP_LOGI("s3xy", "New BLE connection requires holding the M5 button");
      esp_ble_gatts_close(this->server_->get_gatts_if(), connection_id);
    }
  }

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

  esp32_ble::ESP32BLE *ble_;
  esp32_ble_server::BLEServer *server_;
  esp32_ble_server::BLECharacteristic *notify_;
  std::optional<uint16_t> notify_handle_;
  std::optional<uint16_t> notify_client_;
  uint16_t cccd_handle_{0xFFFF};
  bool hold_to_pair_{false};
  bool pairing_held_{false};
  bool advertising_requested_{false};
  bool link_allowed_{false};
  bool forget_peer_{false};
  bool subscribed_{false};
  bool front_button_down_{false};
  bool pending_init_{false};
  std::optional<std::array<uint8_t, 4>> pending_rename_;
};

}  // namespace esphome::s3xy_ble_security
