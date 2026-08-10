#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/eg4_modbus/eg4_modbus.h"

namespace esphome {
namespace eg4_bms {

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

  void on_modbus_data(const std::vector<uint8_t> &data) override;

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

  struct Cell {
    sensor::Sensor *cell_voltage_sensor_{nullptr};
  } cells_[16];

  struct Temperature {
    sensor::Sensor *temperature_sensor_{nullptr};
  } temperatures_[6];

  uint8_t request_step_{0};
  uint8_t text_request_step_{0};
  uint8_t no_response_count_{0};
  uint8_t max_no_response_count_{5};
  uint32_t update_counter_{0};
  uint32_t rejected_frame_count_{0};

  void publish_state_(binary_sensor::BinarySensor *binary_sensor, const bool &state);
  void publish_state_(sensor::Sensor *sensor, float value);
  void publish_state_(text_sensor::TextSensor *text_sensor, const std::string &state);
  
  // Returns false when the block was rejected as implausible, in which case no
  // state was published and the online tracker must not be reset.
  bool on_status_data_(const std::vector<uint8_t> &data);

  void reject_block_(const std::vector<uint8_t> &data, const char *reason);

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
