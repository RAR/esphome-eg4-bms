# Wiring Schematic for EG4 BMS ESPHome Component

## Standard Wiring (Most Common)

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  EG4 BMS RJ45 Port (looking into the socket, tab down)         │
│  ┌─────────────────────────┐                                    │
│  │  1  2  3  4  5  6  7  8 │                                    │
│  │  │  │  │  │  │  │  │  │ │                                    │
│  └──┼──┼──┼──┼──┼──┼──┼──┼─┘                                    │
│     │  │  │  │  │  │  │  │                                      │
│     │  │  │  │  │  │  │  └── RS485 B  (Brown)                  │
│     │  │  │  │  │  │  └───── RS485 A  (Brown/White)            │
│     │  │  │  │  │  └──────── GND      (Green)                  │
│     │  │  │  │  └─────────── (unused)  (Blue/White)            │
│     │  │  │  └────────────── (unused)  (Blue)                  │
│     │  │  └───────────────── GND      (Green/White)            │
│     │  └──────────────────── RS485 A  (Orange)                 │
│     └─────────────────────── RS485 B  (Orange/White)           │
│                                                                 │
│  Pins 1/8 (B) and 2/7 (A) are bridged — use either end.        │
│  Pins 4/5 are not RS485 on this port. Do not tap them.         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                    │
                    ↓
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  RS485 to TTL Converter (e.g., MAX485)                         │
│  ┌────────────────────────────────────┐                        │
│  │  [VCC] [GND] [A+] [B-] [RX] [TX]  │                        │
│  │    │     │     │    │     │    │   │                        │
│  └────│─────│─────│────│─────│────│───┘                        │
│       │     │     │    │     │    │                            │
│       │     │     │    │     │    │                            │
└───────│─────│─────│────│─────│────│─────────────────────────────┘
        │     │     │    │     │    │
        ↓     ↓     ↓    ↓     ↓    ↓
┌───────────────────────────────────────────────────────────────────┐
│                                                                   │
│  ESP32 Development Board                                         │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                                                           │    │
│  │  3.3V ─────────────────────────┐                        │    │
│  │  GND ──────────────────┐       │                         │    │
│  │  GPIO17 (RX) ─────┐   │       │                         │    │
│  │  GPIO16 (TX) ──┐  │   │       │                         │    │
│  │                │  │   │       │                         │    │
│  └────────────────│──│───│───────│──────────────────────────┘    │
│                   │  │   │       │                               │
│                   │  │   │       │                               │
└───────────────────│──│───│───────│───────────────────────────────┘
                    │  │   │       │
                   TX  RX GND     VCC
```

## Component Pinout Reference

### RS485 Module Pins
| Pin | Function | Connect To |
|-----|----------|------------|
| VCC | Power | ESP 3.3V |
| GND | Ground | ESP GND (also tie to BMS RJ45 Pin 3 or 6) |
| A | RS485 A | BMS RJ45 Pin 2 or Pin 7 |
| B | RS485 B | BMS RJ45 Pin 1 or Pin 8 |
| RX | UART Receive | ESP TX Pin |
| TX | UART Transmit | ESP RX Pin |
| DE/RE | Flow Control (optional) | ESP GPIO (if needed) |

> **Note:** Pins 4 and 5 on this RJ45 are **not RS485**. Earlier versions of this guide listed pins 4/5 by mistake — if you wired to those pins you won't get RS485 traffic. Use pins 1/2 or 7/8 for this component.

### ESP32 Pins (Default Configuration)
| ESP32 Pin | Function | Connect To |
|-----------|----------|------------|
| GPIO16 | UART TX | RS485 RX |
| GPIO17 | UART RX | RS485 TX |
| 3.3V | Power | RS485 VCC |
| GND | Ground | RS485 GND |

### ESP8266 Pins (Default Configuration)
| ESP8266 Pin | Function | Connect To |
|-------------|----------|------------|
| GPIO1 (TX) | UART TX | RS485 RX |
| GPIO3 (RX) | UART RX | RS485 TX |
| 3.3V | Power | RS485 VCC |
| GND | Ground | RS485 GND |

## Multi-BMS Setup

To connect multiple BMSes on the same RS485 bus:

```
┌──────────┐     ┌──────────┐     ┌──────────┐
│  BMS #1  │     │  BMS #2  │     │  BMS #3  │
│  Addr:1  │     │  Addr:2  │     │  Addr:3  │
└────┬─────┘     └────┬─────┘     └────┬─────┘
     │                │                │
    A+/B-            A+/B-            A+/B-
     │                │                │
     └────────┬───────┴────────┬───────┘
              │                │
         ┌────┴────┐      ┌────┴────┐
         │  120Ω   │      │  120Ω   │
         │Terminator│      │Terminator│
         └────┬────┘      └─────────┘
              │
         RS485 Bus
              │
         ┌────┴────┐
         │ RS485   │
         │  to TTL │
         └────┬────┘
              │
            ESP32
```

**Important Notes for Multi-BMS:**
- Use separate `eg4_bms` entries with different `address` values
- Add 120Ω termination resistors at both ends of the bus
- Keep total cable length under 1200m (typical RS485 limit)
- Use shielded twisted pair cable for longer runs

## Power Supply Recommendations

### Option 1: Power from USB (Recommended for Development)
- Connect ESP32/ESP8266 via USB
- RS485 module powered from ESP's 3.3V pin
- **Limit:** Not suitable for permanent installation

### Option 2: External Power Supply (Recommended for Production)
- Use 5V power supply with voltage regulator
- Or use ESP board with onboard voltage regulator
- Provides stable power for reliable operation

### Option 3: Power from Battery System (Advanced)
- Use DC-DC buck converter (e.g., LM2596) if battery voltage > 5V
- Add proper fusing and protection
- **Warning:** Only if you understand battery system safety

## Important Warnings

⚠️ **Always use 3.3V for RS485 module VCC**
- Using 5V may damage the ESP's GPIO pins
- If your RS485 module is 5V-only, use a logic level converter

⚠️ **Check A+/B- polarity**
- If no communication, try swapping A+ and B-
- RS485 is differential, so polarity matters

⚠️ **Cable quality matters**
- Use twisted pair cable for RS485
- Shielded cable for electrically noisy environments
- Cat5e/Cat6 ethernet cable works well

⚠️ **Grounding**
- Connect GND between all devices
- Improper grounding can cause communication errors
- In multi-BMS setups, ensure common ground

## Testing Your Wiring

Before powering on:
1. ✅ Check all connections with multimeter
2. ✅ Verify no shorts between VCC and GND
3. ✅ Confirm A+ and B- are not swapped
4. ✅ Ensure RS485 module is powered from 3.3V

After powering on:
1. Check ESP boots normally (look for WiFi activity)
2. Check logs for "eg4_modbus" initialization
3. Look for Modbus communication in debug logs
4. Verify voltage readings make sense

## Troubleshooting

### No Communication
- Swap A+ and B- wires
- Check baud rate is 9600
- Verify BMS address (default 0x10)
- Try shorter cable

### Intermittent Communication
- Add termination resistors
- Check for loose connections
- Use better quality cable
- Reduce cable length

### Incorrect Data
- Verify power supply is stable
- Check for electromagnetic interference
- Ensure proper grounding
- Try lower polling frequency
