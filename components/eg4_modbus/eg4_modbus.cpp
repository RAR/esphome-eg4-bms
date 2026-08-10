#include "eg4_modbus.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace eg4_modbus {

static const char *const TAG = "eg4_modbus";

static const uint8_t MODBUS_READ_HOLDING_REGISTERS = 0x03;
static const uint8_t MODBUS_WRITE_MULTIPLE_REGISTERS = 0x10;

static const uint16_t EG4_MODBUS_RESPONSE_TIMEOUT = 1000;
static const uint16_t EG4_MODBUS_MIN_MSG_LEN = 5;

void EG4Modbus::setup() {
  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->setup();
  }

  // Two devices sharing an address both match every response for it and publish
  // interleaved states, which is near impossible to diagnose from the sensor
  // values alone. Configuration cannot catch this, so report it at startup.
  for (size_t i = 0; i < this->devices_.size(); i++) {
    for (size_t j = i + 1; j < this->devices_.size(); j++) {
      if (this->devices_[i]->address_ == this->devices_[j]->address_) {
        ESP_LOGE(TAG, "Duplicate device address 0x%02X on this bus - both devices will publish the same data",
                 this->devices_[i]->address_);
      }
    }
  }
}

void EG4Modbus::loop() {
  const uint32_t now = millis();

  // Read incoming bytes
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    if (this->parse_modbus_byte_(byte)) {
      this->last_modbus_byte_ = now;
    } else {
      this->rx_buffer_.clear();
    }
  }

  // Check for timeout on current response
  if (this->waiting_for_response_ && !this->rx_buffer_.empty() && 
      (now - this->last_modbus_byte_ > EG4_MODBUS_RESPONSE_TIMEOUT)) {
    ESP_LOGW(TAG, "Modbus response timeout");
    this->rx_buffer_.clear();
    this->waiting_for_response_ = false;
  }

  // Check for complete timeout (no response at all)
  if (this->waiting_for_response_ && this->rx_buffer_.empty() && 
      (now - this->last_send_ > EG4_MODBUS_RESPONSE_TIMEOUT)) {
    ESP_LOGW(TAG, "No response received");
    this->waiting_for_response_ = false;
  }

  // Send next request if not waiting for a response
  if (!this->waiting_for_response_) {
    this->send_next_request_();
  }
}

void EG4Modbus::dump_config() {
  ESP_LOGCONFIG(TAG, "EG4 Modbus:");
  ESP_LOGCONFIG(TAG, "  Flow control pin: %s", YESNO(this->flow_control_pin_ != nullptr));
}

float EG4Modbus::get_setup_priority() const { return setup_priority::DATA; }

uint16_t crc16_modbus(const uint8_t *data, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void EG4Modbus::send(uint8_t address, uint8_t function, uint16_t start_register, uint16_t num_registers) {
  // Add request to queue instead of sending immediately
  ModbusRequest request;
  request.address = address;
  request.function = function;
  request.start_register = start_register;
  request.num_registers = num_registers;
  
  this->request_queue_.push(request);
  
  ESP_LOGV(TAG, "Queued request for address 0x%02X, queue size: %d", address, this->request_queue_.size());
}

void EG4Modbus::send_next_request_() {
  if (this->request_queue_.empty() || this->waiting_for_response_) {
    return;
  }

  ModbusRequest request = this->request_queue_.front();
  this->request_queue_.pop();

  uint8_t frame[8];
  frame[0] = request.address;
  frame[1] = request.function;
  frame[2] = request.start_register >> 8;
  frame[3] = request.start_register & 0xFF;
  frame[4] = request.num_registers >> 8;
  frame[5] = request.num_registers & 0xFF;

  uint16_t crc = crc16_modbus(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = crc >> 8;

  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->digital_write(true);
  }

  this->write_array(frame, 8);
  this->flush();

  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->digital_write(false);
  }

  ESP_LOGV(TAG, "Sent: %s", format_hex_pretty(frame, 8).c_str());
  this->last_send_ = millis();
  this->waiting_for_response_ = true;
}

bool EG4Modbus::parse_modbus_byte_(uint8_t byte) {
  const uint32_t now = millis();
  
  // Start of new frame if buffer is empty or timeout occurred
  if (this->rx_buffer_.empty()) {
    this->rx_buffer_.push_back(byte);
    return true;
  }

  this->rx_buffer_.push_back(byte);
  const uint8_t *raw = &this->rx_buffer_[0];
  const size_t len = this->rx_buffer_.size();

  // Wait for minimum message length
  if (len < EG4_MODBUS_MIN_MSG_LEN) {
    return true;
  }

  // Check if we have a complete frame
  uint8_t address = raw[0];
  uint8_t function = raw[1];

  // Calculate expected length based on function code
  size_t expected_len = 0;
  
  if (function == MODBUS_READ_HOLDING_REGISTERS) {
    if (len >= 3) {
      uint8_t byte_count = raw[2];
      expected_len = 3 + byte_count + 2;  // addr + func + count + data + crc
    }
  } else if ((function & 0x80) != 0) {
    // Error response
    expected_len = 5;  // addr + func + error_code + crc
  }

  if (expected_len == 0 || len < expected_len) {
    return true;  // Need more data
  }

  if (len >= expected_len) {
    // Verify CRC
    uint16_t crc_calc = crc16_modbus(raw, expected_len - 2);
    uint16_t crc_recv = raw[expected_len - 2] | (raw[expected_len - 1] << 8);

    if (crc_calc != crc_recv) {
      ESP_LOGW(TAG, "Modbus CRC check failed! Calculated: 0x%04X, Received: 0x%04X", crc_calc, crc_recv);
      this->rx_buffer_.clear();
      return false;
    }

    ESP_LOGV(TAG, "Received: %s", format_hex_pretty(raw, expected_len).c_str());

    // Dispatch to devices
    for (auto *device : this->devices_) {
      device->on_modbus_data(this->rx_buffer_);
    }

    this->rx_buffer_.clear();
    this->waiting_for_response_ = false;  // Ready for next request
    return true;
  }

  return true;
}

}  // namespace eg4_modbus
}  // namespace esphome
