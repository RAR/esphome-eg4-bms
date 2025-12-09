# EG4 BMS Register Map

## Known Register Blocks

### Real-time Data Registers (0x0000 - 0x0026)
Currently implemented - see `eg4_bms.cpp` for details.

### Configuration/Threshold Registers (0x002D - 0x0087)

Based on analysis of Modbus response from request starting at 0x002D (91 registers total).

#### Real-Time Clock (RTC) / Timestamp

Format: YYYY-MM-DD HH:MM:SS

- **0x002D (45)**: Year + Month
  - High byte: Year = value + 1922 (e.g., 103 + 1922 = 2025)
  - Low byte: Month = value - 5 (e.g., 17 - 5 = 12)
  
- **0x002E (46)**: Hour + Minute
  - High byte: Hour in BCD format (e.g., 0x17 BCD = 17)
  - Low byte: Minute = value - 110 (e.g., 140 - 110 = 30)
  
- **0x002F (47)**: Unused + Day
  - High byte: 0 (unused)
  - Low byte: Day (e.g., 8)
  
- **0x0030 (48)**: Second + Unknown
  - High byte: Second = value - 5 (e.g., 17 - 5 = 12)
  - Low byte: Unknown (possibly subseconds or status)

#### Balancing Configuration
- **0x0038 (56)**: Balance Voltage - 3400 mV (3.400V)
- **0x0039 (57)**: Balance Diff - 40 mV (0.040V) [Note: GUI shows 20?]
- **0x003A (58)**: Low Capacity Warning - 10 (Ah or %)

#### Cell Under-Voltage Thresholds (mV units, 1mV resolution)
- **0x003D (61)**: Cell UV Warning - 2500 mV (2.500V)
- **0x003E (62)**: Cell UV Protection - 2300 mV (2.300V)  
- **0x003F (63)**: Cell UV Release - 3100 mV (3.100V)

#### Pack Under-Voltage Thresholds (0.1V units, 100mV resolution)
- **0x0040 (64)**: Pack UV Warning - 4650 (46.50V)
- **0x0041 (65)**: Pack UV Protection - 4480 (44.80V)
- **0x0042 (66)**: Pack UV Release - 4800 (48.00V)

#### Cell Over-Voltage Thresholds (mV units, 1mV resolution)
- **0x0043 (67)**: Cell OV Warning - 3700 mV (3.700V)
- **0x0044 (68)**: Cell OV Protection - 3800 mV (3.800V)
- **0x0045 (69)**: Cell OV Release - 3450 mV (3.450V)

#### Pack Over-Voltage Thresholds (0.1V units, 100mV resolution)
- **0x0046 (70)**: Pack OV Warning - 5920 (59.20V)
- **0x0047 (71)**: Pack OV Protection - 6000 (60.00V)
- **0x0048 (72)**: Pack OV Release - 5520 (55.20V)

#### Charge Over-Current Protection (0.01A units for current)
- **0x0050 (80)**: Charge OC1 Protect - 20500 (205.00A)
- **0x0055 (85)**: Charge OC1 Delay - 10 (seconds)
- **0x0053 (83)**: Charge OC2 Protect - 22500 (225.00A)
- **0x0056 (86)**: Charge OC2 Delay - 3 (seconds)
- **0x004E (78)**: Charge OC Release Delay - 180 (seconds)
- **0x004C (76)**: Charge OC Times - 5 (retry count or trigger count)

#### Discharge Over-Current Protection (0.01A units for current)
- **0x0051 (81)**: Discharge OC1 Protect - 20500 (205.00A)
- **0x0057 (87)**: Discharge OC1 Delay - 10 (seconds)
- **0x0052 (82)**: Discharge OC2 Protect - 30000 (300.00A)
- **0x0058 (88)**: Discharge OC2 Delay - 3 (seconds)
- **0x004F (79)**: Discharge OC Release Delay - 60 (seconds)
- **0x004D (77)**: Discharge OC Times - 5 (retry count or trigger count)
- **0x0054 (84)**: Load Short Current - 30000 (300.00A)

### Heating Control

Temperature thresholds use +50 offset (subtract 50 to get °C).

- **0x0080 (128)**: Heating ON Temperature - 50 (50-50 = 0°C) - Turn heating on when temp drops to 0°C
- **0x0081 (129)**: Heating OFF Temperature - 53 (53-50 = 3°C) - Turn heating off when temp rises to 3°C

---

#### Temperature Protection Thresholds
Temperature values stored as single bytes in low byte of each register.
**All temperature values use +50 offset** (subtract 50 to get actual temperature in °C).

##### Charge Undertemp Protection
- **0x005A (90)**: Charge UT Warning - 48 (48-50 = -2°C)
- **0x005B (91)**: Charge UT Protection - 45 (45-50 = -5°C)
- **0x005C (92)**: Charge UT Release - 50 (50-50 = 0°C)

##### Charge Overtemp Protection
- **0x005D (93)**: Charge OT Warning - 115 (115-50 = 65°C)
- **0x005E (94)**: Charge OT Protection - 120 (120-50 = 70°C)
- **0x005F (95)**: Charge OT Release - 110 (110-50 = 60°C)

##### Discharge Undertemp Protection
- **0x0060 (96)**: Discharge UT Warning - 35 (35-50 = -15°C)
- **0x0061 (97)**: Discharge UT Protection - 30 (30-50 = -20°C)
- **0x0062 (98)**: Discharge UT Release - 40 (40-50 = -10°C)

##### Discharge Overtemp Protection
- **0x0063 (99)**: Discharge OT Warning - 120 (120-50 = 70°C)
- **0x0064 (100)**: Discharge OT Protection - 125 (125-50 = 75°C)
- **0x0065 (101)**: Discharge OT Release - 115 (115-50 = 65°C)

##### PCB Overtemp Protection
- **0x0066 (102)**: PCB OT Warning - 150 (150-50 = 100°C)
- **0x0067 (103)**: PCB OT Protection - 160 (160-50 = 110°C)
- **0x0068 (104)**: PCB OT Release - 130 (130-50 = 80°C)

##### Ambient Undertemp Protection
- **0x0082 (130)**: Ambient UT Warning - 30 (30-50 = -20°C)
- **0x0083 (131)**: Ambient UT Protection - 25 (25-50 = -25°C)
- **0x0084 (132)**: Ambient UT Release - 35 (35-50 = -15°C)

##### Ambient Overtemp Protection
- **0x0085 (133)**: Ambient OT Warning - 120 (120-50 = 70°C)
- **0x0086 (134)**: Ambient OT Protection - 130 (130-50 = 80°C)
- **0x0087 (135)**: Ambient OT Release - 110 (110-50 = 60°C)

#### Other Current Settings (To Be Mapped)
- **0x0049 (73)**: 555 (55.5A or 5.55A?) - unknown
- **0x004A (74)**: 500 (50.0A or 5.00A?) - unknown

#### Device Identification (Read-only)
- **0x0069-0x0073** (105-115): Model Name (22 bytes ASCII) - "ZTR-16S200A-10KW-STD2"
- **0x0075-0x0077** (117-119): Firmware Version (6 bytes ASCII) - "S02T11"
- **0x0078-0x007F** (120-127): Serial Number (16 bytes ASCII) - "2309876543210987"

## Sample Request/Response

### Reading Configuration

**Request:** `01 03 00 2D 00 5B 94 38`
- Device: 0x01
- Function: 0x03 (Read Holding Registers)
- Start: 0x002D (45)
- Count: 0x005B (91 registers)
- CRC: 0x94 0x38

**Response:** Full 182-byte payload with configuration and identification data.

### Writing Configuration

**CAUTION:** Writing incorrect values can damage your battery system. Always validate values before writing!

#### Write Single Register (Function 0x06)

Format: `[Device] [0x06] [Reg High] [Reg Low] [Value High] [Value Low] [CRC Low] [CRC High]`

**Example:** Set balance differential to 20mV at register 0x0039
- Command: `01 06 00 39 00 14 59 C8`
  - Device: 0x01
  - Function: 0x06 (Write Single Register)
  - Register: 0x0039 (Balance Diff)
  - Value: 0x0014 (20 mV)
  - CRC: 0x59 0xC8
- Response: Echo of command if successful

#### Write Multiple Registers (Function 0x10)

Format: `[Device] [0x10] [Start High] [Start Low] [Count High] [Count Low] [Byte Count] [Data...] [CRC Low] [CRC High]`

**Example:** Set RTC to 2025-12-08 18:00:00
- Command: `01 10 00 2D 00 04 08 67 11 18 6E 00 08 05 00 F4 5A`
  - Device: 0x01
  - Function: 0x10 (Write Multiple Registers)
  - Start Register: 0x002D
  - Register Count: 4 (8 bytes)
  - Data breakdown:
    - 0x67: Year (103 + 1922 = 2025)
    - 0x11: Month (17 - 5 = 12)
    - 0x18: Hour BCD (18)
    - 0x6E: Minute (110 + 0 = 110, minute = 0)
    - 0x00: Unused
    - 0x08: Day (8)
    - 0x05: Second (5 - 5 = 0)
    - 0x00: Unknown
  - CRC: 0xF4 0x5A
- Response: `01 10 00 2D 00 04 [CRC]` if successful

#### Important Notes

1. **Read-Only Registers:** Device identification (0x0069-0x007F) cannot be written
2. **Validation:** Always read back after writing to confirm the change
3. **Safety Limits:** Ensure voltage/current/temperature values are within safe operating ranges
4. **BMS Mode:** Some writes may require specific BMS operating modes
5. **CRC Calculation:** Use Modbus CRC-16 (polynomial 0xA001, initial value 0xFFFF)

## TODO: Implementation Ideas

1. Add optional configuration sensors to read these threshold values
2. Allow writing to configuration registers (function 0x10) if supported
3. Map remaining unknown registers through testing
4. Create settings/configuration component for adjusting thresholds
