#include "eg4_modbus.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include <algorithm>

namespace esphome {
namespace eg4_modbus {

static const char *const TAG = "eg4_modbus";

static const uint16_t EG4_MODBUS_RESPONSE_TIMEOUT = 1000;
static const uint16_t EG4_MODBUS_MIN_MSG_LEN = 5;

// Passive mode: once no byte has arrived for this long, a partial frame at the
// head of the buffer can never complete and is abandoned so the splitter can
// resync. This is NOT used to delimit frames - at 19200 baud it is ~38 byte
// times, far longer than any intra-frame pause, and only triggers in the gap
// between the master's polls (~45 ms of every 100 ms on the EG4 link).
static const uint32_t PASSIVE_IDLE_FLUSH_MS = 20;

void EG4Modbus::setup() {
  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->setup();
    if (this->is_passive()) {
      // Hold an RS485 transceiver in receive for good.
      this->flow_control_pin_->digital_write(false);
    }
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
  if (this->is_passive()) {
    this->loop_passive_();
    return;
  }

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

void EG4Modbus::loop_passive_() {
  const uint32_t now = millis();

  uint8_t chunk[64];
  bool received = false;
  while (this->available() > 0) {
    const size_t n = std::min<size_t>(static_cast<size_t>(this->available()), sizeof(chunk));
    if (!this->read_array(chunk, n))
      break;
    this->splitter_.push(chunk, n);
    received = true;
  }
  if (received)
    this->last_rx_ = now;

  const bool idle = !received && this->splitter_.buffered() > 0 && (now - this->last_rx_) > PASSIVE_IDLE_FLUSH_MS;

  Frame frame;
  while (this->splitter_.next(frame, idle)) {
    this->handle_passive_frame_(frame);
  }

  const uint32_t dropped = this->splitter_.dropped_bytes();
  if (dropped != this->reported_dropped_) {
    ESP_LOGD(TAG, "Resync: skipped %u byte(s) that did not form a valid frame (%u total)",
             (unsigned) (dropped - this->reported_dropped_), (unsigned) dropped);
    this->reported_dropped_ = dropped;
  }
}

void EG4Modbus::handle_passive_frame_(const Frame &frame) {
  ESP_LOGV(TAG, "Seen: %s", format_hex_pretty(frame.raw, frame.raw_len).c_str());

  uint16_t start_register = MODBUS_START_REGISTER_UNKNOWN;
  const bool paired = this->requests_.pair(frame, &start_register);

  switch (frame.type) {
    case FrameType::READ_REQUEST:
    case FrameType::WRITE_REQUEST:
    case FrameType::WRITE_RESPONSE:
      // Requests only teach the tracker; writes are not decoded.
      return;
    case FrameType::READ_RESPONSE:
      if (!paired) {
        ESP_LOGV(TAG, "Response from 0x%02X (%u registers) without a matching request - skipped", frame.address,
                 frame.quantity);
        return;
      }
      break;
    case FrameType::EXCEPTION:
      break;
  }

  this->dispatch_(frame.address, frame.function, start_register, frame.payload, frame.payload_len);
}

void EG4Modbus::dispatch_(uint8_t address, uint8_t function, uint16_t start_register, const uint8_t *data,
                          size_t len) {
  for (auto *device : this->devices_) {
    if (device->address_ == address) {
      device->on_modbus_block(address, function, start_register, data, len);
    }
  }
}

void EG4Modbus::dump_config() {
  ESP_LOGCONFIG(TAG, "EG4 Modbus:");
  ESP_LOGCONFIG(TAG, "  Mode: %s", this->is_passive() ? "passive (listen only, never transmits)" : "active");
  ESP_LOGCONFIG(TAG, "  Flow control pin: %s", YESNO(this->flow_control_pin_ != nullptr));
}

float EG4Modbus::get_setup_priority() const { return setup_priority::DATA; }

void EG4Modbus::send(uint8_t address, uint8_t function, uint16_t start_register, uint16_t num_registers) {
  if (this->is_passive()) {
    ESP_LOGV(TAG, "Passive mode - not sending request for address 0x%02X", address);
    return;
  }
  if (function == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
    ESP_LOGE(TAG, "send() is for reads; use write_registers() for FC16");
    return;
  }

  // Add request to queue instead of sending immediately
  ModbusRequest request;
  request.address = address;
  request.function = function;
  request.start_register = start_register;
  request.num_registers = num_registers;

  this->request_queue_.push(request);

  ESP_LOGV(TAG, "Queued request for address 0x%02X, queue size: %d", address, this->request_queue_.size());
}

bool EG4Modbus::write_registers(uint8_t address, uint16_t start_register, const uint16_t *values, uint8_t count) {
  if (this->is_passive()) {
    ESP_LOGW(TAG, "Passive mode - refusing to write to address 0x%02X", address);
    return false;
  }
  if (count == 0 || count > MAX_WRITE_REGISTERS) {
    ESP_LOGE(TAG, "Write of %u registers not supported", count);
    return false;
  }

  ModbusRequest request{};
  request.address = address;
  request.function = MODBUS_FC_WRITE_MULTIPLE_REGISTERS;
  request.start_register = start_register;
  request.num_registers = count;
  for (uint8_t i = 0; i < count; i++)
    request.values[i] = values[i];
  request.queued_ms = millis();
  this->request_queue_.push(request);
  return true;
}

void EG4Modbus::cancel_writes(uint8_t address) {
  std::queue<ModbusRequest> kept;
  size_t dropped = 0;
  while (!this->request_queue_.empty()) {
    const ModbusRequest &r = this->request_queue_.front();
    if (r.address == address && r.function == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
      dropped++;
    } else {
      kept.push(r);
    }
    this->request_queue_.pop();
  }
  this->request_queue_.swap(kept);
  if (dropped > 0)
    ESP_LOGW(TAG, "Cancelled %u queued write(s) to 0x%02X", (unsigned) dropped, address);
}

void EG4Modbus::send_next_request_() {
  if (this->request_queue_.empty() || this->waiting_for_response_) {
    return;
  }

  ModbusRequest request = this->request_queue_.front();
  this->request_queue_.pop();

  if (request.function == MODBUS_FC_WRITE_MULTIPLE_REGISTERS && millis() - request.queued_ms > WRITE_MAX_AGE_MS) {
    ESP_LOGW(TAG, "Dropped write to 0x%02X register 0x%04X: queued %u ms ago", request.address,
             request.start_register, (unsigned) (millis() - request.queued_ms));
    return;  // the next loop() sends the following request
  }

  // FC03: addr fc start(2) qty(2) crc(2). FC16 adds a byte count and the
  // register values: addr 10 start(2) qty(2) n values(n) crc(2).
  uint8_t frame[9 + 2 * MAX_WRITE_REGISTERS];
  frame[0] = request.address;
  frame[1] = request.function;
  frame[2] = request.start_register >> 8;
  frame[3] = request.start_register & 0xFF;
  frame[4] = request.num_registers >> 8;
  frame[5] = request.num_registers & 0xFF;
  size_t len = 6;
  if (request.function == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
    frame[len++] = request.num_registers * 2;
    for (uint16_t i = 0; i < request.num_registers; i++) {
      frame[len++] = request.values[i] >> 8;
      frame[len++] = request.values[i] & 0xFF;
    }
  }

  uint16_t crc = crc16_modbus(frame, len);
  frame[len++] = crc & 0xFF;
  frame[len++] = crc >> 8;

  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->digital_write(true);
  }

  this->write_array(frame, len);
  this->flush();

  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->digital_write(false);
  }

  ESP_LOGV(TAG, "Sent: %s", format_hex_pretty(frame, len).c_str());
  this->requests_.on_request(request.address, request.function, request.start_register, request.num_registers);
  this->last_send_ = millis();
  this->waiting_for_response_ = true;
}

bool EG4Modbus::parse_modbus_byte_(uint8_t byte) {
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

  if (function == MODBUS_FC_READ_HOLDING_REGISTERS) {
    if (len >= 3) {
      uint8_t byte_count = raw[2];
      expected_len = 3 + byte_count + 2;  // addr + func + count + data + crc
    }
  } else if (function == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
    expected_len = 8;  // addr + func + start(2) + qty(2) + crc
  } else if ((function & MODBUS_EXCEPTION_FLAG) != 0) {
    // Error response
    expected_len = 5;  // addr + func + error_code + crc
  }

  if (expected_len == 0 || len < expected_len) {
    return true;  // Need more data
  }

  // Verify CRC
  uint16_t crc_calc = crc16_modbus(raw, expected_len - 2);
  uint16_t crc_recv = raw[expected_len - 2] | (raw[expected_len - 1] << 8);

  if (crc_calc != crc_recv) {
    ESP_LOGW(TAG, "Modbus CRC check failed! Calculated: 0x%04X, Received: 0x%04X", crc_calc, crc_recv);
    this->rx_buffer_.clear();
    return false;
  }

  ESP_LOGV(TAG, "Received: %s", format_hex_pretty(raw, expected_len).c_str());

  // This is a strict request/response queue, so the response answers the
  // request last sent to its address - that request supplies the start register.
  if ((function & MODBUS_EXCEPTION_FLAG) != 0) {
    uint16_t start_register = MODBUS_START_REGISTER_UNKNOWN;
    this->requests_.take(address, function, 0, &start_register);
    this->dispatch_(address, function, start_register, raw + 2, 1);
  } else if (function == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
    // The acknowledgement echoes the start register and quantity; it must
    // match the write we sent.
    const uint16_t echoed_start = (uint16_t(raw[2]) << 8) | raw[3];
    const uint16_t echoed_qty = (uint16_t(raw[4]) << 8) | raw[5];
    uint16_t start_register;
    if (this->requests_.take(address, function, echoed_qty, &start_register) && start_register == echoed_start) {
      this->dispatch_(address, function, start_register, raw + 2, 4);
    } else {
      ESP_LOGW(TAG, "Write acknowledgement from 0x%02X (0x%04X x%u) does not match the write sent - ignored",
               address, echoed_start, echoed_qty);
    }
  } else {
    const uint8_t byte_count = raw[2];
    uint16_t start_register;
    if (this->requests_.take(address, function, byte_count / 2, &start_register)) {
      this->dispatch_(address, function, start_register, raw + 3, byte_count);
    } else {
      ESP_LOGW(TAG, "Response from 0x%02X (%u bytes) does not match the request sent - ignored", address,
               byte_count);
    }
  }

  this->rx_buffer_.clear();
  this->waiting_for_response_ = false;  // Ready for next request
  return true;
}

}  // namespace eg4_modbus
}  // namespace esphome
