"""Checks components/eg4_bms/sensor.py's SETTINGS table against a real reply.

    make -C tests          # runs with the C++ tests

Decodes the 0x2D x91 parameter block in rs232_service_frames.txt with the
table exactly as the component does (raw * multiplier + offset) and compares
the result with the values the vendor BMS tool shows for that pack. Reads the
table with ast, so ESPHome is not needed.
"""

import ast
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
SENSOR_PY = HERE.parent / "components" / "eg4_bms" / "sensor.py"
REG_SETTINGS, SETTINGS_REGISTERS = 0x2D, 0x5B

EXPECTED = {
    "balance_start_voltage": 3.4,
    "balance_delta_voltage": 0.02,
    "low_capacity_warning": 10,
    "cell_undervoltage_warning": 2.5,
    "cell_undervoltage_protection": 2.3,
    "cell_undervoltage_release": 3.1,
    "pack_undervoltage_warning": 46.5,
    "pack_undervoltage_protection": 44.8,
    "pack_undervoltage_release": 48.0,
    "cell_overvoltage_warning": 3.7,
    "cell_overvoltage_protection": 3.8,
    "cell_overvoltage_release": 3.45,
    "pack_overvoltage_warning": 59.2,
    "pack_overvoltage_protection": 60.0,
    "pack_overvoltage_release": 55.2,
    "charge_overcurrent_count": 5,
    "discharge_overcurrent_count": 5,
    "charge_overcurrent_release_time": 180,
    "discharge_overcurrent_release_time": 60,
    "charge_overcurrent_1_protection": 205.0,
    "discharge_overcurrent_1_protection": 205.0,
    "short_circuit_current": 300.0,
    "charge_overcurrent_2_protection": 225.0,
    "discharge_overcurrent_2_protection": 300.0,
    "charge_overcurrent_1_delay": 10,
    "charge_overcurrent_2_delay": 3,
    "discharge_overcurrent_1_delay": 10,
    "discharge_overcurrent_2_delay": 3,
    "charge_undertemp_warning": -2,
    "charge_undertemp_protection": -5,
    "charge_undertemp_release": 0,
    "charge_overtemp_warning": 65,
    "charge_overtemp_protection": 70,
    "charge_overtemp_release": 60,
    "discharge_undertemp_warning": -15,
    "discharge_undertemp_protection": -20,
    "discharge_undertemp_release": -10,
    "discharge_overtemp_warning": 70,
    "discharge_overtemp_protection": 75,
    "discharge_overtemp_release": 65,
    "mos_overtemp_warning": 100,
    "mos_overtemp_protection": 110,
    "mos_overtemp_release": 80,
    "heater_on_temperature": 0,
    "heater_off_temperature": 3,
    "ambient_undertemp_warning": -20,
    "ambient_undertemp_protection": -25,
    "ambient_undertemp_release": -15,
    "ambient_overtemp_warning": 70,
    "ambient_overtemp_protection": 80,
    "ambient_overtemp_release": 60,
}


def load_settings():
    tree = ast.parse(SENSOR_PY.read_text(encoding="utf-8"))
    for node in tree.body:
        if isinstance(node, ast.Assign) and any(
            isinstance(t, ast.Name) and t.id == "SETTINGS" for t in node.targets
        ):
            return ast.literal_eval(node.value)
    raise SystemExit("SETTINGS not found in sensor.py")


def load_parameter_block(path):
    for line in path.read_text().splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] == "rs232":
            frame = bytes.fromhex(parts[2])
            if frame[1] == 3 and frame[2] == SETTINGS_REGISTERS * 2:
                data = frame[3:-2]
                return [int.from_bytes(data[i : i + 2], "big") for i in range(0, len(data), 2)]
    raise SystemExit("no parameter block reply in " + str(path))


def main():
    settings = load_settings()
    regs = load_parameter_block(HERE / "rs232_service_frames.txt")
    errors = []

    seen = {}
    for key, (register, *_rest) in settings.items():
        if not (0x2F <= register <= 0x68 or 0x80 <= register <= 0x87):
            errors.append(f"{key}: register 0x{register:02X} is not a threshold register")
        if register in seen:
            errors.append(f"{key}: register 0x{register:02X} also used by {seen[register]}")
        seen[register] = key

    if set(settings) != set(EXPECTED):
        errors.append(f"table/expected keys differ: {sorted(set(settings) ^ set(EXPECTED))}")

    for key, expected in EXPECTED.items():
        if key not in settings:
            continue
        register, multiplier, offset, _unit, _device_class, _accuracy = settings[key]
        value = regs[register - REG_SETTINGS] * multiplier + offset
        if abs(value - expected) > 1e-6:
            errors.append(f"{key}: decoded {value}, BMS tool shows {expected}")

    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        print(f"FAILED: {len(errors)} check(s)")
        return 1
    print(f"All settings table tests passed ({len(settings)} settings)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
