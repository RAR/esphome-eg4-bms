#pragma once

#include "../eg4_bms.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace eg4_bms {

// A parameter block setting. Shows the value the BMS last reported; setting it
// asks EG4Bms::write_setting(), which validates, writes and re-reads.
class EG4SettingNumber : public number::Number, public SettingListener {
 public:
  void set_parent(EG4Bms *parent) { this->parent_ = parent; }
  void set_register(uint16_t reg) { this->reg_ = reg; }
  void on_setting_value(float value) override { this->publish_state(value); }

 protected:
  void control(float value) override { this->parent_->write_setting(this->reg_, value); }

  EG4Bms *parent_{nullptr};
  uint16_t reg_{0};
};

}  // namespace eg4_bms
}  // namespace esphome
