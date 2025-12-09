#pragma once

#include "../eg4_bms.h"
#include "esphome/core/component.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace eg4_bms {

class EG4Bms;

class Eg4Number : public number::Number, public Component {
 public:
  void set_parent(EG4Bms *parent) { this->parent_ = parent; };
  void set_holding_register(uint16_t holding_register) { this->holding_register_ = holding_register; };
  void set_factor(float factor) { this->factor_ = factor; };
  void dump_config() override;
  void loop() override {}
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(float value) override;
  EG4Bms *parent_;
  uint16_t holding_register_;
  float factor_{1.0};
};

}  // namespace eg4_bms
}  // namespace esphome
