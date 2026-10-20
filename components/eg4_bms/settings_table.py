"""Parameter block registers shared by the sensor and number platforms.

Every table here is a pure literal so the host test
(tests/settings_table_test.py) can read it with ast, without ESPHome.
"""

# Parameter block (FC03 0x2D x91), served only on the pack's RS232 service port
# (115200 8N1, address 1 at any DIP ID, no other pack on its paralleling ports).
# key: (register, multiplier, offset, unit, device_class, accuracy_decimals)
# Published value = raw * multiplier + offset. Thresholds come in warning /
# protection / release triples. Temperatures are stored as degC + 50. Units
# marked "inferred" in REGISTER_MAP.md were not confirmed by the BMS tool;
# counts and delays are published raw.
SETTINGS = {
    "balance_start_voltage": (0x38, 0.001, 0, "V", "voltage", 3),
    "balance_delta_voltage": (0x39, 0.001, 0, "V", "voltage", 3),
    "low_capacity_warning": (0x3A, 1, 0, "%", None, 0),
    "cell_undervoltage_warning": (0x3D, 0.001, 0, "V", "voltage", 3),
    "cell_undervoltage_protection": (0x3E, 0.001, 0, "V", "voltage", 3),
    "cell_undervoltage_release": (0x3F, 0.001, 0, "V", "voltage", 3),
    "pack_undervoltage_warning": (0x40, 0.01, 0, "V", "voltage", 2),
    "pack_undervoltage_protection": (0x41, 0.01, 0, "V", "voltage", 2),
    "pack_undervoltage_release": (0x42, 0.01, 0, "V", "voltage", 2),
    "cell_overvoltage_warning": (0x43, 0.001, 0, "V", "voltage", 3),
    "cell_overvoltage_protection": (0x44, 0.001, 0, "V", "voltage", 3),
    "cell_overvoltage_release": (0x45, 0.001, 0, "V", "voltage", 3),
    "pack_overvoltage_warning": (0x46, 0.01, 0, "V", "voltage", 2),
    "pack_overvoltage_protection": (0x47, 0.01, 0, "V", "voltage", 2),
    "pack_overvoltage_release": (0x48, 0.01, 0, "V", "voltage", 2),
    "charge_overcurrent_count": (0x4C, 1, 0, None, None, 0),
    "discharge_overcurrent_count": (0x4D, 1, 0, None, None, 0),
    "charge_overcurrent_release_time": (0x4E, 1, 0, None, None, 0),
    "discharge_overcurrent_release_time": (0x4F, 1, 0, None, None, 0),
    "charge_overcurrent_1_protection": (0x50, 0.01, 0, "A", "current", 1),
    "discharge_overcurrent_1_protection": (0x51, 0.01, 0, "A", "current", 1),
    "short_circuit_current": (0x52, 0.01, 0, "A", "current", 1),
    "charge_overcurrent_2_protection": (0x53, 0.01, 0, "A", "current", 1),
    "discharge_overcurrent_2_protection": (0x54, 0.01, 0, "A", "current", 1),
    "charge_overcurrent_1_delay": (0x55, 1, 0, None, None, 0),
    "charge_overcurrent_2_delay": (0x56, 1, 0, None, None, 0),
    "discharge_overcurrent_1_delay": (0x57, 1, 0, None, None, 0),
    "discharge_overcurrent_2_delay": (0x58, 1, 0, None, None, 0),
    "charge_undertemp_warning": (0x5A, 1, -50, "°C", "temperature", 0),
    "charge_undertemp_protection": (0x5B, 1, -50, "°C", "temperature", 0),
    "charge_undertemp_release": (0x5C, 1, -50, "°C", "temperature", 0),
    "charge_overtemp_warning": (0x5D, 1, -50, "°C", "temperature", 0),
    "charge_overtemp_protection": (0x5E, 1, -50, "°C", "temperature", 0),
    "charge_overtemp_release": (0x5F, 1, -50, "°C", "temperature", 0),
    "discharge_undertemp_warning": (0x60, 1, -50, "°C", "temperature", 0),
    "discharge_undertemp_protection": (0x61, 1, -50, "°C", "temperature", 0),
    "discharge_undertemp_release": (0x62, 1, -50, "°C", "temperature", 0),
    "discharge_overtemp_warning": (0x63, 1, -50, "°C", "temperature", 0),
    "discharge_overtemp_protection": (0x64, 1, -50, "°C", "temperature", 0),
    "discharge_overtemp_release": (0x65, 1, -50, "°C", "temperature", 0),
    "mos_overtemp_warning": (0x66, 1, -50, "°C", "temperature", 0),
    "mos_overtemp_protection": (0x67, 1, -50, "°C", "temperature", 0),
    "mos_overtemp_release": (0x68, 1, -50, "°C", "temperature", 0),
    "heater_on_temperature": (0x80, 1, -50, "°C", "temperature", 0),
    "heater_off_temperature": (0x81, 1, -50, "°C", "temperature", 0),
    "ambient_undertemp_warning": (0x82, 1, -50, "°C", "temperature", 0),
    "ambient_undertemp_protection": (0x83, 1, -50, "°C", "temperature", 0),
    "ambient_undertemp_release": (0x84, 1, -50, "°C", "temperature", 0),
    "ambient_overtemp_warning": (0x85, 1, -50, "°C", "temperature", 0),
    "ambient_overtemp_protection": (0x86, 1, -50, "°C", "temperature", 0),
    "ambient_overtemp_release": (0x87, 1, -50, "°C", "temperature", 0),
}


# Writable range for each setting, in published units: (min, max, step).
# The BMS validates nothing, so these bound every write. No limit lets a
# protection go past what the pack ships with (ZTR-16S200A, 2026-10-20):
# cell OV 3.80 V / pack OV 60.80 V, cell UV 2.30 V, currents 300 A, charge
# OT 70 C, discharge OT 75 C, MOSFET OT 110 C, ambient OT 80 C; charge UT
# no lower than -10 C. Tightening is always possible, loosening is not.
LIMITS = {
    "balance_start_voltage": (3.3, 3.6, 0.001),
    "balance_delta_voltage": (0.005, 0.1, 0.001),
    "low_capacity_warning": (0, 100, 1),
    "cell_undervoltage_warning": (2.3, 3.4, 0.001),
    "cell_undervoltage_protection": (2.3, 3.4, 0.001),
    "cell_undervoltage_release": (2.3, 3.4, 0.001),
    "pack_undervoltage_warning": (36.8, 54.4, 0.01),
    "pack_undervoltage_protection": (36.8, 54.4, 0.01),
    "pack_undervoltage_release": (36.8, 54.4, 0.01),
    "cell_overvoltage_warning": (3.3, 3.8, 0.001),
    "cell_overvoltage_protection": (3.3, 3.8, 0.001),
    "cell_overvoltage_release": (3.3, 3.8, 0.001),
    "pack_overvoltage_warning": (52.8, 60.8, 0.01),
    "pack_overvoltage_protection": (52.8, 60.8, 0.01),
    "pack_overvoltage_release": (52.8, 60.8, 0.01),
    "charge_overcurrent_count": (1, 255, 1),
    "discharge_overcurrent_count": (1, 255, 1),
    "charge_overcurrent_release_time": (0, 3600, 1),
    "discharge_overcurrent_release_time": (0, 3600, 1),
    "charge_overcurrent_1_protection": (1, 300, 0.1),
    "discharge_overcurrent_1_protection": (1, 300, 0.1),
    "short_circuit_current": (1, 300, 0.1),
    "charge_overcurrent_2_protection": (1, 300, 0.1),
    "discharge_overcurrent_2_protection": (1, 300, 0.1),
    "charge_overcurrent_1_delay": (0, 255, 1),
    "charge_overcurrent_2_delay": (0, 255, 1),
    "discharge_overcurrent_1_delay": (0, 255, 1),
    "discharge_overcurrent_2_delay": (0, 255, 1),
    "charge_undertemp_warning": (-10, 20, 1),
    "charge_undertemp_protection": (-10, 20, 1),
    "charge_undertemp_release": (-10, 20, 1),
    "charge_overtemp_warning": (30, 70, 1),
    "charge_overtemp_protection": (30, 70, 1),
    "charge_overtemp_release": (30, 70, 1),
    "discharge_undertemp_warning": (-30, 20, 1),
    "discharge_undertemp_protection": (-30, 20, 1),
    "discharge_undertemp_release": (-30, 20, 1),
    "discharge_overtemp_warning": (30, 75, 1),
    "discharge_overtemp_protection": (30, 75, 1),
    "discharge_overtemp_release": (30, 75, 1),
    "mos_overtemp_warning": (50, 110, 1),
    "mos_overtemp_protection": (50, 110, 1),
    "mos_overtemp_release": (50, 110, 1),
    "heater_on_temperature": (-20, 15, 1),
    "heater_off_temperature": (-20, 20, 1),
    "ambient_undertemp_warning": (-30, 20, 1),
    "ambient_undertemp_protection": (-30, 20, 1),
    "ambient_undertemp_release": (-30, 20, 1),
    "ambient_overtemp_warning": (30, 80, 1),
    "ambient_overtemp_protection": (30, 80, 1),
    "ambient_overtemp_release": (30, 80, 1),
}

# A write is refused unless every rule still holds with the values the BMS
# last reported (and any writes still in flight). Over-limit triples need
# release < warning < protection, under-limit triples protection < warning <
# release. Both settings of a rule share a scale, so the check runs on raw
# values.
ORDER = [
    # (lower, upper, strict). Strict keeps hysteresis between a protection and
    # its release; over-current stages may be equal (this pack ships with
    # discharge stage 2 == short circuit == 300 A).
    ("cell_overvoltage_release", "cell_overvoltage_warning", True),
    ("cell_overvoltage_warning", "cell_overvoltage_protection", True),
    ("pack_overvoltage_release", "pack_overvoltage_warning", True),
    ("pack_overvoltage_warning", "pack_overvoltage_protection", True),
    ("cell_undervoltage_protection", "cell_undervoltage_warning", True),
    ("cell_undervoltage_warning", "cell_undervoltage_release", True),
    ("pack_undervoltage_protection", "pack_undervoltage_warning", True),
    ("pack_undervoltage_warning", "pack_undervoltage_release", True),
    ("charge_overtemp_release", "charge_overtemp_warning", True),
    ("charge_overtemp_warning", "charge_overtemp_protection", True),
    ("discharge_overtemp_release", "discharge_overtemp_warning", True),
    ("discharge_overtemp_warning", "discharge_overtemp_protection", True),
    ("mos_overtemp_release", "mos_overtemp_warning", True),
    ("mos_overtemp_warning", "mos_overtemp_protection", True),
    ("ambient_overtemp_release", "ambient_overtemp_warning", True),
    ("ambient_overtemp_warning", "ambient_overtemp_protection", True),
    ("charge_undertemp_protection", "charge_undertemp_warning", True),
    ("charge_undertemp_warning", "charge_undertemp_release", True),
    ("discharge_undertemp_protection", "discharge_undertemp_warning", True),
    ("discharge_undertemp_warning", "discharge_undertemp_release", True),
    ("ambient_undertemp_protection", "ambient_undertemp_warning", True),
    ("ambient_undertemp_warning", "ambient_undertemp_release", True),
    ("heater_on_temperature", "heater_off_temperature", True),
    ("charge_overcurrent_1_protection", "charge_overcurrent_2_protection", False),
    ("discharge_overcurrent_1_protection", "discharge_overcurrent_2_protection", False),
    ("discharge_overcurrent_2_protection", "short_circuit_current", False),
    # Across groups: the under-voltage release must stay below the over-voltage
    # release, and balancing must start below the over-voltage warning.
    ("cell_undervoltage_release", "cell_overvoltage_release", True),
    ("balance_start_voltage", "cell_overvoltage_warning", True),
]
