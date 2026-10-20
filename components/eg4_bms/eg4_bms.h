#pragma once

#include "esphome/core/component.h"
#include "esphome/core/time.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/eg4_modbus/eg4_modbus.h"
#include "eg4_bms_settings.h"

#include <bitset>
#include <vector>

namespace esphome {
namespace eg4_bms {

// Receives a setting's current value whenever the BMS reports it (the
// number platform implements this).
class SettingListener {
 public:
  virtual void on_setting_value(float value) = 0;
};

class EG4Bms : public PollingComponent, public eg4_modbus::EG4ModbusDevice {
 public:
  // Binary sensors
  void set_online_status_binary_sensor(binary_sensor::BinarySensor *online_status) {
    online_status_binary_sensor_ = online_status;
  }
  void set_heating_binary_sensor(binary_sensor::BinarySensor *heating) { heating_binary_sensor_ = heating; }
  void set_charging_binary_sensor(binary_sensor::BinarySensor *charging) { charging_binary_sensor_ = charging; }
  void set_discharging_binary_sensor(binary_sensor::BinarySensor *discharging) {
    discharging_binary_sensor_ = discharging;
  }

  // Voltage sensors
  void set_total_voltage_sensor(sensor::Sensor *total_voltage) { total_voltage_sensor_ = total_voltage; }
  void set_cell_voltage_sensor(uint8_t cell, sensor::Sensor *cell_voltage) {
    this->cells_[cell].cell_voltage_sensor_ = cell_voltage;
  }
  void set_min_cell_voltage_sensor(sensor::Sensor *min_cell_voltage) { min_cell_voltage_sensor_ = min_cell_voltage; }
  void set_max_cell_voltage_sensor(sensor::Sensor *max_cell_voltage) { max_cell_voltage_sensor_ = max_cell_voltage; }
  void set_delta_cell_voltage_sensor(sensor::Sensor *delta_cell_voltage) {
    delta_cell_voltage_sensor_ = delta_cell_voltage;
  }
  void set_cell_average_voltage_sensor(sensor::Sensor *cell_average_voltage) {
    cell_average_voltage_sensor_ = cell_average_voltage;
  }
  void set_min_voltage_cell_sensor(sensor::Sensor *min_voltage_cell) {
    min_voltage_cell_sensor_ = min_voltage_cell;
  }
  void set_max_voltage_cell_sensor(sensor::Sensor *max_voltage_cell) {
    max_voltage_cell_sensor_ = max_voltage_cell;
  }

  // Current and power sensors
  void set_current_sensor(sensor::Sensor *current) { current_sensor_ = current; }
  void set_power_sensor(sensor::Sensor *power) { power_sensor_ = power; }
  void set_charging_power_sensor(sensor::Sensor *charging_power) { charging_power_sensor_ = charging_power; }
  void set_discharging_power_sensor(sensor::Sensor *discharging_power) {
    discharging_power_sensor_ = discharging_power;
  }

  // Temperature sensors
  void set_temperature_sensor(uint8_t temp, sensor::Sensor *temperature) {
    this->temperatures_[temp].temperature_sensor_ = temperature;
  }
  void set_pcb_temperature_sensor(sensor::Sensor *pcb_temp) { pcb_temperature_sensor_ = pcb_temp; }
  void set_avg_temperature_sensor(sensor::Sensor *avg_temp) { avg_temperature_sensor_ = avg_temp; }
  void set_max_temperature_sensor(sensor::Sensor *max_temp) { max_temperature_sensor_ = max_temp; }

  // Capacity sensors
  void set_remaining_capacity_sensor(sensor::Sensor *remaining_capacity) {
    remaining_capacity_sensor_ = remaining_capacity;
  }
  void set_full_capacity_sensor(sensor::Sensor *full_capacity) { full_capacity_sensor_ = full_capacity; }
  void set_designed_capacity_sensor(sensor::Sensor *designed_capacity) {
    designed_capacity_sensor_ = designed_capacity;
  }

  // State sensors
  void set_state_of_charge_sensor(sensor::Sensor *soc) { state_of_charge_sensor_ = soc; }
  void set_state_of_health_sensor(sensor::Sensor *soh) { state_of_health_sensor_ = soh; }
  void set_cycle_count_sensor(sensor::Sensor *cycle_count) { cycle_count_sensor_ = cycle_count; }
  void set_max_charge_current_sensor(sensor::Sensor *max_charge_current) {
    max_charge_current_sensor_ = max_charge_current;
  }
  void set_cell_count_sensor(sensor::Sensor *cell_count) { cell_count_sensor_ = cell_count; }

  // Diagnostics
  void set_rejected_frame_count_sensor(sensor::Sensor *rejected_frame_count) {
    rejected_frame_count_sensor_ = rejected_frame_count;
  }
  void set_max_no_response_count(uint8_t count) { max_no_response_count_ = count; }

  // Bitmask sensors
  void set_errors_bitmask_sensor(sensor::Sensor *errors_bitmask) { errors_bitmask_sensor_ = errors_bitmask; }
  void set_warnings_bitmask_sensor(sensor::Sensor *warnings_bitmask) {
    warnings_bitmask_sensor_ = warnings_bitmask;
  }
  void set_protection_bitmask_sensor(sensor::Sensor *protection_bitmask) {
    protection_bitmask_sensor_ = protection_bitmask;
  }

  // Text sensors
  void set_status_text_sensor(text_sensor::TextSensor *status) { status_text_sensor_ = status; }
  void set_warnings_text_sensor(text_sensor::TextSensor *warnings) { warnings_text_sensor_ = warnings; }
  void set_protection_text_sensor(text_sensor::TextSensor *protection) { protection_text_sensor_ = protection; }
  void set_error_text_sensor(text_sensor::TextSensor *error) { error_text_sensor_ = error; }
  void set_model_text_sensor(text_sensor::TextSensor *model) { model_text_sensor_ = model; }
  void set_firmware_version_text_sensor(text_sensor::TextSensor *firmware) { firmware_text_sensor_ = firmware; }
  void set_serial_number_text_sensor(text_sensor::TextSensor *serial) { serial_number_text_sensor_ = serial; }
  void set_bms_time_text_sensor(text_sensor::TextSensor *bms_time) { bms_time_text_sensor_ = bms_time; }

  // Parameter block (0x2D-0x87, RS232 service port only). Each setting sensor
  // publishes raw * multiplier + offset of one register; configuring any of
  // them (or bms_time) makes update() read the block.
  void add_setting_sensor(uint16_t reg, float multiplier, float offset, sensor::Sensor *sensor) {
    this->setting_sensors_.push_back({reg, multiplier, offset, sensor});
  }
  void set_read_settings(bool read_settings) { read_settings_ = read_settings; }

  // Writing settings (RS232 service port). Every write needs writes enabled
  // (the write-enable switch), a parameter block read from the BMS, a value
  // in range and the ordering rules still holding; it is then sent as one
  // FC16 per register, like the BMS tool, and verified by re-reading the
  // block.
  void add_setting_number(uint16_t reg, float multiplier, float offset, float min_value, float max_value,
                          SettingListener *listener) {
    this->setting_numbers_.push_back({reg, multiplier, offset, min_value, max_value, listener});
  }
  // lower must stay <= upper, or < upper when strict.
  void add_setting_order(uint16_t lower_reg, uint16_t upper_reg, bool strict) {
    this->setting_order_.push_back({lower_reg, upper_reg, strict});
  }
  void set_writes_enabled(bool enabled);
  bool get_writes_enabled() const { return writes_enabled_; }
  // Returns false (and re-publishes the BMS's value) when the write is refused.
  bool write_setting(uint16_t reg, float value);
  bool write_clock(const ESPTime &time);

  void on_modbus_block(uint8_t address, uint8_t function, uint16_t start_register, const uint8_t *data,
                       size_t len) override;

  void dump_config() override;
  void update() override;
  float get_setup_priority() const override;

 protected:
  // Binary sensors
  binary_sensor::BinarySensor *online_status_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *heating_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *charging_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *discharging_binary_sensor_{nullptr};

  // Voltage sensors
  sensor::Sensor *total_voltage_sensor_{nullptr};
  sensor::Sensor *min_cell_voltage_sensor_{nullptr};
  sensor::Sensor *max_cell_voltage_sensor_{nullptr};
  sensor::Sensor *delta_cell_voltage_sensor_{nullptr};
  sensor::Sensor *cell_average_voltage_sensor_{nullptr};
  sensor::Sensor *min_voltage_cell_sensor_{nullptr};
  sensor::Sensor *max_voltage_cell_sensor_{nullptr};

  // Current and power sensors
  sensor::Sensor *current_sensor_{nullptr};
  sensor::Sensor *power_sensor_{nullptr};
  sensor::Sensor *charging_power_sensor_{nullptr};
  sensor::Sensor *discharging_power_sensor_{nullptr};

  // Temperature sensors
  sensor::Sensor *pcb_temperature_sensor_{nullptr};
  sensor::Sensor *avg_temperature_sensor_{nullptr};
  sensor::Sensor *max_temperature_sensor_{nullptr};

  // Capacity sensors
  sensor::Sensor *remaining_capacity_sensor_{nullptr};
  sensor::Sensor *full_capacity_sensor_{nullptr};
  sensor::Sensor *designed_capacity_sensor_{nullptr};

  // State sensors
  sensor::Sensor *state_of_charge_sensor_{nullptr};
  sensor::Sensor *state_of_health_sensor_{nullptr};
  sensor::Sensor *cycle_count_sensor_{nullptr};
  sensor::Sensor *max_charge_current_sensor_{nullptr};
  sensor::Sensor *cell_count_sensor_{nullptr};

  // Diagnostics
  sensor::Sensor *rejected_frame_count_sensor_{nullptr};

  // Bitmask sensors
  sensor::Sensor *errors_bitmask_sensor_{nullptr};
  sensor::Sensor *warnings_bitmask_sensor_{nullptr};
  sensor::Sensor *protection_bitmask_sensor_{nullptr};

  // Text sensors
  text_sensor::TextSensor *status_text_sensor_{nullptr};
  text_sensor::TextSensor *warnings_text_sensor_{nullptr};
  text_sensor::TextSensor *protection_text_sensor_{nullptr};
  text_sensor::TextSensor *error_text_sensor_{nullptr};
  text_sensor::TextSensor *model_text_sensor_{nullptr};
  text_sensor::TextSensor *firmware_text_sensor_{nullptr};
  text_sensor::TextSensor *serial_number_text_sensor_{nullptr};
  text_sensor::TextSensor *bms_time_text_sensor_{nullptr};

  struct SettingSensor {
    uint16_t reg;
    float multiplier;
    float offset;
    sensor::Sensor *sensor;
  };
  std::vector<SettingSensor> setting_sensors_;
  bool read_settings_{false};
  bool blank_settings_logged_{false};

  struct SettingNumber {
    uint16_t reg;
    float multiplier;
    float offset;
    float min_value;
    float max_value;
    SettingListener *listener;
  };
  std::vector<SettingNumber> setting_numbers_;
  struct SettingOrder {
    uint16_t lower_reg;
    uint16_t upper_reg;
    bool strict;
  };
  std::vector<SettingOrder> setting_order_;
  bool writes_enabled_{false};
  // True once a parameter block with real thresholds has been stored, so
  // registers_ holds the BMS's values for the ordering checks.
  bool settings_seen_{false};
  // Writes queued but not yet confirmed by a re-read. The ordering checks use
  // these values over registers_ so two quick writes cannot together break
  // an order neither breaks alone. Only acknowledged writes are verified (a
  // read queued before the write would still show the old value); entries
  // expire after WRITE_EXPIRY_MS.
  struct ExpectedWrite {
    uint16_t reg;
    uint16_t value;
    bool acked;
    uint32_t queued_ms;
  };
  std::vector<ExpectedWrite> expected_writes_;
  static const uint32_t WRITE_EXPIRY_MS = 10000;

  void republish_setting_(uint16_t reg);
  void verify_writes_(uint16_t start_register, size_t count);
  void expire_writes_();
  uint16_t effective_register_(uint16_t reg) const;
  // Forgets the parameter block (offline, another pack, blank block) so no
  // write is checked against values this BMS may not hold.
  void forget_settings_(const char *why);

  struct Cell {
    sensor::Sensor *cell_voltage_sensor_{nullptr};
  } cells_[16];

  struct Temperature {
    sensor::Sensor *temperature_sensor_{nullptr};
  } temperatures_[6];

  // Local image of registers 0x00-0x87. Accepted blocks are written here and
  // the registers they covered are marked pending; publish_pending_() then
  // decodes every sensor whose registers are all pending, so any block layout
  // that covers a sensor's registers decodes it (active mode's five small
  // blocks as well as the link master's single 0x00 x45 block).
  static const uint16_t REGISTER_IMAGE_SIZE = REG_SETTINGS_LAST + 1;
  uint16_t registers_[REGISTER_IMAGE_SIZE]{};
  std::bitset<REGISTER_IMAGE_SIZE> pending_;
  bool cell_count_register_seen_{false};

  // Passive mode: when a valid block for this device was last seen on the bus.
  bool seen_{false};
  uint32_t last_seen_ms_{0};

  uint8_t request_step_{0};
  uint8_t text_request_step_{0};
  uint8_t no_response_count_{0};
  uint8_t max_no_response_count_{5};
  uint32_t update_counter_{0};
  uint32_t rejected_frame_count_{0};

  void publish_state_(binary_sensor::BinarySensor *binary_sensor, const bool &state);
  void publish_state_(sensor::Sensor *sensor, float value);
  void publish_state_(text_sensor::TextSensor *text_sensor, const std::string &state);
  
  // Returns false when the block was rejected as implausible, in which case it
  // must not be stored or published and must not count as the BMS being online.
  bool validate_block_(uint16_t start_register, const uint8_t *data, size_t len);
  void store_block_(uint16_t start_register, const uint8_t *data, size_t len);
  void publish_pending_();
  void on_device_answered_();
  void update_passive_();

  void reject_block_(const uint8_t *data, size_t len, const char *reason);

  void reset_online_status_tracker_();
  void track_online_status_();
  void publish_device_unavailable_();
  
  std::string extract_ascii_string_(const uint8_t *data, size_t length);
  std::string decode_status_(uint16_t status);
  std::string decode_warnings_(uint16_t warnings);
  std::string decode_protection_(uint16_t protection);
  std::string decode_error_(uint16_t error);
};

}  // namespace eg4_bms
}  // namespace esphome
