#include "eg4_bms.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace eg4_bms {

static const char *const TAG = "eg4_bms";

static const uint8_t FUNCTION_READ_HOLDING = 0x03;
static const uint8_t MAX_NO_RESPONSE_COUNT = 5;

// Register addresses (from working YamBMS modbus_controller implementation)
static const uint16_t REG_VOLTAGE = 0x0000;          // Total voltage (10mV)
static const uint16_t REG_CURRENT = 0x0001;          // Current (10mA, signed)
static const uint16_t REG_CELL_VOLTAGE = 0x0002;     // Cell voltages start (mV, 16 cells)
static const uint16_t REG_TEMP_PCB = 0x0012;         // PCB/MOSFET temperature (°C, signed)
static const uint16_t REG_TEMP_AVG = 0x0013;         // Average temperature (°C, signed)
static const uint16_t REG_TEMP_MAX = 0x0014;         // Max temperature (°C, signed)
static const uint16_t REG_CAP_REMAINING = 0x0015;    // Remaining capacity (Ah)
static const uint16_t REG_MAX_CHARGE_CURRENT = 0x0016;  // Max charging current (A)
static const uint16_t REG_SOH = 0x0017;              // State of health (0-100%)
static const uint16_t REG_SOC = 0x0018;              // State of charge (0-100%)
static const uint16_t REG_STATUS = 0x0019;           // Status
static const uint16_t REG_WARNING = 0x001A;          // Warnings
static const uint16_t REG_PROTECTION = 0x001B;       // Protection status
static const uint16_t REG_ERROR = 0x001C;            // Error code
static const uint16_t REG_CYCLE_COUNT = 0x001D;      // Cycle counts (4 bytes)
static const uint16_t REG_TEMPS = 0x0021;            // Temperature sensors 1-6 (6 bytes, packed)
static const uint16_t REG_BALANCING = 0x0026;        // Balancing status
static const uint16_t REG_FULL_CAPACITY = 0x0025;    // Full capacity (0.1 Ah units)
static const uint16_t REG_MODEL = 0x0069;            // Model (22 bytes)
static const uint16_t REG_FW_VERSION = 0x0075;       // Firmware version (6 bytes)
static const uint16_t REG_SERIAL_NO = 0x0078;        // Serial number (16 bytes)

void EG4Bms::dump_config() {
  ESP_LOGCONFIG(TAG, "EG4 BMS:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);
  LOG_BINARY_SENSOR("  ", "Online Status", this->online_status_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Heating", this->heating_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Charging", this->charging_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Discharging", this->discharging_binary_sensor_);
  LOG_SENSOR("  ", "Total Voltage", this->total_voltage_sensor_);
  LOG_SENSOR("  ", "Current", this->current_sensor_);
  LOG_SENSOR("  ", "Power", this->power_sensor_);
  LOG_SENSOR("  ", "State of Charge", this->state_of_charge_sensor_);
  LOG_TEXT_SENSOR("  ", "Status", this->status_text_sensor_);
}

float EG4Bms::get_setup_priority() const { return setup_priority::DATA; }

void EG4Bms::update() {
  if (this->no_response_count_ >= MAX_NO_RESPONSE_COUNT) {
    this->publish_device_unavailable_();
    ESP_LOGW(TAG, "No response from BMS (address 0x%02X)", this->address_);
  }

  this->track_online_status_();
  this->no_response_count_++;

  // Cycle through main register blocks and config registers
  switch (this->request_step_) {
    case 0:
      // Request main data block (voltage, current, cell voltages)
      ESP_LOGV(TAG, "Requesting registers 0x%04X, count=%d", REG_VOLTAGE, 0x12);
      this->send(FUNCTION_READ_HOLDING, REG_VOLTAGE, 0x12);  // Read 18 registers (0x00-0x11)
      break;
    case 1:
      // Request temps, capacities, SOC, SOH, status, warnings, errors
      ESP_LOGV(TAG, "Requesting registers 0x%04X, count=%d", REG_TEMP_PCB, 0x15);
      this->send(FUNCTION_READ_HOLDING, REG_TEMP_PCB, 0x15);  // Read 21 registers (0x12-0x26)
      break;
    case 2:
      // Request first config block: 0x002D-0x0054 (40 registers = 80 bytes)
      // Includes balance voltage, diff, cell OV/UV limits
      ESP_LOGD(TAG, "Requesting config block 1: registers 0x002D, count=40 (0x28)");
      this->send(FUNCTION_READ_HOLDING, 0x002D, 0x28);
      break;
    case 3:
      // Request second config block: 0x0055-0x0087 (51 registers = 102 bytes)
      // Includes temp limits, charge/discharge limits
      ESP_LOGD(TAG, "Requesting config block 2: registers 0x0055, count=51 (0x33)");
      this->send(FUNCTION_READ_HOLDING, 0x0055, 0x33);
      break;
  }

  this->request_step_ = (this->request_step_ + 1) % 4;

  // Request text sensor data occasionally (every 30 updates)
  if ((this->update_counter_ % 30) == 0) {
    // Cycle through text sensor requests
    switch (this->text_request_step_) {
      case 0:
        this->send(FUNCTION_READ_HOLDING, REG_MODEL, 0x0B);  // Read 11 registers (22 bytes)
        break;
      case 1:
        this->send(FUNCTION_READ_HOLDING, REG_FW_VERSION, 0x03);  // Read 3 registers (6 bytes)
        break;
      case 2:
        this->send(FUNCTION_READ_HOLDING, REG_SERIAL_NO, 0x08);  // Read 8 registers (16 bytes)
        break;
    }
    this->text_request_step_ = (this->text_request_step_ + 1) % 3;
  }

  this->update_counter_++;
}

void EG4Bms::on_modbus_data(const std::vector<uint8_t> &data) {
  if (data.size() < 5) {
    ESP_LOGW(TAG, "Invalid Modbus response length: %d", data.size());
    return;
  }

  uint8_t address = data[0];
  uint8_t function = data[1];

  // Check if this response is for this device
  if (address != this->address_) {
    return;  // Not for this device
  }

  // Reset the no-response counter since we got a valid response for this address
  this->reset_online_status_tracker_();

  // Check for error response
  if ((function & 0x80) != 0) {
    ESP_LOGW(TAG, "Modbus error response: function=0x%02X, exception=0x%02X", function, data[2]);
    return;
  }

  if (function != FUNCTION_READ_HOLDING) {
    ESP_LOGW(TAG, "Unexpected function code: 0x%02X", function);
    return;
  }

  uint8_t byte_count = data[2];
  ESP_LOGD(TAG, "Received response: byte_count=0x%02X (%d bytes), data.size()=%d", byte_count, byte_count, data.size());
  
  if (data.size() < (size_t)(3 + byte_count + 2)) {
    ESP_LOGW(TAG, "Incomplete response");
    return;
  }

  this->on_status_data_(data);
}

void EG4Bms::on_status_data_(const std::vector<uint8_t> &data) {
  const uint8_t *payload = &data[3];  // Skip address, function, byte count
  size_t byte_count = data[2];

  auto get_16bit = [&](size_t i) -> uint16_t {
    return (uint16_t(payload[i]) << 8) | uint16_t(payload[i + 1]);
  };

  auto get_32bit = [&](size_t i) -> uint32_t {
    return (uint32_t(payload[i]) << 24) | (uint32_t(payload[i + 1]) << 16) | 
           (uint32_t(payload[i + 2]) << 8) | uint32_t(payload[i + 3]);
  };

  ESP_LOGV(TAG, "Processing %d bytes of data", byte_count);
  
  // Debug: Log first few bytes of payload for config blocks
  if (byte_count == 0x50) {
    ESP_LOGD(TAG, "Config block payload (first 20 bytes): %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8], payload[9],
             payload[10], payload[11], payload[12], payload[13], payload[14], payload[15], payload[16], payload[17], payload[18], payload[19]);
  }

  // Determine which register block this is based on byte count
  if (byte_count == 0x24) {  // 36 bytes = 18 registers (voltage, current, cells)
    // Total voltage (register 0x0000) - 10mV units
    float total_voltage = get_16bit(0) * 0.01f;
    this->publish_state_(this->total_voltage_sensor_, total_voltage);

    // Current (register 0x0001) - 10mA units, signed
    int16_t current_raw = (int16_t) get_16bit(2);
    float current = current_raw * 0.01f;
    this->publish_state_(this->current_sensor_, current);

    // Calculate power
    float power = total_voltage * current;
    this->publish_state_(this->power_sensor_, power);
    this->publish_state_(this->charging_power_sensor_, power > 0 ? power : 0);
    this->publish_state_(this->discharging_power_sensor_, power < 0 ? -power : 0);

    // Cell voltages (registers 0x0002-0x0011) - mV units
    float min_cell_voltage = 10.0f;
    float max_cell_voltage = 0.0f;
    float sum_cell_voltage = 0.0f;
    uint8_t valid_cells = 0;
    uint8_t min_cell_number = 0;
    uint8_t max_cell_number = 0;

    for (uint8_t i = 0; i < 16; i++) {
      uint16_t cell_mv = get_16bit(4 + i * 2);
      if (cell_mv > 0 && cell_mv < 5000) {  // Valid range check
        float cell_voltage = cell_mv * 0.001f;
        this->publish_state_(this->cells_[i].cell_voltage_sensor_, cell_voltage);
        
        if (cell_voltage < min_cell_voltage) {
          min_cell_voltage = cell_voltage;
          min_cell_number = i + 1;  // Cell numbers are 1-indexed
        }
        if (cell_voltage > max_cell_voltage) {
          max_cell_voltage = cell_voltage;
          max_cell_number = i + 1;  // Cell numbers are 1-indexed
        }
        sum_cell_voltage += cell_voltage;
        valid_cells++;
      }
    }

    if (valid_cells > 0) {
      this->publish_state_(this->min_cell_voltage_sensor_, min_cell_voltage);
      this->publish_state_(this->max_cell_voltage_sensor_, max_cell_voltage);
      this->publish_state_(this->delta_cell_voltage_sensor_, max_cell_voltage - min_cell_voltage);
      this->publish_state_(this->cell_average_voltage_sensor_, sum_cell_voltage / valid_cells);
      this->publish_state_(this->min_voltage_cell_sensor_, (float) min_cell_number);
      this->publish_state_(this->max_voltage_cell_sensor_, (float) max_cell_number);
    }

  } else if (byte_count == 0x2A) {  // 42 bytes = 21 registers (temps, SOC, SOH, status, etc.)
    // PCB/MOSFET temperature (register 0x0012) - °C, signed
    int16_t pcb_temp = (int16_t) get_16bit(0);
    this->publish_state_(this->pcb_temperature_sensor_, (float) pcb_temp);
    
    // Average temperature (register 0x0013) - °C, signed
    int16_t avg_temp = (int16_t) get_16bit(2);
    this->publish_state_(this->avg_temperature_sensor_, (float) avg_temp);
    
    // Max temperature (register 0x0014) - °C, signed
    int16_t max_temp = (int16_t) get_16bit(4);
    this->publish_state_(this->max_temperature_sensor_, (float) max_temp);

    // Remaining capacity (register 0x0015) - Ah
    uint16_t remaining_cap = get_16bit(6);
    this->publish_state_(this->remaining_capacity_sensor_, (float) remaining_cap);

    // Max charge current (register 0x0016) - A
    uint16_t max_charge_current = get_16bit(8);
    this->publish_state_(this->max_charge_current_sensor_, (float) max_charge_current);

    // SOH (register 0x0017) - percentage
    uint16_t soh = get_16bit(10);
    this->publish_state_(this->state_of_health_sensor_, (float) soh);

    // SOC (register 0x0018) - percentage
    uint16_t soc = get_16bit(12);
    this->publish_state_(this->state_of_charge_sensor_, (float) soc);

    // Status (register 0x0019)
    uint16_t status = get_16bit(14);
    this->publish_state_(this->status_text_sensor_, this->decode_status_(status));
    
    // Decode heating from status
    bool heating = (status & 0x8000) != 0;
    this->publish_state_(this->heating_binary_sensor_, heating);

    // Warnings (register 0x001A)
    uint16_t warnings = get_16bit(16);
    this->publish_state_(this->warnings_text_sensor_, this->decode_warnings_(warnings));
    this->publish_state_(this->warnings_bitmask_sensor_, (float) warnings);

    // Protection (register 0x001B)
    uint16_t protection = get_16bit(18);
    this->publish_state_(this->protection_text_sensor_, this->decode_protection_(protection));
    this->publish_state_(this->protection_bitmask_sensor_, (float) protection);

    // Error (register 0x001C)
    uint16_t error = get_16bit(20);
    this->publish_state_(this->error_text_sensor_, this->decode_error_(error));
    this->publish_state_(this->errors_bitmask_sensor_, (float) error);
    
    // Charging/discharging allowed based on protection bits
    // Charging blocked by: Cell OV (0x0002), Pack OV (0x0001), Charge OC (0x0010), 
    //                      Charge OT (0x0100), Charge UT (0x0400)
    // Discharging blocked by: Cell UV (0x0008), Pack UV (0x0004), Discharge OC (0x0020),
    //                         Discharge OT (0x0200), Discharge UT (0x0800), Discharge SC (0x2000)
    bool charging_allowed = !(protection & (0x0001 | 0x0002 | 0x0010 | 0x0100 | 0x0400));
    bool discharging_allowed = !(protection & (0x0004 | 0x0008 | 0x0020 | 0x0200 | 0x0800 | 0x2000));
    
    this->publish_state_(this->charging_binary_sensor_, charging_allowed);
    this->publish_state_(this->discharging_binary_sensor_, discharging_allowed);

    // Cycle count (registers 0x001D-0x001E) - 32-bit
    uint32_t cycle_count = get_32bit(22);
    this->publish_state_(this->cycle_count_sensor_, (float) cycle_count);

    // Temperature sensors 1-6 (registers 0x0021-0x0023) - packed as bytes
    // Each register contains 2 temperature values in its high and low bytes
    if (byte_count >= 0x2A) {  // Make sure we have enough data
      this->publish_state_(this->temperatures_[0].temperature_sensor_, (float) (int8_t) payload[30]);  // Reg 0x0021 high byte
      this->publish_state_(this->temperatures_[1].temperature_sensor_, (float) (int8_t) payload[31]);  // Reg 0x0021 low byte
      this->publish_state_(this->temperatures_[2].temperature_sensor_, (float) (int8_t) payload[32]);  // Reg 0x0022 high byte
      this->publish_state_(this->temperatures_[3].temperature_sensor_, (float) (int8_t) payload[33]);  // Reg 0x0022 low byte
      this->publish_state_(this->temperatures_[4].temperature_sensor_, (float) (int8_t) payload[34]);  // Reg 0x0023 high byte
      this->publish_state_(this->temperatures_[5].temperature_sensor_, (float) (int8_t) payload[35]);  // Reg 0x0023 low byte
    }

    // Full capacity (register 0x0025) - 0.1 Ah units
    uint16_t full_capacity = get_16bit(38);  // (0x0025 - 0x0012) * 2 = 38
    this->publish_state_(this->full_capacity_sensor_, full_capacity * 0.1f);

  } else if (byte_count == 0x16) {  // 22 bytes = 11 registers (model)
    std::string model((char *) payload, 22);
    // Trim null characters and whitespace
    model.erase(model.find_last_not_of(" \0") + 1);
    this->publish_state_(this->model_text_sensor_, model);

  } else if (byte_count == 0x06) {  // 6 bytes = 3 registers (firmware version)
    std::string firmware((char *) payload, 6);
    firmware.erase(firmware.find_last_not_of(" \0") + 1);
    this->publish_state_(this->firmware_text_sensor_, firmware);

  } else if (byte_count == 0x10) {  // 16 bytes = 8 registers (serial number)
    std::string serial((char *) payload, 16);
    serial.erase(serial.find_last_not_of(" \0") + 1);
    this->publish_state_(this->serial_number_text_sensor_, serial);
  
  } else if (byte_count == 0xB6) {  // 182 bytes = 91 registers (config block 0x002D-0x0087)
    ESP_LOGD(TAG, "Config block received: 182 bytes from 0x002D");
    ESP_LOGD(TAG, "Config block payload (first 20 bytes): %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8], payload[9],
             payload[10], payload[11], payload[12], payload[13], payload[14], payload[15], payload[16], payload[17], payload[18], payload[19]);
    
    // Calculate offsets from 0x002D base
    // Register 0x0038 is at offset (0x0038 - 0x002D) * 2 = 11 * 2 = 22 bytes
    // Register 0x0039 is at offset (0x0039 - 0x002D) * 2 = 12 * 2 = 24 bytes
    // Register 0x003E is at offset (0x003E - 0x002D) * 2 = 17 * 2 = 34 bytes
    // Register 0x0044 is at offset (0x0044 - 0x002D) * 2 = 23 * 2 = 46 bytes
    // Register 0x005E is at offset (0x005E - 0x002D) * 2 = 49 * 2 = 98 bytes
    // Register 0x0064 is at offset (0x0064 - 0x002D) * 2 = 55 * 2 = 110 bytes
    
    // Register 0x0038: Balance Voltage (mV)
    uint16_t balance_voltage = get_16bit(22);
    ESP_LOGD(TAG, "Balance voltage: raw=%d, %.3fV", balance_voltage, balance_voltage * 0.001f);
    this->publish_state_(this->balance_starting_voltage_number_, balance_voltage * 0.001f);
    
    // Register 0x0039: Balance Diff (mV)
    uint16_t balance_diff = get_16bit(24);
    ESP_LOGD(TAG, "Balance diff: raw=%d, %.3fV", balance_diff, balance_diff * 0.001f);
    this->publish_state_(this->balance_voltage_difference_number_, balance_diff * 0.001f);
    
    // Register 0x0044: Cell OV Protection (mV)
    uint16_t cell_ov = get_16bit(46);
    ESP_LOGD(TAG, "Cell OV protection: raw=%d, %.3fV", cell_ov, cell_ov * 0.001f);
    this->publish_state_(this->cell_overvoltage_protection_number_, cell_ov * 0.001f);
    
    // Register 0x003E: Cell UV Protection (mV)
    uint16_t cell_uv = get_16bit(34);
    ESP_LOGD(TAG, "Cell UV protection: raw=%d, %.3fV", cell_uv, cell_uv * 0.001f);
    this->publish_state_(this->cell_undervoltage_protection_number_, cell_uv * 0.001f);
    
    // Register 0x005E: Charge OT Protection (°C + 50 offset)
    uint16_t charge_ot = get_16bit(98);
    float charge_ot_temp = (float) charge_ot - 50.0f;
    ESP_LOGD(TAG, "Charge OT protection: raw=%d, temp=%.1f°C", charge_ot, charge_ot_temp);
    this->publish_state_(this->charge_overtemperature_protection_number_, charge_ot_temp);
    
    // Register 0x0064: Discharge OT Protection (°C + 50 offset)
    uint16_t discharge_ot = get_16bit(110);
    float discharge_ot_temp = (float) discharge_ot - 50.0f;
    ESP_LOGD(TAG, "Discharge OT protection: raw=%d, temp=%.1f°C", discharge_ot, discharge_ot_temp);
    this->publish_state_(this->discharge_overtemperature_protection_number_, discharge_ot_temp);
  }
}

std::string EG4Bms::decode_status_(uint16_t status) {
  std::string result;
  
  bool heating = (status & 0x8000) != 0;
  uint8_t state = status & 0x000F;
  
  if (heating) result += "Heating+";
  
  switch (state) {
    case 0x00: result += "Standby"; break;
    case 0x01: result += "Charging"; break;
    case 0x02: result += "Discharging"; break;
    case 0x04: result += "Protect"; break;
    case 0x08: result += "Charging Limited"; break;
    default: result += "Unknown"; break;
  }
  
  return result;
}

std::string EG4Bms::decode_warnings_(uint16_t warnings) {
  if (warnings == 0) return "None";
  
  std::string result;
  if (warnings & 0x0001) result += "Pack OV;";
  if (warnings & 0x0002) result += "Cell OV;";
  if (warnings & 0x0004) result += "Pack UV;";
  if (warnings & 0x0008) result += "Cell UV;";
  if (warnings & 0x0010) result += "Charge OC;";
  if (warnings & 0x0020) result += "Discharge OC;";
  if (warnings & 0x0040) result += "Abnormal Temp;";
  if (warnings & 0x0080) result += "MOS Overheat;";
  if (warnings & 0x0100) result += "Charge OT;";
  if (warnings & 0x0200) result += "Discharge OT;";
  if (warnings & 0x0400) result += "Charge UT;";
  if (warnings & 0x0800) result += "Discharge UT;";
  if (warnings & 0x1000) result += "Low Capacity;";
  if (warnings & 0x2000) result += "Float Stopped;";
  
  if (!result.empty() && result.back() == ';') result.pop_back();
  return result;
}

std::string EG4Bms::decode_protection_(uint16_t protection) {
  if (protection == 0) return "None";
  
  std::string result;
  if (protection & 0x0001) result += "Pack OV;";
  if (protection & 0x0002) result += "Cell OV;";
  if (protection & 0x0004) result += "Pack UV;";
  if (protection & 0x0008) result += "Cell UV;";
  if (protection & 0x0010) result += "Charge OC;";
  if (protection & 0x0020) result += "Discharge OC;";
  if (protection & 0x0040) result += "Abnormal Temp;";
  if (protection & 0x0080) result += "MOS Overheat;";
  if (protection & 0x0100) result += "Charge OT;";
  if (protection & 0x0200) result += "Discharge OT;";
  if (protection & 0x0400) result += "Charge UT;";
  if (protection & 0x0800) result += "Discharge UT;";
  if (protection & 0x1000) result += "Low Capacity;";
  if (protection & 0x2000) result += "Discharge SC;";
  
  if (!result.empty() && result.back() == ';') result.pop_back();
  return result;
}

std::string EG4Bms::decode_error_(uint16_t error) {
  if (error == 0) return "None";
  
  std::string result;
  if (error & 0x0001) result += "Voltage Error;";
  if (error & 0x0002) result += "Temperature Error;";
  if (error & 0x0004) result += "Current Flow Error;";
  if (error & 0x0010) result += "Cell Unbalance;";
  
  if (!result.empty() && result.back() == ';') result.pop_back();
  return result;
}

void EG4Bms::publish_state_(binary_sensor::BinarySensor *binary_sensor, const bool &state) {
  if (binary_sensor != nullptr) {
    binary_sensor->publish_state(state);
  }
}

void EG4Bms::publish_state_(sensor::Sensor *sensor, float value) {
  if (sensor != nullptr && !std::isnan(value)) {
    sensor->publish_state(value);
  }
}

void EG4Bms::publish_state_(text_sensor::TextSensor *text_sensor, const std::string &state) {
  if (text_sensor != nullptr && !state.empty()) {
    text_sensor->publish_state(state);
  }
}

void EG4Bms::publish_state_(number::Number *number, float value) {
  if (number != nullptr && !std::isnan(value)) {
    number->publish_state(value);
  }
}

void EG4Bms::reset_online_status_tracker_() {
  this->no_response_count_ = 0;
  this->publish_state_(this->online_status_binary_sensor_, true);
}

void EG4Bms::track_online_status_() {
  if (this->no_response_count_ < MAX_NO_RESPONSE_COUNT) {
    this->publish_state_(this->online_status_binary_sensor_, true);
  }
}

void EG4Bms::publish_device_unavailable_() {
  this->publish_state_(this->online_status_binary_sensor_, false);
}

}  // namespace eg4_bms
}  // namespace esphome
