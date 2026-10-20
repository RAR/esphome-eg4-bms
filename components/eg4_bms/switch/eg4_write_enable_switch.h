#pragma once

#include "../eg4_bms.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace eg4_bms {

class EG4WriteEnableSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(EG4Bms *parent) { this->parent_ = parent; }
  // Always boots disarmed, whatever restore_mode says.
  void setup() override {
    this->parent_->set_writes_enabled(false);
    this->publish_state(false);
  }

 protected:
  void write_state(bool state) override {
    this->parent_->set_writes_enabled(state);
    this->publish_state(state);
  }

  EG4Bms *parent_{nullptr};
};

}  // namespace eg4_bms
}  // namespace esphome
