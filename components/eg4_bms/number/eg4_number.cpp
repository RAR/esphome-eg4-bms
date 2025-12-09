#include "eg4_number.h"
#include "esphome/core/log.h"

namespace esphome {
namespace eg4_bms {

static const char *const TAG = "eg4_bms.number";

void Eg4Number::dump_config() { LOG_NUMBER("", "EG4 BMS Number", this); }

void Eg4Number::control(float value) {
  // Convert user value to register value using factor
  // For temperatures with +50 offset, the factor is 1.0 and we add 50 here
  // For voltages/currents, the factor handles unit conversion
  uint16_t register_value;
  
  // Check if this is a temperature register (registers 0x005A-0x0068, 0x0080-0x0087)
  // These use +50 offset encoding
  bool is_temperature = (this->holding_register_ >= 0x005A && this->holding_register_ <= 0x0068) ||
                        (this->holding_register_ >= 0x0080 && this->holding_register_ <= 0x0087);
  
  if (is_temperature) {
    // Temperature: add 50 offset, then cast to uint16_t
    register_value = static_cast<uint16_t>(value + 50.0f);
  } else {
    // Other values: multiply by factor (for unit conversion)
    register_value = static_cast<uint16_t>(value * this->factor_);
  }
  
  ESP_LOGD(TAG, "Writing %.3f to register 0x%04X (raw value: %d, factor: %.1f, is_temp: %d)", 
           value, this->holding_register_, register_value, this->factor_, is_temperature);
  
  this->parent_->write_register(this->holding_register_, register_value);
  this->publish_state(value);
}

}  // namespace eg4_bms
}  // namespace esphome
