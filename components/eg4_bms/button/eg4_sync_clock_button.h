#pragma once

#include "../eg4_bms.h"
#include "esphome/components/button/button.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace eg4_bms {

class EG4SyncClockButton : public button::Button {
 public:
  void set_parent(EG4Bms *parent) { this->parent_ = parent; }
  void set_time(time::RealTimeClock *time) { this->time_ = time; }

 protected:
  void press_action() override { this->parent_->write_clock(this->time_->now()); }

  EG4Bms *parent_{nullptr};
  time::RealTimeClock *time_{nullptr};
};

}  // namespace eg4_bms
}  // namespace esphome
