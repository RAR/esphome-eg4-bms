# EG4 BMS Configuration Number Entities

## Overview

This implementation adds writable configuration parameters to the ESPHome EG4 BMS component, allowing you to modify BMS protection thresholds and settings via Home Assistant.

## Implementation Details

### Components Created

1. **number/__init__.py** (803 lines)
   - Python schema for 53 configuration parameters
   - Register address mappings (0x0038-0x0087)
   - Factor-based unit conversion (mV, 0.1V, 0.01A, °C)
   - Min/max/step validation for each parameter

2. **number/eg4_number.h**
   - C++ Eg4Number class definition
   - Inherits from number::Number and Component
   - Stores parent component, register address, and conversion factor

3. **number/eg4_number.cpp**
   - Implementation of control() method for handling writes
   - Automatic temperature offset detection (+50 for registers 0x005A-0x0068, 0x0080-0x0087)
   - Factor-based value conversion for voltages and currents
   - Logging of all write operations

4. **eg4_modbus modifications**
   - Added MODBUS_WRITE_SINGLE_REGISTER (0x06) support
   - Implemented send_write_single() method
   - Added write_register() to EG4ModbusDevice class
   - Proper CRC calculation and flow control handling

5. **eg4_bms.h modifications**
   - Added 53 setter methods for number entities
   - Added 53 member variables to store number entity pointers
   - Included number.h header

## Configuration Parameters

### Balancing (3 parameters)
- **balance_starting_voltage** (0x0038): Voltage threshold to start balancing
- **balance_voltage_difference** (0x0039): Max voltage difference before balancing
- **low_capacity_warning** (0x003A): Low capacity warning threshold

### Cell Voltage Protection (6 parameters)
- **cell_undervoltage_warning/protection/release** (0x003D-0x003F)
- **cell_overvoltage_warning/protection/release** (0x0043-0x0045)

### Pack Voltage Protection (6 parameters)
- **pack_undervoltage_warning/protection/release** (0x0040-0x0042)
- **pack_overvoltage_warning/protection/release** (0x0046-0x0048)

### Charge Overcurrent Protection (5 parameters)
- **charge_overcurrent1_protection** (0x0050): First level current limit
- **charge_overcurrent1_delay** (0x0055): Delay before triggering OC1
- **charge_overcurrent2_protection** (0x0053): Second level current limit
- **charge_overcurrent2_delay** (0x0056): Delay before triggering OC2
- **short_circuit_protection_delay** (0x0051): Short circuit delay

### Discharge Overcurrent Protection (4 parameters)
- **discharge_overcurrent1_protection** (0x004D): First level current limit
- **discharge_overcurrent1_delay** (0x0057): Delay before triggering OC1
- **discharge_overcurrent2_protection** (0x004F): Second level current limit
- **discharge_overcurrent2_delay** (0x0058): Delay before triggering OC2

### Temperature Protection (24 parameters)
Each category has warning/protection/release thresholds:
- **Charge temperature** (0x005A-0x005F): Undertemp and overtemp limits during charging
- **Discharge temperature** (0x0060-0x0065): Undertemp and overtemp limits during discharge
- **PCB temperature** (0x0066-0x0068): PCB undertemp and overtemp limits
- **Ambient temperature** (0x0080-0x0085): Ambient undertemp and overtemp limits

### Heating Control (6 parameters)
- **heating_start_temp/stop_temp** (0x0086-0x0087): General heating thresholds
- **charge_heating_start_temp/stop_temp**: Heating during charge (TBD registers)
- **discharge_heating_start_temp/stop_temp**: Heating during discharge (TBD registers)

## Data Encoding

### Temperature Values
- **Storage**: Single byte with +50 offset
- **Reading**: Subtract 50 from register value
- **Writing**: Add 50 before writing to register
- **Example**: 5°C is stored as 55 (0x37)
- **Register ranges**: 0x005A-0x0068, 0x0080-0x0087

### Voltage Values
- **Cell voltages**: Stored in mV (factor 1000.0)
  - Example: 3.4V = 3400 (0x0D48)
- **Pack voltages**: Stored in 0.1V units (factor 10.0)
  - Example: 54.4V = 544 (0x0220)

### Current Values
- **Storage**: 0.01A units (factor 100.0)
- **Example**: 205A = 20500 (0x5014)

### Delay Values
- **Storage**: Direct values in seconds
- **No conversion needed** (factor 1.0)

## Modbus Communication

### Write Single Register (Function 0x06)
```
Request:  [addr][0x06][reg_h][reg_l][val_h][val_l][crc_l][crc_h]
Response: [addr][0x06][reg_h][reg_l][val_h][val_l][crc_l][crc_h] (echo)
```

### Example: Set balance starting voltage to 3.4V
```
Register: 0x0038
Value: 3400 mV = 0x0D48
Command: 10 06 00 38 0D 48 [CRC]
```

## Usage Example

```yaml
number:
  - platform: eg4_bms
    eg4_bms_id: bms0
    
    # Balancing
    balance_starting_voltage:
      name: "Balance starting voltage"
    
    # Cell protection
    cell_undervoltage_protection:
      name: "Cell UV protection"
    cell_overvoltage_protection:
      name: "Cell OV protection"
    
    # Temperature protection
    charge_overtemp_protection:
      name: "Charge overtemp protection"
```

## Safety Considerations

⚠️ **WARNING**: These parameters control critical battery protection functions.

- **Test changes carefully** before deploying to production systems
- **Verify values** are within safe ranges for your specific cells
- **Read BMS manual** for recommended protection values
- **Monitor the BMS** after making changes to ensure proper operation
- **Keep backups** of original configuration values

### Recommended Testing Procedure

1. Read current values from BMS and document them
2. Make small incremental changes
3. Monitor BMS behavior under normal operation
4. Test protection triggering with safe test conditions
5. Verify protection releases properly
6. Only after validation, use in production

## Register Map Reference

Full register documentation is available in `REGISTER_MAP.md`, including:
- Complete register addresses and descriptions
- Value ranges and units
- Encoding formulas
- Example values from actual BMS

## Future Enhancements

### Planned Features
- **Read-back validation**: After writing, read register to confirm change
- **Batch writes**: Use Modbus function 0x10 to write multiple registers efficiently
- **Configuration backup/restore**: Save and restore full BMS configuration
- **Preset profiles**: Pre-defined safe configurations for common cell chemistries
- **Write protection**: Require confirmation for critical parameter changes

### Testing Needed
- Hardware validation with actual EG4 BMS
- Verify all 53 parameters write correctly
- Test temperature offset handling
- Confirm protection triggering at new thresholds
- Validate multi-BMS configurations (addresses 0x01-0x10)

## Development Notes

### Pattern Source
This implementation follows the established pattern from the JK-BMS ESPHome component, adapted for EG4 BMS specifics:
- Similar number entity structure
- Comparable register mapping approach
- Matching control() method pattern
- Consistent Modbus write implementation

### Key Differences from JK-BMS
1. Temperature encoding uses +50 offset instead of signed values
2. Two separate temperature register ranges (0x005A-0x0068, 0x0080-0x0087)
3. Different factor values for voltage conversion (mV vs 0.1V for cells/pack)
4. Extended protection parameters (heating control, ambient temperature)

### Automatic Temperature Detection
The implementation automatically detects temperature registers by address range:
```cpp
bool is_temperature = (this->holding_register_ >= 0x005A && this->holding_register_ <= 0x0068) ||
                      (this->holding_register_ >= 0x0080 && this->holding_register_ <= 0x0087);
```

This eliminates the need for manual flagging of temperature parameters in the schema.

## Integration Status

✅ **Completed**
- Number component structure
- Register mappings and schemas
- Temperature offset handling
- Modbus write support
- Example configuration

⏳ **Pending**
- Hardware testing
- Documentation of heating control registers (registers need confirmation)
- Batch write implementation
- Read-back validation

## Files Modified/Created

### Created
- `components/eg4_bms/number/__init__.py`
- `components/eg4_bms/number/eg4_number.h`
- `components/eg4_bms/number/eg4_number.cpp`
- `esp32-example-with-config.yaml`
- `CONFIGURATION.md` (this file)

### Modified
- `components/eg4_bms/__init__.py` - Added "number" to AUTO_LOAD
- `components/eg4_bms/eg4_bms.h` - Added number.h include, 53 setters, 53 member variables
- `components/eg4_modbus/eg4_modbus.h` - Added send_write_single() and write_register()
- `components/eg4_modbus/eg4_modbus.cpp` - Implemented Modbus write function 0x06

## Version History

- **v1.1.0-dev** (Current): Added writable configuration parameters
- **v1.0.0**: Initial read-only implementation
