# Quick Start Guide - EG4 BMS ESPHome Component

## What You'll Need

### Hardware
- ESP32 or ESP8266 development board
- RS485 to TTL converter (MAX485 or similar)
- EG4 Battery with RS485 port
- Jumper wires
- RJ45 cable (to connect to BMS)

### Software
- ESPHome 2024.6.0 or newer
- Home Assistant (optional, but recommended)

## Step 1: Wire Everything Up

Connect your RS485 converter to the ESP:
- RS485 RX → ESP TX (GPIO16 on ESP32)
- RS485 TX → ESP RX (GPIO17 on ESP32)
- RS485 VCC → ESP 3.3V
- RS485 GND → ESP GND

Connect the RS485 converter to the EG4 BMS RJ45 port:
- A (pin 2 or pin 7 of RJ45)
- B (pin 1 or pin 8 of RJ45)
- GND (pin 3 or pin 6 of RJ45)

Note: the RS485 A/B pair is bridged across pins 1/8 and 2/7, so either end of the connector works. Do not use pins 4/5 — they are not RS485 on this port.

## Step 2: Create Your Configuration

1. Copy `esp32-example.yaml` to a new file (e.g., `my-eg4-bms.yaml`)
2. Create `secrets.yaml` with your WiFi credentials:
   ```yaml
   wifi_ssid: "YourWiFiName"
   wifi_password: "YourWiFiPassword"
   ```

3. Modify the device name in your config:
   ```yaml
   substitutions:
     name: my-battery-monitor
   ```

## Step 3: Flash Your ESP

```bash
# Install ESPHome if you haven't already
pip install esphome

# Compile and upload
esphome run my-eg4-bms.yaml
```

## Step 4: Check the Logs

Once flashed, monitor the logs to verify communication:

```bash
esphome logs my-eg4-bms.yaml
```

You should see:
```
[I][eg4_bms:xxx]: Processing xx bytes of data
[D][sensor:xxx]: 'my-battery-monitor total voltage': Sending state 52.40 V
[D][sensor:xxx]: 'my-battery-monitor current': Sending state 12.34 A
```

## Step 5: Add to Home Assistant

If using Home Assistant with the API enabled, the device will auto-discover. Go to:
**Settings → Devices & Services → ESPHome**

You should see your battery monitor appear automatically!

## Troubleshooting

### "No response from BMS"
- Check wiring, especially A+/B- (try swapping them)
- Verify baud rate is 9600
- Check BMS address (default is 0x10)

### "CRC check failed"
- Cable might be too long or poor quality
- Check all connections are secure
- Try adding 120Ω termination resistors

### "Incorrect values"
- Some BMS models may have different register layouts
- Check the BMS documentation
- Open an issue on GitHub with your BMS model

## Next Steps

- **Customize sensors**: Remove sensors you don't need to save memory
- **Add automations**: Use the sensors in Home Assistant automations
- **Monitor multiple batteries**: Connect additional BMSes with different addresses
- **Create dashboards**: Visualize your battery data in Home Assistant

## Getting Help

- Read the full [README.md](README.md)
- Check existing [GitHub Issues](https://github.com/rar/esphome-eg4-bms/issues)
- Review the [protocol documentation](docs/EG4-LL-MODBUS-Communication-Protocol.pdf)

## Safety Warning

⚠️ **Important**: This component is for monitoring only. Do not rely on it for critical battery protection. Your BMS has built-in protections - this is for data visibility only.

Battery systems can be dangerous if misused. Always:
- Follow manufacturer guidelines
- Use proper wire gauges
- Install appropriate fuses and circuit breakers
- Monitor for unusual behavior
- Keep batteries in well-ventilated areas
