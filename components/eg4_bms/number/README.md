# EG4 BMS Number Component

This number component allows you to modify writable configuration parameters on the EG4 BMS via Modbus.

## Features

- **53 configurable parameters** covering all BMS protection thresholds
- **Automatic unit conversion** between ESPHome and Modbus representations
- **Temperature offset handling** for temperature-related parameters
- **Modbus write support** using function 0x06 (Write Single Register)
- **Safe value validation** with min/max limits enforced

## Configuration Parameters

### Balancing
- `balance_starting_voltage` - Voltage threshold to start cell balancing
- `balance_voltage_difference` - Maximum voltage difference before balancing
- `low_capacity_warning` - Low capacity warning threshold

### Cell Voltage Protection
- Undervoltage: warning, protection, release thresholds
- Overvoltage: warning, protection, release thresholds

### Pack Voltage Protection
- Undervoltage: warning, protection, release thresholds
- Overvoltage: warning, protection, release thresholds

### Charge/Discharge Current Protection
- Overcurrent 1 & 2: protection and delay values
- Short circuit protection delay

### Temperature Protection
- Charge, discharge, PCB, and ambient temperature thresholds
- Each with warning, protection, and release values

### Heating Control
- Heating start/stop temperatures for various operating modes

## Usage

```yaml
number:
  - platform: eg4_bms
    eg4_bms_id: bms0
    
    balance_starting_voltage:
      name: "Balance starting voltage"
    
    cell_overvoltage_protection:
      name: "Cell OV protection"
    
    charge_overtemp_protection:
      name: "Charge overtemp protection"
```

See `esp32-example-with-config.yaml` for a complete example.

## Safety Warning

⚠️ **These parameters control critical battery protection functions.**

- Always test changes carefully
- Verify values are appropriate for your cells
- Monitor BMS behavior after changes
- Keep backups of original values

## Technical Details

### Register Encoding

**Voltages:**
- Cell voltages: millivolts (mV)
- Pack voltages: 0.1V units

**Currents:**
- All currents: 0.01A units

**Temperatures:**
- Storage: +50 offset
- Reading: subtract 50
- Writing: add 50
- Example: 5°C stored as 55

**Delays:**
- Direct values in seconds

### Automatic Temperature Detection

The component automatically detects temperature parameters by register address:
- Range 1: 0x005A - 0x0068
- Range 2: 0x0080 - 0x0087

Temperature parameters automatically get the +50 offset applied when writing.

### Modbus Communication

Uses Modbus function 0x06 (Write Single Register):
```
Request:  [addr][0x06][reg_h][reg_l][val_h][val_l][crc_l][crc_h]
Response: [addr][0x06][reg_h][reg_l][val_h][val_l][crc_l][crc_h]
```

The response echoes the request to confirm the write.

## Implementation Files

- `__init__.py` - Python schema and register mappings
- `eg4_number.h` - C++ class definition
- `eg4_number.cpp` - Write handling and value conversion

## Future Enhancements

- Read-back validation after writes
- Batch write support (Modbus function 0x10)
- Configuration backup/restore functionality
- Preset protection profiles for different cell chemistries

## See Also

- `REGISTER_MAP.md` - Complete register documentation
- `CONFIGURATION.md` - Detailed implementation notes
- `esp32-example-with-config.yaml` - Full configuration example
