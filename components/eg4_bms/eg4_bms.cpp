#include "eg4_bms.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include <algorithm>
#include <cmath>

namespace esphome {
namespace eg4_bms {

static const char *const TAG = "eg4_bms";

static const uint8_t FUNCTION_READ_HOLDING = 0x03;
static const uint8_t FUNCTION_WRITE_MULTIPLE = 0x10;

// Register addresses - official EG4-LL Modbus protocol V01.06 (docs/), checked
// against a capture of the link-port master's 0x00 x45 poll.
static const uint16_t REG_VOLTAGE = 0x0000;          // Total voltage (10mV)
static const uint16_t REG_CURRENT = 0x0001;          // Current (10mA, signed, + = charge)
static const uint16_t REG_CELL_VOLTAGE = 0x0002;     // Cell voltages 0x02-0x11 (mV, 16 cells)
static const uint16_t REG_CELL_VOLTAGE_LAST = 0x0011;
static const uint16_t REG_TEMP_PCB = 0x0012;         // PCB/MOSFET temperature (°C, signed)
static const uint16_t REG_TEMP_AVG = 0x0013;         // Average temperature (°C, signed)
static const uint16_t REG_TEMP_MAX = 0x0014;         // Max temperature (°C, signed)
static const uint16_t REG_CAP_REMAINING = 0x0015;    // Remaining capacity (Ah)
static const uint16_t REG_MAX_CHARGE_CURRENT = 0x0016;  // Max charging current (A)
static const uint16_t REG_SOH = 0x0017;              // State of health (0-100%)
static const uint16_t REG_SOC = 0x0018;              // State of charge (0-100%)
static const uint16_t REG_STATUS = 0x0019;           // Status (low byte bits, bit 15 = heater)
static const uint16_t REG_WARNING = 0x001A;          // Warnings
static const uint16_t REG_PROTECTION = 0x001B;       // Protection status
static const uint16_t REG_ERROR = 0x001C;            // Error code
static const uint16_t REG_CYCLE_COUNT = 0x001D;      // Cycle count, ULONG 0x1D-0x1E (high word first)
static const uint16_t REG_FULL_CAPACITY = 0x001F;    // Full capacity, ULONG 0x1F-0x20 (mA·s)
static const uint16_t REG_TEMPS = 0x0021;            // Temperature sensors 1-6, 0x21-0x23 (1 byte each, signed)
static const uint16_t REG_CELL_COUNT = 0x0024;       // Cell count
static const uint16_t REG_DESIGN_CAPACITY = 0x0025;  // Design capacity (0.1 Ah)
static const uint16_t REG_BALANCING = 0x0026;        // Cell balancing bitmap (not exposed)
static const uint16_t REG_REALTIME_LAST = REG_BALANCING;
static const uint16_t REG_MODEL = 0x0069;            // Model (22 bytes)
static const uint16_t REG_FW_VERSION = 0x0075;       // Firmware version (6 bytes)
static const uint16_t REG_SERIAL_NO = 0x0078;        // Serial number (16 bytes)

static const uint16_t MODEL_REGISTERS = 11;
static const uint16_t FW_VERSION_REGISTERS = 3;
static const uint16_t SERIAL_NO_REGISTERS = 8;

// Some EG4 LL-V2 units intermittently answer with a complete, CRC-valid
// function-03 response whose entire register data area is zero (issue #7).
// Publishing those zeroes produces transient SOC/voltage/capacity collapses
// downstream, so a block is only accepted once it looks physically plausible.
static bool is_all_zero(const uint8_t *payload, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (payload[i] != 0)
      return false;
  }
  return true;
}

// True if [start, start + count) overlaps [first, last].
static bool overlaps(uint16_t start, size_t count, uint16_t first, uint16_t last) {
  return count > 0 && start <= last && (uint32_t) start + count > first;
}

void EG4Bms::dump_config() {
  ESP_LOGCONFIG(TAG, "EG4 BMS:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);
  ESP_LOGCONFIG(TAG, "  Max no response count: %u", this->max_no_response_count_);
  if (this->is_passive()) {
    ESP_LOGCONFIG(TAG, "  Passive: offline after %u ms without a valid block on the bus",
                  (unsigned) (this->max_no_response_count_ * this->get_update_interval()));
  }
  LOG_BINARY_SENSOR("  ", "Online Status", this->online_status_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Heating", this->heating_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Charging", this->charging_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Discharging", this->discharging_binary_sensor_);
  LOG_SENSOR("  ", "Total Voltage", this->total_voltage_sensor_);
  LOG_SENSOR("  ", "Current", this->current_sensor_);
  LOG_SENSOR("  ", "Power", this->power_sensor_);
  LOG_SENSOR("  ", "State of Charge", this->state_of_charge_sensor_);
  LOG_TEXT_SENSOR("  ", "Status", this->status_text_sensor_);
  if (this->read_settings_) {
    ESP_LOGCONFIG(TAG, "  Parameter block: read every 30 updates (%u setting sensors)",
                  (unsigned) this->setting_sensors_.size());
  }
}

float EG4Bms::get_setup_priority() const { return setup_priority::DATA; }

void EG4Bms::update() {
  if (this->update_counter_ == 0) {
    this->publish_state_(this->rejected_frame_count_sensor_, 0.0f);
  }

  // A staged write whose parameter block never arrived is refused here rather
  // than left showing the requested value.
  if (!this->staged_writes_.empty() && millis() - this->staged_writes_.front().staged_ms > STAGED_WRITE_MAX_AGE_MS) {
    for (const auto &w : this->staged_writes_) {
      ESP_LOGW(TAG, "BMS 0x%02X: write of %.3f to 0x%04X refused: the parameter block could not be read in time",
               this->address_, w.value, w.reg);
      this->republish_setting_(w.reg);
    }
    this->staged_writes_.clear();
  }

  if (this->is_passive()) {
    this->update_passive_();
    this->update_counter_++;
    return;
  }

  if (this->no_response_count_ >= this->max_no_response_count_) {
    this->publish_device_unavailable_();
    ESP_LOGW(TAG, "No response from BMS (address 0x%02X)", this->address_);
  }

  this->track_online_status_();
  // Saturate rather than wrap - an 8-bit counter left to overflow would flap a
  // long-dead BMS back to "online" every 256 polls.
  if (this->no_response_count_ < 255) {
    this->no_response_count_++;
  }

  // Always cycle through the two main register blocks
  switch (this->request_step_) {
    case 0:
      // Request main data block (voltage, current, cell voltages)
      this->send(FUNCTION_READ_HOLDING, REG_VOLTAGE, 0x12);  // Read 18 registers (0x00-0x11)
      break;
    case 1:
      // Request temps, capacities, SOC, SOH, status, warnings, errors
      this->send(FUNCTION_READ_HOLDING, REG_TEMP_PCB, 0x15);  // Read 21 registers (0x12-0x26)
      break;
  }

  this->request_step_ = (this->request_step_ + 1) % 2;

  // Request text sensor data occasionally (every 30 updates)
  if (this->update_counter_ < 3 || (this->update_counter_ % 30) == 0) {
    // Cycle through text sensor requests
    switch (this->text_request_step_) {
      case 0:
        this->send(FUNCTION_READ_HOLDING, REG_MODEL, MODEL_REGISTERS);
        break;
      case 1:
        this->send(FUNCTION_READ_HOLDING, REG_FW_VERSION, FW_VERSION_REGISTERS);
        break;
      case 2:
        this->send(FUNCTION_READ_HOLDING, REG_SERIAL_NO, SERIAL_NO_REGISTERS);
        break;
    }
    this->text_request_step_ = (this->text_request_step_ + 1) % 3;
  }

  // The parameter block changes only when someone writes it, so read it at
  // startup and then as rarely as the text blocks.
  if (this->read_settings_ && (this->update_counter_ % 30) == 0) {
    this->send(FUNCTION_READ_HOLDING, REG_SETTINGS, SETTINGS_REGISTERS);
  }

  this->update_counter_++;
}

// Passive mode sends nothing. The link master polls each pack several times a
// second, so blocks are only stored as they arrive and published here, once per
// update_interval. The BMS is online while a valid block has been seen within
// max_no_response_count x update_interval.
void EG4Bms::update_passive_() {
  const uint64_t timeout_ms = (uint64_t) this->max_no_response_count_ * this->get_update_interval();
  const bool online = this->seen_ && (uint64_t) (millis() - this->last_seen_ms_) <= timeout_ms;

  if (online) {
    this->publish_state_(this->online_status_binary_sensor_, true);
    this->publish_pending_();
  } else {
    this->publish_device_unavailable_();
    ESP_LOGW(TAG, "No traffic seen for BMS (address 0x%02X)", this->address_);
  }
}

void EG4Bms::on_modbus_block(uint8_t address, uint8_t function, uint16_t start_register, const uint8_t *data,
                             size_t len) {
  // Check if this response is for this device
  if (address != this->address_) {
    return;  // Not for this device
  }

  // Check for error response
  if ((function & 0x80) != 0) {
    ESP_LOGW(TAG, "Modbus error response: 0x%02X", len > 0 ? data[0] : 0);
    if ((function & 0x7F) == FUNCTION_WRITE_MULTIPLE) {
      for (auto it = this->expected_writes_.begin(); it != this->expected_writes_.end(); ++it) {
        if (!it->acked && it->reg == start_register) {
          ESP_LOGE(TAG, "BMS 0x%02X: write to 0x%04X REJECTED by the BMS (exception 0x%02X)", this->address_,
                   it->reg, len > 0 ? data[0] : 0);
          this->expected_writes_.erase(it);
          break;
        }
      }
    }
    // The BMS did answer, it just refused the request - it is still online.
    this->on_device_answered_();
    return;
  }

  if (function == FUNCTION_WRITE_MULTIPLE) {
    // The hub only dispatches an acknowledgement that matches the write sent.
    ESP_LOGI(TAG, "BMS 0x%02X: write to 0x%04X acknowledged", this->address_, start_register);
    for (auto &w : this->expected_writes_) {
      if (!w.acked && w.reg == start_register) {
        w.acked = true;
        break;
      }
    }
    return;
  }

  if (function != FUNCTION_READ_HOLDING) {
    ESP_LOGW(TAG, "Unexpected function code: 0x%02X", function);
    return;
  }

  if (start_register == eg4_modbus::MODBUS_START_REGISTER_UNKNOWN || len < 2 || (len % 2) != 0) {
    ESP_LOGW(TAG, "BMS 0x%02X: block of %u bytes with unknown start register ignored", this->address_,
             (unsigned) len);
    return;
  }

  const size_t count = len / 2;
  ESP_LOGD(TAG, "Received block: start=0x%04X registers=%u", start_register, (unsigned) count);

  if (!overlaps(start_register, count, REG_VOLTAGE, REG_REALTIME_LAST) &&
      !overlaps(start_register, count, REG_SETTINGS, REG_SETTINGS_LAST)) {
    ESP_LOGW(TAG, "BMS 0x%02X: unrecognised block 0x%04X x%u", this->address_, start_register, (unsigned) count);
    return;
  }

  if (!this->validate_block_(start_register, data, len)) {
    return;
  }

  // The EG4-LL paralleling bus answers the parameter block with every
  // threshold zero (and a glitching BMS might too); storing or publishing
  // those would report 0 V protection limits and defeat the ordering checks.
  // Keep the clock and text registers, drop the thresholds.
  if (thresholds_blank(start_register, data, len)) {
    for (size_t i = 0; i < count; i++) {
      if (!is_threshold_register(start_register + i)) {
        // Single-register store keeps the clock and text.
        this->store_block_(start_register + i, data + i * 2, 2);
      }
    }
    if (this->settings_seen_)
      this->forget_settings_("parameter block read back blank");
    if (!this->blank_settings_logged_) {
      ESP_LOGW(TAG,
               "BMS 0x%02X: parameter block reads all zero - this port does not serve the settings "
               "(use the RS232 service port)",
               this->address_);
      this->blank_settings_logged_ = true;
    }
  } else {
    this->store_block_(start_register, data, len);
    if (overlaps(start_register, count, REG_SETTINGS, REG_SETTINGS_LAST)) {
      this->verify_writes_(start_register, count);
      if (start_register <= 0x002F && start_register + count > 0x0087) {
        this->settings_seen_ = true;  // every threshold register now holds the BMS's value
        this->apply_staged_writes_();
      }
    }
  }

  if (!this->is_passive()) {
    this->publish_pending_();
  }

  // Only the measurement registers prove the BMS is measuring correctly. The
  // text blocks are served from static strings and keep answering even while a
  // unit is returning zeroed measurements, so they must not mark it online.
  if (overlaps(start_register, count, REG_VOLTAGE, REG_REALTIME_LAST)) {
    this->on_device_answered_();
  }
}

void EG4Bms::on_device_answered_() {
  if (this->is_passive()) {
    this->seen_ = true;
    this->last_seen_ms_ = millis();
    this->publish_state_(this->online_status_binary_sensor_, true);
  } else {
    this->reset_online_status_tracker_();
  }
}

bool EG4Bms::validate_block_(uint16_t start_register, const uint8_t *data, size_t len) {
  const size_t count = len / 2;
  if (!overlaps(start_register, count, REG_VOLTAGE, REG_REALTIME_LAST)) {
    return true;  // Text blocks have no plausibility rule.
  }

  auto has = [&](uint16_t reg) -> bool { return reg >= start_register && reg < start_register + count; };
  auto reg16 = [&](uint16_t reg) -> uint16_t {
    const size_t i = (size_t) (reg - start_register) * 2;
    return (uint16_t(data[i]) << 8) | uint16_t(data[i + 1]);
  };
  const bool all_zero = is_all_zero(data, len);

  // A live BMS never reports a 0.00 V pack; reject the whole block atomically
  // so the last good values are retained.
  if (has(REG_VOLTAGE) && (all_zero || reg16(REG_VOLTAGE) == 0)) {
    this->reject_block_(data, len, "electrical/cell block with zero pack voltage");
    return false;
  }

  // SOC 0 and max charge current 0 are legitimate, but remaining capacity,
  // SOH and design capacity cannot all read zero on a working pack.
  if (has(REG_CAP_REMAINING) && has(REG_SOH) &&
      (all_zero || (reg16(REG_CAP_REMAINING) == 0 && reg16(REG_SOH) == 0 &&
                    (!has(REG_DESIGN_CAPACITY) || reg16(REG_DESIGN_CAPACITY) == 0)))) {
    this->reject_block_(data, len, "capacity/SOC block with zero capacity and SOH");
    return false;
  }

  if (all_zero) {
    this->reject_block_(data, len, "all-zero measurement block");
    return false;
  }

  return true;
}

void EG4Bms::store_block_(uint16_t start_register, const uint8_t *data, size_t len) {
  const size_t count = len / 2;
  for (size_t i = 0; i < count; i++) {
    const uint32_t reg = (uint32_t) start_register + i;
    if (reg >= REGISTER_IMAGE_SIZE)
      break;
    this->registers_[reg] = (uint16_t(data[i * 2]) << 8) | uint16_t(data[i * 2 + 1]);
    this->pending_.set(reg);
  }
}

void EG4Bms::publish_pending_() {
  if (this->pending_.none())
    return;

  auto has = [&](uint16_t first, uint16_t last) -> bool {
    for (uint16_t r = first; r <= last; r++) {
      if (!this->pending_.test(r))
        return false;
    }
    return true;
  };
  auto reg = [&](uint16_t r) -> uint16_t { return this->registers_[r]; };
  auto reg32 = [&](uint16_t r) -> uint32_t { return (uint32_t(reg(r)) << 16) | reg(r + 1); };
  auto text = [&](uint16_t first, uint16_t count) -> std::string {
    uint8_t buf[2 * 11];
    for (uint16_t i = 0; i < count; i++) {
      buf[i * 2] = reg(first + i) >> 8;
      buf[i * 2 + 1] = reg(first + i) & 0xFF;
    }
    return this->extract_ascii_string_(buf, count * 2);
  };

  // Total voltage (register 0x0000) - 10mV units
  if (has(REG_VOLTAGE, REG_VOLTAGE)) {
    this->publish_state_(this->total_voltage_sensor_, reg(REG_VOLTAGE) * 0.01f);
  }

  // Current (register 0x0001) - 10mA units, signed
  if (has(REG_CURRENT, REG_CURRENT)) {
    this->publish_state_(this->current_sensor_, (int16_t) reg(REG_CURRENT) * 0.01f);
  }

  if (has(REG_VOLTAGE, REG_CURRENT)) {
    float power = reg(REG_VOLTAGE) * 0.01f * ((int16_t) reg(REG_CURRENT) * 0.01f);
    this->publish_state_(this->power_sensor_, power);
    this->publish_state_(this->charging_power_sensor_, power > 0 ? power : 0);
    this->publish_state_(this->discharging_power_sensor_, power < 0 ? -power : 0);
  }

  // Cell voltages (registers 0x0002-0x0011) - mV units
  if (has(REG_CELL_VOLTAGE, REG_CELL_VOLTAGE_LAST)) {
    float min_cell_voltage = 10.0f;
    float max_cell_voltage = 0.0f;
    float sum_cell_voltage = 0.0f;
    uint8_t valid_cells = 0;
    uint8_t min_cell_number = 0;
    uint8_t max_cell_number = 0;

    for (uint8_t i = 0; i < 16; i++) {
      uint16_t cell_mv = reg(REG_CELL_VOLTAGE + i);
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
      // Until the cell count register (0x0024) has been read, fall back to
      // counting the cells that report a voltage.
      if (!this->cell_count_register_seen_) {
        this->publish_state_(this->cell_count_sensor_, (float) valid_cells);
      }
      this->publish_state_(this->min_cell_voltage_sensor_, min_cell_voltage);
      this->publish_state_(this->max_cell_voltage_sensor_, max_cell_voltage);
      this->publish_state_(this->delta_cell_voltage_sensor_, max_cell_voltage - min_cell_voltage);
      this->publish_state_(this->cell_average_voltage_sensor_, sum_cell_voltage / valid_cells);
      this->publish_state_(this->min_voltage_cell_sensor_, (float) min_cell_number);
      this->publish_state_(this->max_voltage_cell_sensor_, (float) max_cell_number);
    }
  }

  // Temperatures (registers 0x0012-0x0014) - °C, signed
  if (has(REG_TEMP_PCB, REG_TEMP_PCB)) {
    this->publish_state_(this->pcb_temperature_sensor_, (float) (int16_t) reg(REG_TEMP_PCB));
  }
  if (has(REG_TEMP_AVG, REG_TEMP_AVG)) {
    this->publish_state_(this->avg_temperature_sensor_, (float) (int16_t) reg(REG_TEMP_AVG));
  }
  if (has(REG_TEMP_MAX, REG_TEMP_MAX)) {
    this->publish_state_(this->max_temperature_sensor_, (float) (int16_t) reg(REG_TEMP_MAX));
  }

  // Remaining capacity (register 0x0015) - Ah
  if (has(REG_CAP_REMAINING, REG_CAP_REMAINING)) {
    this->publish_state_(this->remaining_capacity_sensor_, (float) reg(REG_CAP_REMAINING));
  }

  // Max charge current (register 0x0016) - A
  if (has(REG_MAX_CHARGE_CURRENT, REG_MAX_CHARGE_CURRENT)) {
    this->publish_state_(this->max_charge_current_sensor_, (float) reg(REG_MAX_CHARGE_CURRENT));
  }

  // SOH / SOC (registers 0x0017 / 0x0018) - percentage
  if (has(REG_SOH, REG_SOH)) {
    this->publish_state_(this->state_of_health_sensor_, (float) reg(REG_SOH));
  }
  if (has(REG_SOC, REG_SOC)) {
    this->publish_state_(this->state_of_charge_sensor_, (float) reg(REG_SOC));
  }

  // Status (register 0x0019)
  if (has(REG_STATUS, REG_STATUS)) {
    uint16_t status = reg(REG_STATUS);
    this->publish_state_(this->status_text_sensor_, this->decode_status_(status));
    this->publish_state_(this->heating_binary_sensor_, (status & 0x8000) != 0);
  }

  // Warnings (register 0x001A)
  if (has(REG_WARNING, REG_WARNING)) {
    uint16_t warnings = reg(REG_WARNING);
    this->publish_state_(this->warnings_text_sensor_, this->decode_warnings_(warnings));
    this->publish_state_(this->warnings_bitmask_sensor_, (float) warnings);
  }

  // Protection (register 0x001B)
  if (has(REG_PROTECTION, REG_PROTECTION)) {
    uint16_t protection = reg(REG_PROTECTION);
    this->publish_state_(this->protection_text_sensor_, this->decode_protection_(protection));
    this->publish_state_(this->protection_bitmask_sensor_, (float) protection);

    // Charging/discharging allowed based on protection bits
    // Charging blocked by: Cell OV (0x0002), Pack OV (0x0001), Charge OC (0x0010),
    //                      Charge OT (0x0100), Charge UT (0x0400)
    // Discharging blocked by: Cell UV (0x0008), Pack UV (0x0004), Discharge OC (0x0020),
    //                         Discharge OT (0x0200), Discharge UT (0x0800), Discharge SC (0x2000)
    bool charging_allowed = !(protection & (0x0001 | 0x0002 | 0x0010 | 0x0100 | 0x0400));
    bool discharging_allowed = !(protection & (0x0004 | 0x0008 | 0x0020 | 0x0200 | 0x0800 | 0x2000));
    this->publish_state_(this->charging_binary_sensor_, charging_allowed);
    this->publish_state_(this->discharging_binary_sensor_, discharging_allowed);
  }

  // Error (register 0x001C)
  if (has(REG_ERROR, REG_ERROR)) {
    uint16_t error = reg(REG_ERROR);
    this->publish_state_(this->error_text_sensor_, this->decode_error_(error));
    this->publish_state_(this->errors_bitmask_sensor_, (float) error);
  }

  // Cycle count (registers 0x001D-0x001E) - ULONG, high word first
  if (has(REG_CYCLE_COUNT, REG_CYCLE_COUNT + 1)) {
    this->publish_state_(this->cycle_count_sensor_, (float) reg32(REG_CYCLE_COUNT));
  }

  // Full capacity (registers 0x001F-0x0020) - ULONG in mA·s; /3600 = mAh
  if (has(REG_FULL_CAPACITY, REG_FULL_CAPACITY + 1)) {
    this->publish_state_(this->full_capacity_sensor_, reg32(REG_FULL_CAPACITY) / 3600000.0f);
  }

  // Temperature sensors 1-6 (registers 0x0021-0x0023) - one signed byte each,
  // high byte first
  for (uint8_t i = 0; i < 3; i++) {
    if (has(REG_TEMPS + i, REG_TEMPS + i)) {
      uint16_t value = reg(REG_TEMPS + i);
      this->publish_state_(this->temperatures_[i * 2].temperature_sensor_, (float) (int8_t) (value >> 8));
      this->publish_state_(this->temperatures_[i * 2 + 1].temperature_sensor_, (float) (int8_t) (value & 0xFF));
    }
  }

  // Cell count (register 0x0024)
  if (has(REG_CELL_COUNT, REG_CELL_COUNT)) {
    this->cell_count_register_seen_ = true;
    this->publish_state_(this->cell_count_sensor_, (float) reg(REG_CELL_COUNT));
  }

  // Design capacity (register 0x0025) - 0.1 Ah units
  if (has(REG_DESIGN_CAPACITY, REG_DESIGN_CAPACITY)) {
    this->publish_state_(this->designed_capacity_sensor_, reg(REG_DESIGN_CAPACITY) * 0.1f);
  }

  if (has(REG_MODEL, REG_MODEL + MODEL_REGISTERS - 1)) {
    this->publish_state_(this->model_text_sensor_, text(REG_MODEL, MODEL_REGISTERS));
  }
  if (has(REG_FW_VERSION, REG_FW_VERSION + FW_VERSION_REGISTERS - 1)) {
    this->publish_state_(this->firmware_text_sensor_, text(REG_FW_VERSION, FW_VERSION_REGISTERS));
  }
  if (has(REG_SERIAL_NO, REG_SERIAL_NO + SERIAL_NO_REGISTERS - 1)) {
    this->publish_state_(this->serial_number_text_sensor_, text(REG_SERIAL_NO, SERIAL_NO_REGISTERS));
  }

  // BMS clock (registers 0x002D-0x002E) - packed date/time, see format_bms_time()
  if (has(REG_BMS_TIME, REG_BMS_TIME + 1)) {
    char buf[24];
    if (format_bms_time(reg32(REG_BMS_TIME), buf, sizeof(buf))) {
      this->publish_state_(this->bms_time_text_sensor_, std::string(buf));
    }
  }

  // Parameter block thresholds (registers 0x002F-0x0087)
  for (const auto &setting : this->setting_sensors_) {
    if (setting.reg < REGISTER_IMAGE_SIZE && this->pending_.test(setting.reg)) {
      this->publish_state_(setting.sensor, reg(setting.reg) * setting.multiplier + setting.offset);
    }
  }
  for (const auto &setting : this->setting_numbers_) {
    if (setting.reg < REGISTER_IMAGE_SIZE && this->pending_.test(setting.reg)) {
      setting.listener->on_setting_value(reg(setting.reg) * setting.multiplier + setting.offset);
    }
  }

  this->pending_.reset();
}

void EG4Bms::set_writes_enabled(bool enabled) {
  this->writes_enabled_ = enabled;
  ESP_LOGI(TAG, "BMS 0x%02X: settings writes %s", this->address_, enabled ? "ENABLED" : "disabled");
  if (!enabled) {
    if (this->parent_ != nullptr)
      this->parent_->cancel_writes(this->address_);
    // Cancelled writes will never be acknowledged.
    this->expected_writes_.erase(std::remove_if(this->expected_writes_.begin(), this->expected_writes_.end(),
                                                [](const ExpectedWrite &w) { return !w.acked; }),
                                 this->expected_writes_.end());
  }
}

void EG4Bms::forget_settings_(const char *why) {
  if (this->settings_seen_ || !this->expected_writes_.empty() || !this->staged_writes_.empty())
    ESP_LOGW(TAG, "BMS 0x%02X: %s - forgetting the parameter block", this->address_, why);
  this->settings_seen_ = false;
  this->expected_writes_.clear();
  for (const auto &w : this->staged_writes_)
    ESP_LOGW(TAG, "BMS 0x%02X: staged write to 0x%04X dropped", this->address_, w.reg);
  this->staged_writes_.clear();
  if (this->parent_ != nullptr)
    this->parent_->cancel_writes(this->address_);
}

void EG4Bms::expire_writes_() {
  const uint32_t now = millis();
  for (auto it = this->expected_writes_.begin(); it != this->expected_writes_.end();) {
    if (now - it->queued_ms > WRITE_EXPIRY_MS) {
      ESP_LOGE(TAG, "BMS 0x%02X: write to 0x%04X = %u not confirmed within %u s", this->address_, it->reg,
               it->value, (unsigned) (WRITE_EXPIRY_MS / 1000));
      it = this->expected_writes_.erase(it);
    } else {
      ++it;
    }
  }
}

uint16_t EG4Bms::effective_register_(uint16_t reg) const {
  // The newest pending write wins over what the BMS last reported.
  for (auto it = this->expected_writes_.rbegin(); it != this->expected_writes_.rend(); ++it) {
    if (it->reg == reg)
      return it->value;
  }
  return this->registers_[reg];
}

void EG4Bms::republish_setting_(uint16_t reg) {
  if (!this->settings_seen_ || reg >= REGISTER_IMAGE_SIZE)
    return;
  for (const auto &setting : this->setting_numbers_) {
    if (setting.reg == reg)
      setting.listener->on_setting_value(this->registers_[reg] * setting.multiplier + setting.offset);
  }
}

// Writes are read-before-write: write_setting() only checks what it can
// without the BMS (armed, range) and stages the value, then asks for the
// parameter block. apply_staged_writes_() runs when that block arrives, so the
// ordering checks always use what the pack on the port holds right now - the
// ZTR "serial" is the same on every pack, so a swapped pack cannot be told
// apart any other way.
bool EG4Bms::write_setting(uint16_t reg, float value) {
  const SettingNumber *setting = this->find_setting_number_(reg);
  if (setting == nullptr || !is_threshold_register(reg) || reg >= REGISTER_IMAGE_SIZE) {
    ESP_LOGE(TAG, "BMS 0x%02X: 0x%04X is not a writable setting", this->address_, reg);
    return false;
  }

  auto refuse = [&](const char *why) {
    ESP_LOGW(TAG, "BMS 0x%02X: write of %.3f to 0x%04X refused: %s", this->address_, value, reg, why);
    this->republish_setting_(reg);
    return false;
  };

  if (!this->writes_enabled_)
    return refuse("settings writes are disabled (turn on the write-enable switch first)");
  if (this->is_passive())
    return refuse("passive mode never transmits");
  if (std::isnan(value) || value < setting->min_value - setting->multiplier / 2 ||
      value > setting->max_value + setting->multiplier / 2)
    return refuse("value is outside the allowed range");

  const float raw_f = std::round((value - setting->offset) / setting->multiplier);
  if (std::isnan(raw_f) || raw_f < 0.0f || raw_f > 65535.0f)
    return refuse("value does not fit the register");

  this->staged_writes_.push_back({reg, (uint16_t) raw_f, value, millis()});
  ESP_LOGD(TAG, "BMS 0x%02X: 0x%04X = %u staged, reading the parameter block first", this->address_, reg,
           (uint16_t) raw_f);
  this->send(FUNCTION_READ_HOLDING, REG_SETTINGS, SETTINGS_REGISTERS);
  return true;
}

const EG4Bms::SettingNumber *EG4Bms::find_setting_number_(uint16_t reg) const {
  for (const auto &s : this->setting_numbers_) {
    if (s.reg == reg)
      return &s;
  }
  return nullptr;
}

void EG4Bms::apply_staged_writes_() {
  if (this->staged_writes_.empty())
    return;
  std::vector<StagedWrite> staged;
  staged.swap(this->staged_writes_);
  this->expire_writes_();
  const uint32_t now = millis();

  for (const auto &w : staged) {
    const uint16_t reg = w.reg;
    const uint16_t raw = w.raw;
    auto refuse = [&](const char *why) {
      ESP_LOGW(TAG, "BMS 0x%02X: write of %.3f to 0x%04X refused: %s", this->address_, w.value, reg, why);
      this->republish_setting_(reg);
    };

    if (!this->writes_enabled_) {
      refuse("settings writes were disabled before the block was read");
      continue;
    }
    if (now - w.staged_ms > STAGED_WRITE_MAX_AGE_MS) {
      refuse("the parameter block could not be read in time");
      continue;
    }

    bool ordered = true;
    for (const auto &order : this->setting_order_) {
      if (order.lower_reg == reg) {
        const uint16_t upper = this->effective_register_(order.upper_reg);
        if (order.strict ? raw >= upper : raw > upper) {
          ESP_LOGW(TAG, "BMS 0x%02X: 0x%04X must stay %s 0x%04X (raw %u)", this->address_, reg,
                   order.strict ? "<" : "<=", order.upper_reg, upper);
          ordered = false;
        }
      }
      if (order.upper_reg == reg) {
        const uint16_t lower = this->effective_register_(order.lower_reg);
        if (order.strict ? raw <= lower : raw < lower) {
          ESP_LOGW(TAG, "BMS 0x%02X: 0x%04X must stay %s 0x%04X (raw %u)", this->address_, reg,
                   order.strict ? ">" : ">=", order.lower_reg, lower);
          ordered = false;
        }
      }
    }
    if (!ordered) {
      refuse("it would break the warning/protection/release order");
      continue;
    }

    const uint16_t current = this->effective_register_(reg);
    if (raw == current) {
      ESP_LOGI(TAG, "BMS 0x%02X: 0x%04X already %u, nothing to write", this->address_, reg, raw);
      this->republish_setting_(reg);
      continue;
    }

    if (!this->write_registers(reg, &raw, 1)) {
      refuse("the hub could not queue the write");
      continue;
    }
    ESP_LOGI(TAG, "BMS 0x%02X: writing 0x%04X = %u (was %u)", this->address_, reg, raw, current);
    this->expected_writes_.push_back({reg, raw, false, now});
  }

  // Read the block back: the numbers publish what the BMS actually stored.
  if (!this->expected_writes_.empty())
    this->send(FUNCTION_READ_HOLDING, REG_SETTINGS, SETTINGS_REGISTERS);
}

bool EG4Bms::write_clock(const ESPTime &time) {
  if (!this->writes_enabled_) {
    ESP_LOGW(TAG, "BMS 0x%02X: clock not set: settings writes are disabled", this->address_);
    return false;
  }
  if (!time.is_valid()) {
    ESP_LOGW(TAG, "BMS 0x%02X: clock not set: no valid time yet", this->address_);
    return false;
  }
  const uint32_t packed =
      pack_bms_time(time.year, time.month, time.day_of_month, time.hour, time.minute, time.second);
  if (packed == 0) {
    ESP_LOGW(TAG, "BMS 0x%02X: clock not set: time out of range", this->address_);
    return false;
  }
  const uint16_t values[2] = {(uint16_t) (packed >> 16), (uint16_t) (packed & 0xFFFF)};
  if (!this->write_registers(REG_BMS_TIME, values, 2))
    return false;
  ESP_LOGI(TAG, "BMS 0x%02X: setting clock to %04u-%02u-%02u %02u:%02u:%02u", this->address_, time.year,
           time.month, time.day_of_month, time.hour, time.minute, time.second);
  this->send(FUNCTION_READ_HOLDING, REG_SETTINGS, SETTINGS_REGISTERS);
  return true;
}

void EG4Bms::verify_writes_(uint16_t start_register, size_t count) {
  for (auto it = this->expected_writes_.begin(); it != this->expected_writes_.end();) {
    // A read answered before the write's acknowledgement still has the old value.
    if (!it->acked || it->reg < start_register || it->reg >= start_register + count) {
      ++it;
      continue;
    }
    const uint16_t now = this->registers_[it->reg];
    if (now == it->value) {
      ESP_LOGI(TAG, "BMS 0x%02X: 0x%04X = %u confirmed", this->address_, it->reg, now);
    } else {
      ESP_LOGE(TAG, "BMS 0x%02X: write to 0x%04X NOT applied: wrote %u, BMS reports %u", this->address_, it->reg,
               it->value, now);
    }
    it = this->expected_writes_.erase(it);
  }
}

void EG4Bms::reject_block_(const uint8_t *data, size_t len, const char *reason) {
  this->rejected_frame_count_++;
  ESP_LOGW(TAG, "BMS 0x%02X: rejected %s (%u rejected so far): %s", this->address_, reason,
           (unsigned) this->rejected_frame_count_, format_hex_pretty(data, len).c_str());
  this->publish_state_(this->rejected_frame_count_sensor_, (float) this->rejected_frame_count_);
}

// Status (register 0x0019): low byte is a bit field (0x01 charging,
// 0x02 discharging, 0x04 protect, 0x08 charging limited), bit 15 = heater on.
std::string EG4Bms::decode_status_(uint16_t status) {
  std::string result;

  if (status & 0x8000) result += "Heating+";

  const uint8_t state = status & 0x00FF;
  if (state == 0) {
    result += "Standby";
    return result;
  }

  std::string states;
  if (state & 0x01) states += "Charging+";
  if (state & 0x02) states += "Discharging+";
  if (state & 0x04) states += "Protect+";
  if (state & 0x08) states += "Charging Limited+";
  if (states.empty()) {
    states = "Unknown";
  } else {
    states.pop_back();
  }

  return result + states;
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

std::string EG4Bms::extract_ascii_string_(const uint8_t *data, size_t length) {
  std::string result;
  result.reserve(length);
  
  for (size_t i = 0; i < length; i++) {
    uint8_t c = data[i];
    // Only accept printable ASCII characters (0x20-0x7E)
    if (c >= 0x20 && c <= 0x7E) {
      result += static_cast<char>(c);
    } else if (c == 0x00) {
      // Null terminator - stop here
      break;
    }
    // Skip non-printable characters (including 0xFF)
  }
  
  // Trim trailing spaces
  while (!result.empty() && result.back() == ' ') {
    result.pop_back();
  }
  
  return result;
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

void EG4Bms::reset_online_status_tracker_() {
  this->no_response_count_ = 0;
  this->publish_state_(this->online_status_binary_sensor_, true);
}

void EG4Bms::track_online_status_() {
  if (this->no_response_count_ < this->max_no_response_count_) {
    this->publish_state_(this->online_status_binary_sensor_, true);
  }
}

void EG4Bms::publish_device_unavailable_() {
  this->publish_state_(this->online_status_binary_sensor_, false);
  // The pack may be swapped while it is away; never write against old values.
  if (this->settings_seen_ || !this->expected_writes_.empty() || !this->staged_writes_.empty())
    this->forget_settings_("BMS offline");
}

}  // namespace eg4_bms
}  // namespace esphome
