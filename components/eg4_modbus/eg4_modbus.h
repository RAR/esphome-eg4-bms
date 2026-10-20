#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "eg4_modbus_frame.h"
#include <queue>

namespace esphome {
namespace eg4_modbus {

class EG4ModbusDevice;

enum EG4ModbusMode : uint8_t {
  // Poll the devices: the hub is the bus master (default).
  EG4_MODBUS_MODE_ACTIVE = 0,
  // Listen only: another master (e.g. the DIP-ID-1 pack on the link port) polls
  // the devices and the hub decodes both directions without ever transmitting.
  EG4_MODBUS_MODE_PASSIVE = 1,
};

// Largest FC16 write the hub queues: the BMS tool writes one register per
// frame, except the clock (0x2D-0x2E), which takes two.
static const uint8_t MAX_WRITE_REGISTERS = 2;

struct ModbusRequest {
  uint8_t address;
  uint8_t function;
  uint16_t start_register;
  uint16_t num_registers;
  uint16_t values[MAX_WRITE_REGISTERS];  // FC16 only
  uint32_t queued_ms;                    // FC16 only: dropped if not sent within WRITE_MAX_AGE_MS
};

// A write still queued after this long is dropped instead of sent: the user
// asked for it seconds ago, against a BMS that may since have gone away.
static const uint32_t WRITE_MAX_AGE_MS = 5000;

class EG4Modbus : public uart::UARTDevice, public Component {
 public:
  EG4Modbus() = default;

  void setup() override;
  void loop() override;
  void dump_config() override;

  void register_device(EG4ModbusDevice *device) { this->devices_.push_back(device); }

  float get_setup_priority() const override;

  void send(uint8_t address, uint8_t function, uint16_t start_register, uint16_t num_registers);
  // Queues an FC16 write of 1..MAX_WRITE_REGISTERS registers. Returns false
  // (nothing queued) in passive mode or for an invalid count.
  bool write_registers(uint8_t address, uint16_t start_register, const uint16_t *values, uint8_t count);
  // Drops every queued (not yet sent) write to this address.
  void cancel_writes(uint8_t address);
  void set_flow_control_pin(GPIOPin *flow_control_pin) { this->flow_control_pin_ = flow_control_pin; }
  void set_mode(EG4ModbusMode mode) { this->mode_ = mode; }
  bool is_passive() const { return this->mode_ == EG4_MODBUS_MODE_PASSIVE; }

 protected:
  GPIOPin *flow_control_pin_{nullptr};
  EG4ModbusMode mode_{EG4_MODBUS_MODE_ACTIVE};

  bool parse_modbus_byte_(uint8_t byte);
  void send_next_request_();
  void loop_passive_();
  void handle_passive_frame_(const Frame &frame);
  void dispatch_(uint8_t address, uint8_t function, uint16_t start_register, const uint8_t *data, size_t len);

  std::vector<uint8_t> rx_buffer_;
  uint32_t last_modbus_byte_{0};
  uint32_t last_send_{0};
  std::vector<EG4ModbusDevice *> devices_;

  std::queue<ModbusRequest> request_queue_;
  bool waiting_for_response_{false};

  // Request each address was last asked for (active: sent by us; passive: seen
  // from the bus master), used to give responses their start register.
  RequestTracker requests_;

  // Passive mode only.
  FrameSplitter splitter_;
  uint32_t last_rx_{0};
  uint32_t reported_dropped_{0};
};

class EG4ModbusDevice {
 public:
  void set_parent(EG4Modbus *parent) { parent_ = parent; }
  void set_address(uint8_t address) { address_ = address; }

  // Called for every response addressed to this device.
  //   function:       as on the wire; the 0x80 bit marks an exception response.
  //                   An FC16 (0x10) write acknowledgement has data/len = the
  //                   echoed start register and quantity (4 bytes).
  //   start_register: first register the block holds, taken from the request
  //                   it answers (MODBUS_START_REGISTER_UNKNOWN if unknown)
  //   data/len:       FC03 register data (big-endian, len = 2 x registers),
  //                   or the single exception code byte
  virtual void on_modbus_block(uint8_t address, uint8_t function, uint16_t start_register, const uint8_t *data,
                               size_t len) = 0;

  void send(uint8_t function, uint16_t start_register, uint16_t num_registers) {
    this->parent_->send(this->address_, function, start_register, num_registers);
  }
  bool write_registers(uint16_t start_register, const uint16_t *values, uint8_t count) {
    return this->parent_->write_registers(this->address_, start_register, values, count);
  }
  bool is_passive() const { return this->parent_ != nullptr && this->parent_->is_passive(); }

 protected:
  friend EG4Modbus;

  EG4Modbus *parent_{nullptr};
  uint8_t address_;
};

}  // namespace eg4_modbus
}  // namespace esphome
