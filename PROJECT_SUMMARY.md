# ESPHome EG4 BMS Component - Project Summary

## Overview

This project provides a native C++ ESPHome component for monitoring EG4 Battery Management Systems via RS485/Modbus RTU. It's designed to be more stable, efficient, and easier to use than the built-in ESPHome Modbus controller approach.

## Project Structure

```
esphome-eg4-bms/
├── components/                      # ESPHome components
│   ├── eg4_modbus/                 # Modbus RTU communication layer
│   │   ├── __init__.py            # Python config for eg4_modbus
│   │   ├── eg4_modbus.h           # Header file
│   │   └── eg4_modbus.cpp         # Implementation (CRC, frame parsing)
│   └── eg4_bms/                   # BMS monitoring component
│       ├── __init__.py            # Python config for eg4_bms
│       ├── binary_sensor.py       # Binary sensor config
│       ├── sensor.py              # Sensor config (voltage, current, temp, etc.)
│       ├── text_sensor.py         # Text sensor config (status, warnings, etc.)
│       ├── eg4_bms.h              # Header file
│       └── eg4_bms.cpp            # Implementation (register parsing, decoding)
├── docs/                          # Documentation
│   └── EG4-LL-MODBUS-Communication-Protocol.pdf  # Protocol specification
├── esp32-example.yaml             # Full ESP32 example configuration
├── esp8266-example.yaml           # Minimal ESP8266 example
├── README.md                      # Main documentation
├── QUICKSTART.md                  # Quick start guide
├── WIRING.md                      # Detailed wiring diagrams
├── LICENSE                        # Apache 2.0 License
├── .gitignore                     # Git ignore file
└── secrets.yaml.example           # Example secrets file

```

## Key Features Implemented

### 1. Modbus RTU Communication (`eg4_modbus`)
- ✅ Native Modbus RTU implementation
- ✅ CRC16 validation
- ✅ Automatic frame detection and parsing
- ✅ Support for Read Holding Registers (0x03)
- ✅ Error handling and timeout detection
- ✅ Multi-device support (multiple BMSes on same bus)
- ✅ Optional flow control pin support

### 2. BMS Monitoring (`eg4_bms`)
- ✅ Comprehensive sensor suite (50+ sensors)
- ✅ Binary sensors (online status, charging, discharging, heating)
- ✅ Voltage sensors (total, cells 1-16, min, max, delta, average)
- ✅ Current and power sensors
- ✅ Temperature sensors (6 individual + PCB/avg/max)
- ✅ Capacity sensors (remaining, full, designed)
- ✅ State sensors (SOC, SOH, cycles, max charge current)
- ✅ Text sensors (status, warnings, protection, errors, model, firmware, serial)
- ✅ Automatic status decoding (human-readable)
- ✅ Warning and protection flag decoding
- ✅ Online/offline detection
- ✅ Sequential register polling (optimized for efficiency)

### 3. Configuration System
- ✅ Python codegen integration
- ✅ Flexible sensor configuration (enable only what you need)
- ✅ Configurable BMS address (0x01-0x10)
- ✅ Configurable polling interval
- ✅ Automatic Home Assistant integration

### 4. Documentation
- ✅ Comprehensive README with full API documentation
- ✅ Quick start guide for beginners
- ✅ Detailed wiring diagrams and schematics
- ✅ Troubleshooting guide
- ✅ Multi-BMS setup instructions
- ✅ Example configurations for ESP32 and ESP8266

## Technical Highlights

### Protocol Implementation
- **Modbus Function Code:** 0x03 (Read Holding Registers)
- **CRC:** Standard Modbus CRC16
- **Baud Rate:** 9600 (default), configurable
- **Addressing:** Supports 0x01-0x10 (individual modules + master BMS)

### Register Mapping (Based on EG4 Protocol)
- 0x0000-0x0001: Voltage and Current
- 0x0002-0x0011: Cell Voltages (16 cells)
- 0x0012-0x0014: Temperatures (PCB, average, max)
- 0x0015-0x0018: Remaining Capacity, Max Charge Current, SOH, SOC
- 0x0019-0x001C: Status, Warnings, Protection, Errors
- 0x001D-0x001E: Cycle Count (32-bit)
- 0x0021-0x0023: Individual Temperature Sensors (packed bytes)
- 0x0025: Full Capacity
- 0x0069 / 0x0075 / 0x0078: Model, Firmware, Serial

See REGISTER_MAP.md for the configuration registers (0x002D-0x0087).

### Optimizations
- **Batch register reads:** Multiple registers read in single Modbus request
- **Selective polling:** Device info polled less frequently than real-time data
- **Efficient memory usage:** Uses standard C++ containers
- **Smart parsing:** Only processes configured sensors

## Comparison with Alternatives

### vs. ESPHome Modbus Controller
| Feature | This Component | Modbus Controller |
|---------|---------------|-------------------|
| Configuration complexity | Low (sensor names) | High (manual registers) |
| Status decoding | Automatic | Manual |
| Resource usage | Low | Medium |
| Home Assistant integration | Excellent | Good |
| Multi-BMS support | Native | Complex |
| Maintenance | Easy | Difficult |

### vs. Original YAML Implementation
| Feature | This Component | YAML (YamBMS) |
|---------|---------------|---------------|
| Language | C++ | YAML |
| Stability | High | Medium |
| Dependencies | None (native) | Modbus controller |
| Performance | Better | Good |
| Extensibility | Easy (C++) | Limited (YAML) |

## Usage Example

```yaml
# Minimal configuration
uart:
  id: uart_0
  baud_rate: 9600
  tx_pin: GPIO16
  rx_pin: GPIO17

eg4_modbus:
  id: modbus0
  uart_id: uart_0

eg4_bms:
  id: bms0
  address: 0x10
  eg4_modbus_id: modbus0

sensor:
  - platform: eg4_bms
    eg4_bms_id: bms0
    total_voltage:
      name: "Battery Voltage"
    state_of_charge:
      name: "Battery SOC"
```

## Testing Status

The component has been designed based on:
- ✅ EG4-LL Modbus protocol documentation
- ✅ Analysis of working YamBMS implementation
- ✅ Study of similar implementations (Seplos BMS)
- ⏳ Pending: Real hardware testing

**Note:** This component has been thoroughly designed but requires testing with actual EG4 hardware. Please report any issues or test results!

## Future Enhancements (Potential)

- [ ] Write register support (configuration changes)
- [ ] Cell balancing status
- [ ] Historical data logging
- [ ] Alert/notification system
- [ ] Advanced diagnostics
- [ ] Web interface for configuration
- [ ] Support for additional EG4 BMS models

## Credits and Acknowledgments

- **Protocol:** Based on EG4-LL-MODBUS-Communication-Protocol.pdf
- **Inspiration:** syssi's esphome-seplos-bms (excellent reference implementation)
- **Original Implementation:** Sleeper85's esphome-yambms (Modbus controller approach)
- **ESPHome Team:** For the excellent platform

## License

Apache License 2.0 - See LICENSE file for details

## Contributing

Contributions welcome! Areas where help is needed:
- Real hardware testing and validation
- Additional BMS model support
- Performance optimization
- Documentation improvements
- Bug reports and fixes

## Getting Started

1. Read [QUICKSTART.md](QUICKSTART.md) for immediate setup
2. Review [WIRING.md](WIRING.md) for hardware connections
3. Check [README.md](README.md) for comprehensive documentation
4. Copy [esp32-example.yaml](esp32-example.yaml) and customize

## Support

- **GitHub Issues:** For bugs and feature requests
- **Discussions:** For questions and community support
- **Protocol Docs:** See `docs/` folder for technical details

---

**Created:** December 8, 2025  
**Version:** 1.0.0  
**Status:** Ready for testing  
**Author:** RAR
