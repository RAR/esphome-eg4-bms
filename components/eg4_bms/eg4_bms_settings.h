#pragma once

// Parameter block helpers shared by the component and the host tests
// (tests/rs232_service_test.cpp). Plain C++, no ESPHome dependencies.
//
// The parameter block is FC03 0x2D x91 (0x2D-0x87). Its registers are only
// served on the pack's RS232 service port (115200 8N1, DIP ID 1, no other pack
// on the paralleling ports); on the EG4-LL paralleling bus the Z02 firmware
// answers the same request with the threshold registers all zero.

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace esphome {
namespace eg4_bms {

static const uint16_t REG_SETTINGS = 0x002D;       // First register of the parameter block
static const uint16_t SETTINGS_REGISTERS = 0x5B;   // 0x2D-0x87, as the BMS tool reads it
static const uint16_t REG_SETTINGS_LAST = REG_SETTINGS + SETTINGS_REGISTERS - 1;
static const uint16_t REG_BMS_TIME = 0x002D;       // Packed clock, ULONG 0x2D-0x2E (high word first)

// Threshold registers of the block: everything except the clock (0x2D-0x2E)
// and the model/firmware/serial text (0x69-0x7F), which every port serves.
inline bool is_threshold_register(uint16_t reg) {
  return (reg >= 0x002F && reg <= 0x0068) || (reg >= 0x0080 && reg <= REG_SETTINGS_LAST);
}

// True if the block covers threshold registers and all of them are zero, i.e.
// the port does not serve the parameter block (EG4-LL paralleling bus).
inline bool thresholds_blank(uint16_t start_register, const uint8_t *data, size_t len) {
  bool covered = false;
  for (size_t i = 0; i + 1 < len; i += 2) {
    const uint16_t reg = start_register + i / 2;
    if (!is_threshold_register(reg))
      continue;
    covered = true;
    if (data[i] != 0 || data[i + 1] != 0)
      return false;
  }
  return covered;
}

// BMS clock: packs a calendar time; 0 if a field is out of range.
inline uint32_t pack_bms_time(unsigned year, unsigned month, unsigned day, unsigned hour, unsigned minute,
                              unsigned second) {
  if (year < 2000 || year > 2063 || month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 ||
      second > 59)
    return 0;
  return (uint32_t(year - 2000) << 26) | (uint32_t(month) << 22) | (uint32_t(day) << 17) | (uint32_t(hour) << 12) |
         (uint32_t(minute) << 6) | uint32_t(second);
}

// BMS clock: year-2000 << 26 | month << 22 | day << 17 | hour << 12 | min << 6 | sec.
// Not Unix time. Writes "YYYY-MM-DD HH:MM:SS"; false if a field is out of range.
inline bool format_bms_time(uint32_t packed, char *buf, size_t len) {
  const unsigned year = 2000 + (packed >> 26);
  const unsigned month = (packed >> 22) & 0x0F;
  const unsigned day = (packed >> 17) & 0x1F;
  const unsigned hour = (packed >> 12) & 0x1F;
  const unsigned minute = (packed >> 6) & 0x3F;
  const unsigned second = packed & 0x3F;
  if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59)
    return false;
  return std::snprintf(buf, len, "%04u-%02u-%02u %02u:%02u:%02u", year, month, day, hour, minute, second) > 0;
}

}  // namespace eg4_bms
}  // namespace esphome
