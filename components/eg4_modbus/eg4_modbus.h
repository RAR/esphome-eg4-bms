#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <queue>

namespace esphome {
namespace eg4_modbus {

class EG4ModbusDevice;

struct ModbusRequest {
  uint8_t address;
  uint8_t function;
  uint16_t start_register;
  uint16_t num_registers;
};

class EG4Modbus : public uart::UARTDevice, public Component {
 public:
  EG4Modbus() = default;

  void setup() override;
  void loop() override;
  void dump_config() override;

  void register_device(EG4ModbusDevice *device) { this->devices_.push_back(device); }

  float get_setup_priority() const override;

  void send(uint8_t address, uint8_t function, uint16_t start_register, uint16_t num_registers);
  void send_write_single(uint8_t address, uint16_t register_address, uint16_t value);
  void set_flow_control_pin(GPIOPin *flow_control_pin) { this->flow_control_pin_ = flow_control_pin; }

 protected:
  GPIOPin *flow_control_pin_{nullptr};

  bool parse_modbus_byte_(uint8_t byte);
  void send_next_request_();
  
  std::vector<uint8_t> rx_buffer_;
  uint32_t last_modbus_byte_{0};
  uint32_t last_send_{0};
  std::vector<EG4ModbusDevice *> devices_;
  
  std::queue<ModbusRequest> request_queue_;
  bool waiting_for_response_{false};
};

uint16_t crc16_modbus(const uint8_t *data, uint16_t len);

class EG4ModbusDevice {
 public:
  void set_parent(EG4Modbus *parent) { parent_ = parent; }
  void set_address(uint8_t address) { address_ = address; }
  virtual void on_modbus_data(const std::vector<uint8_t> &data) = 0;
  void send(uint8_t function, uint16_t start_register, uint16_t num_registers) {
    this->parent_->send(this->address_, function, start_register, num_registers);
  }
  void write_register(uint16_t register_address, uint16_t value) {
    this->parent_->send_write_single(this->address_, register_address, value);
  }

 protected:
  friend EG4Modbus;

  EG4Modbus *parent_;
  uint8_t address_;
};

}  // namespace eg4_modbus
}  // namespace esphome
