# ESPHome EG4 BMS Component

> **⚠️ UNTESTED - ALPHA RELEASE**  
> This component compiles successfully but has not yet been tested with actual EG4 hardware. Use at your own risk and please report any issues or test results!

ESPHome component to monitor EG4 Battery Management Systems (EG4 BMS) via RS485/Modbus RTU.

This is a native C++ implementation that communicates directly with the EG4 BMS using the Modbus RTU protocol, providing better stability and lower resource usage compared to using the built-in ESPHome Modbus controller.

## Supported Devices

- EG4 LL (Lifeline) series batteries
- EG4-LL-S V2 (formerly known as EG4-LL)
- Any EG4 battery with RS485/Modbus RTU interface

## Features

- **Native Modbus RTU implementation** - Direct communication without external dependencies
- **Comprehensive monitoring** - Voltage, current, temperature, capacity, and more
- **Individual cell voltages** - Monitor all 16 cells independently
- **Status reporting** - Real-time status, warnings, protection, and errors
- **Multi-BMS support** - Connect multiple BMSes on the same RS485 bus
- **Low resource usage** - Efficient C++ implementation
- **Home Assistant integration** - Automatic device discovery and configuration

## Hardware Requirements

- ESP32 or ESP8266 board
- RS485 to TTL converter module (e.g., MAX485, SP3485)
- EG4 BMS with RS485 port

## Wiring Diagram

```
                  RS485                      UART
┌────────────┐              ┌──────────┐                ┌─────────┐
│            │              │          │<----- RX ----->│         │
│   EG4      │<-----B- ---->│  RS485   │<----- TX ----->│ ESP32/  │
│   BMS      │<---- A+ ---->│  to TTL  │<----- GND ---->│ ESP8266 │
│            │<--- GND ---->│  module  │<----- 3.3V --->│         │
│            │              │          │                │         │
└────────────┘              └──────────┘                └─────────┘
```

**Important:** Power the RS485 module with 3.3V to match the ESP's logic level.

### EG4 BMS RJ45 Pinout

| Pin | Signal | Wire Color (T-568B) |
|-----|--------|---------------------|
| 1   | -      | -                   |
| 2   | -      | -                   |
| 3   | GND    | Green/White         |
| 4   | B-     | Blue                |
| 5   | A+     | Blue/White          |
| 6   | -      | -                   |
| 7   | -      | -                   |
| 8   | -      | -                   |

## Installation

### Option 1: Using External Components (Recommended)

Add to your ESPHome YAML configuration:

```yaml
external_components:
  - source: github://rar/esphome-eg4-bms@main
    refresh: 0s
```

### Option 2: Local Installation

1. Clone this repository
2. Copy the `components` folder to your ESPHome configuration directory
3. Reference it in your YAML:

```yaml
external_components:
  - source: components
```

## Configuration

### Basic Example

```yaml
uart:
  id: uart_0
  baud_rate: 9600
  tx_pin: GPIO16
  rx_pin: GPIO17
  rx_buffer_size: 256

eg4_modbus:
  id: modbus0
  uart_id: uart_0

eg4_bms:
  id: bms0
  address: 0x10  # Default address for master BMS
  eg4_modbus_id: modbus0
  update_interval: 10s

sensor:
  - platform: eg4_bms
    eg4_bms_id: bms0
    total_voltage:
      name: "Battery Voltage"
    current:
      name: "Battery Current"
    state_of_charge:
      name: "Battery SOC"
```

### Complete Examples

See the example configurations in this repository:
- [esp32-example.yaml](esp32-example.yaml) - Full configuration for ESP32
- [esp8266-example.yaml](esp8266-example.yaml) - Minimal configuration for ESP8266

## Configuration Variables

### `eg4_modbus`

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | **Required** | - | ID of the Modbus component |
| `uart_id` | **Required** | - | ID of the UART component |
| `flow_control_pin` | Optional | - | GPIO pin for RS485 flow control (DE/RE) |

### `eg4_bms`

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | **Required** | - | ID of the BMS component |
| `eg4_modbus_id` | **Required** | - | ID of the parent Modbus component |
| `address` | Optional | `0x10` | Modbus address of the BMS (0x01-0x10) |
| `update_interval` | Optional | `10s` | How often to poll the BMS |

### BMS Addressing

- **Master BMS**: Address `0x10` (default)
- **Battery Module 1**: Address `0x01`
- **Battery Module 2**: Address `0x02`
- ...
- **Battery Module 15**: Address `0x0F`

Use DIP switches on the BMS to configure the address.

## Available Sensors

### Binary Sensors

- `online_status` - BMS communication status
- `heating` - Battery heating active
- `charging` - Battery is charging
- `discharging` - Battery is discharging

### Sensors

**Voltage:**
- `total_voltage` - Total battery pack voltage (V)
- `min_cell_voltage` - Minimum cell voltage (V)
- `max_cell_voltage` - Maximum cell voltage (V)
- `delta_cell_voltage` - Voltage difference between min and max cells (V)
- `average_cell_voltage` - Average cell voltage (V)
- `cell_voltage_1` to `cell_voltage_16` - Individual cell voltages (V)

**Current and Power:**
- `current` - Battery current (A, positive = charging, negative = discharging)
- `power` - Battery power (W)
- `charging_power` - Charging power (W)
- `discharging_power` - Discharging power (W)

**Temperature:**
- `pcb_temperature` - PCB temperature (°C)
- `avg_temperature` - Average temperature (°C)
- `max_temperature` - Maximum temperature (°C)
- `temperature_1` to `temperature_6` - Individual temperature sensors (°C)

**Capacity:**
- `remaining_capacity` - Remaining capacity (Ah)
- `full_capacity` - Full capacity (Ah)
- `designed_capacity` - Designed capacity (Ah)

**State:**
- `state_of_charge` - State of charge (%)
- `state_of_health` - State of health (%)
- `cycle_count` - Charge/discharge cycles
- `max_charge_current` - Maximum charging current (A)
- `cell_count` - Number of cells in the pack

### Text Sensors

- `status` - Battery status (Standby, Charging, Discharging, etc.)
- `warnings` - Active warnings
- `protection` - Active protections
- `error` - Error codes
- `model` - BMS model number
- `firmware_version` - BMS firmware version
- `serial_number` - BMS serial number

## Status Codes

### Battery Status
- **Standby** - Battery is idle
- **Charging** - Battery is charging
- **Discharging** - Battery is discharging
- **Protect** - Protection is active
- **Charging Limited** - Charging current is limited
- **Heating+[Status]** - Battery heating is active

### Warnings and Protections

- **Pack OV** - Pack overvoltage
- **Cell OV** - Cell overvoltage
- **Pack UV** - Pack undervoltage
- **Cell UV** - Cell undervoltage
- **Charge OC** - Charge overcurrent
- **Discharge OC** - Discharge overcurrent
- **Abnormal Temp** - Abnormal ambient temperature
- **MOS Overheat** - MOSFET overheating
- **Charge OT** - Charge over-temperature
- **Discharge OT** - Discharge over-temperature
- **Charge UT** - Charge under-temperature
- **Discharge UT** - Discharge under-temperature
- **Low Capacity** - Low capacity warning
- **Discharge SC** - Discharge short circuit

### Error Codes

- **Voltage Error** - Voltage sensor error
- **Temperature Error** - Temperature sensor error
- **Current Flow Error** - Current sensor error
- **Cell Unbalance** - Cell voltage unbalance error

## Debugging

Enable debug logging to see communication details:

```yaml
logger:
  level: DEBUG
  logs:
    eg4_modbus: DEBUG
    eg4_bms: DEBUG

uart:
  id: uart_0
  baud_rate: 9600
  tx_pin: GPIO16
  rx_pin: GPIO17
  debug:
    direction: BOTH
    dummy_receiver: false
    after:
      delimiter: "\r\n"
    sequence:
      - lambda: UARTDebug::log_hex(direction, bytes, ':');
```

## Troubleshooting

### No data received

1. **Check wiring** - Verify A+, B-, and GND connections
2. **Check baud rate** - EG4 BMS default is 9600
3. **Check address** - Make sure the address matches your BMS configuration
4. **Check power** - RS485 module should be powered with 3.3V
5. **Check termination** - Long cable runs may need 120Ω termination resistors

### Incorrect readings

1. **Check register mapping** - Different BMS models may have different register layouts
2. **Update firmware** - Ensure your BMS has the latest firmware
3. **Check polling interval** - Don't poll too frequently (minimum 1 second recommended)

### CRC errors

1. **Check cable quality** - Use shielded twisted pair cable for RS485
2. **Check cable length** - Keep cables as short as possible (max 1200m for RS485)
3. **Check termination** - Add 120Ω termination resistors at both ends

## Protocol Information

This component implements the EG4 Battery Modbus RTU protocol as documented in the EG4-LL-MODBUS-Communication-Protocol documentation.

**Protocol Details:**
- **Type:** Modbus RTU
- **Function Code:** 0x03 (Read Holding Registers)
- **Baud Rate:** 9600 (default), configurable up to 19200
- **Data Bits:** 8
- **Stop Bits:** 1
- **Parity:** None

## Comparison with Modbus Controller

### Why use this component instead of the built-in Modbus controller?

**Advantages:**
- ✅ **Simpler configuration** - No need to manually configure each register
- ✅ **Better stability** - Native implementation with proper error handling
- ✅ **Lower resource usage** - More efficient memory and CPU usage
- ✅ **Automatic calculations** - Delta voltage, power, etc. calculated automatically
- ✅ **Status decoding** - Human-readable status, warnings, and errors
- ✅ **Better Home Assistant integration** - Proper device classes and units

**Migration from Modbus Controller:**
Simply replace your modbus_controller configuration with this component. All sensors will be available with clearer naming and better organization.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.

## Credits

- Inspired by [esphome-seplos-bms](https://github.com/syssi/esphome-seplos-bms) by syssi
- Based on the EG4 Modbus protocol documentation
- Original Modbus controller implementation from [esphome-yambms](https://github.com/Sleeper85/esphome-yambms)

## Changelog

### v1.0.0 (2025-12-08)
- Initial release
- Native Modbus RTU implementation
- Support for all EG4 LL series batteries
- Comprehensive sensor suite
- Multi-BMS support

## Support

For issues, questions, or feature requests, please open an issue on GitHub.
