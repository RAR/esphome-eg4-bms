import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CURRENT,
    CONF_POWER,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_WATT,
)

from . import EG4_BMS_COMPONENT_SCHEMA, CONF_EG4_BMS_ID

DEPENDENCIES = ["eg4_bms"]

# Voltage sensors
CONF_TOTAL_VOLTAGE = "total_voltage"
CONF_MIN_CELL_VOLTAGE = "min_cell_voltage"
CONF_MAX_CELL_VOLTAGE = "max_cell_voltage"
CONF_DELTA_CELL_VOLTAGE = "delta_cell_voltage"
CONF_CELL_AVERAGE_VOLTAGE = "cell_average_voltage"
CONF_MIN_VOLTAGE_CELL = "min_voltage_cell"
CONF_MAX_VOLTAGE_CELL = "max_voltage_cell"
CONF_CELL_VOLTAGE_1 = "cell_voltage_1"
CONF_CELL_VOLTAGE_2 = "cell_voltage_2"
CONF_CELL_VOLTAGE_3 = "cell_voltage_3"
CONF_CELL_VOLTAGE_4 = "cell_voltage_4"
CONF_CELL_VOLTAGE_5 = "cell_voltage_5"
CONF_CELL_VOLTAGE_6 = "cell_voltage_6"
CONF_CELL_VOLTAGE_7 = "cell_voltage_7"
CONF_CELL_VOLTAGE_8 = "cell_voltage_8"
CONF_CELL_VOLTAGE_9 = "cell_voltage_9"
CONF_CELL_VOLTAGE_10 = "cell_voltage_10"
CONF_CELL_VOLTAGE_11 = "cell_voltage_11"
CONF_CELL_VOLTAGE_12 = "cell_voltage_12"
CONF_CELL_VOLTAGE_13 = "cell_voltage_13"
CONF_CELL_VOLTAGE_14 = "cell_voltage_14"
CONF_CELL_VOLTAGE_15 = "cell_voltage_15"
CONF_CELL_VOLTAGE_16 = "cell_voltage_16"

# Power sensors
CONF_CHARGING_POWER = "charging_power"
CONF_DISCHARGING_POWER = "discharging_power"

# Temperature sensors
CONF_TEMPERATURE_1 = "temperature_1"
CONF_TEMPERATURE_2 = "temperature_2"
CONF_TEMPERATURE_3 = "temperature_3"
CONF_TEMPERATURE_4 = "temperature_4"
CONF_TEMPERATURE_5 = "temperature_5"
CONF_TEMPERATURE_6 = "temperature_6"
CONF_PCB_TEMPERATURE = "pcb_temperature"
CONF_AVG_TEMPERATURE = "avg_temperature"
CONF_MAX_TEMPERATURE = "max_temperature"

# Capacity sensors
CONF_REMAINING_CAPACITY = "remaining_capacity"
CONF_FULL_CAPACITY = "full_capacity"
CONF_DESIGNED_CAPACITY = "designed_capacity"

# State sensors
CONF_STATE_OF_CHARGE = "state_of_charge"
CONF_STATE_OF_HEALTH = "state_of_health"
CONF_CYCLE_COUNT = "cycle_count"
CONF_MAX_CHARGE_CURRENT = "max_charge_current"
CONF_CELL_COUNT = "cell_count"

# Bitmask sensors
CONF_ERRORS_BITMASK = "errors_bitmask"
CONF_WARNINGS_BITMASK = "warnings_bitmask"
CONF_PROTECTION_BITMASK = "protection_bitmask"

# Diagnostic sensors
CONF_REJECTED_FRAME_COUNT = "rejected_frame_count"

UNIT_AMPERE_HOURS = "Ah"

# Parameter block (FC03 0x2D x91), served only on the pack's RS232 service port
# (115200 8N1, pack at DIP ID 1 with no other pack on its paralleling ports).
# key: (register, multiplier, offset, unit, device_class, accuracy_decimals)
# Published value = raw * multiplier + offset. Thresholds come in warning /
# protection / release triples. Temperatures are stored as degC + 50. Units
# marked "inferred" in REGISTER_MAP.md were not confirmed by the BMS tool;
# counts and delays are published raw. Kept a pure literal so the host test
# (tests/settings_table_test.py) can read it without ESPHome.
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


def setting_schema(unit, device_class, accuracy):
    kwargs = {
        "accuracy_decimals": accuracy,
        "entity_category": ENTITY_CATEGORY_DIAGNOSTIC,
    }
    if unit is not None:
        kwargs["unit_of_measurement"] = unit
    if device_class is not None:
        kwargs["device_class"] = device_class
    return sensor.sensor_schema(**kwargs)


CELL_VOLTAGE_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_VOLT,
    accuracy_decimals=3,
    device_class=DEVICE_CLASS_VOLTAGE,
    state_class=STATE_CLASS_MEASUREMENT,
)

TEMPERATURE_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=0,
    device_class=DEVICE_CLASS_TEMPERATURE,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = EG4_BMS_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(key): setting_schema(unit, device_class, accuracy)
        for key, (_, _, _, unit, device_class, accuracy) in SETTINGS.items()
    }
).extend(
    {
        # Voltage sensors
        cv.Optional(CONF_TOTAL_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_MIN_CELL_VOLTAGE): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_MAX_CELL_VOLTAGE): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_DELTA_CELL_VOLTAGE): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_AVERAGE_VOLTAGE): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_MIN_VOLTAGE_CELL): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_MAX_VOLTAGE_CELL): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_CELL_VOLTAGE_1): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_2): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_3): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_4): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_5): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_6): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_7): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_8): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_9): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_10): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_11): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_12): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_13): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_14): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_15): CELL_VOLTAGE_SCHEMA,
        cv.Optional(CONF_CELL_VOLTAGE_16): CELL_VOLTAGE_SCHEMA,
        # Current and power sensors
        cv.Optional(CONF_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CHARGING_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DISCHARGING_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        # Temperature sensors
        cv.Optional(CONF_TEMPERATURE_1): TEMPERATURE_SCHEMA,
        cv.Optional(CONF_TEMPERATURE_2): TEMPERATURE_SCHEMA,
        cv.Optional(CONF_TEMPERATURE_3): TEMPERATURE_SCHEMA,
        cv.Optional(CONF_TEMPERATURE_4): TEMPERATURE_SCHEMA,
        cv.Optional(CONF_TEMPERATURE_5): TEMPERATURE_SCHEMA,
        cv.Optional(CONF_TEMPERATURE_6): TEMPERATURE_SCHEMA,
        cv.Optional(CONF_PCB_TEMPERATURE): TEMPERATURE_SCHEMA,
        cv.Optional(CONF_AVG_TEMPERATURE): TEMPERATURE_SCHEMA,
        cv.Optional(CONF_MAX_TEMPERATURE): TEMPERATURE_SCHEMA,
        # Capacity sensors
        # No device_class: Home Assistant's "energy" class only accepts Wh/kWh
        # style units and rejects Ah, and there is no ampere-hour device class.
        cv.Optional(CONF_REMAINING_CAPACITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE_HOURS,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FULL_CAPACITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE_HOURS,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DESIGNED_CAPACITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE_HOURS,
            accuracy_decimals=1,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        # State sensors
        cv.Optional(CONF_STATE_OF_CHARGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:battery-70",
        ),
        cv.Optional(CONF_STATE_OF_HEALTH): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CYCLE_COUNT): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_MAX_CHARGE_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CELL_COUNT): sensor.sensor_schema(
            accuracy_decimals=0,
        ),
        # Bitmask sensors
        cv.Optional(CONF_ERRORS_BITMASK): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:alert-circle-outline",
        ),
        cv.Optional(CONF_WARNINGS_BITMASK): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:alert-outline",
        ),
        cv.Optional(CONF_PROTECTION_BITMASK): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:shield-alert-outline",
        ),
        # Diagnostic sensors
        cv.Optional(CONF_REJECTED_FRAME_COUNT): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:alert-decagram-outline",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_EG4_BMS_ID])

    # Voltage sensors
    if CONF_TOTAL_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_TOTAL_VOLTAGE])
        cg.add(hub.set_total_voltage_sensor(sens))
    
    if CONF_MIN_CELL_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_MIN_CELL_VOLTAGE])
        cg.add(hub.set_min_cell_voltage_sensor(sens))
    
    if CONF_MAX_CELL_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_MAX_CELL_VOLTAGE])
        cg.add(hub.set_max_cell_voltage_sensor(sens))
    
    if CONF_DELTA_CELL_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_DELTA_CELL_VOLTAGE])
        cg.add(hub.set_delta_cell_voltage_sensor(sens))
    
    if CONF_CELL_AVERAGE_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_CELL_AVERAGE_VOLTAGE])
        cg.add(hub.set_cell_average_voltage_sensor(sens))
    
    if CONF_MIN_VOLTAGE_CELL in config:
        sens = await sensor.new_sensor(config[CONF_MIN_VOLTAGE_CELL])
        cg.add(hub.set_min_voltage_cell_sensor(sens))
    
    if CONF_MAX_VOLTAGE_CELL in config:
        sens = await sensor.new_sensor(config[CONF_MAX_VOLTAGE_CELL])
        cg.add(hub.set_max_voltage_cell_sensor(sens))

    # Cell voltages
    for i in range(1, 17):
        conf_name = f"cell_voltage_{i}"
        if conf_name in config:
            sens = await sensor.new_sensor(config[conf_name])
            cg.add(hub.set_cell_voltage_sensor(i - 1, sens))

    # Current and power sensors
    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(hub.set_current_sensor(sens))
    
    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(hub.set_power_sensor(sens))
    
    if CONF_CHARGING_POWER in config:
        sens = await sensor.new_sensor(config[CONF_CHARGING_POWER])
        cg.add(hub.set_charging_power_sensor(sens))
    
    if CONF_DISCHARGING_POWER in config:
        sens = await sensor.new_sensor(config[CONF_DISCHARGING_POWER])
        cg.add(hub.set_discharging_power_sensor(sens))

    # Temperature sensors
    for i in range(1, 7):
        conf_name = f"temperature_{i}"
        if conf_name in config:
            sens = await sensor.new_sensor(config[conf_name])
            cg.add(hub.set_temperature_sensor(i - 1, sens))

    if CONF_PCB_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_PCB_TEMPERATURE])
        cg.add(hub.set_pcb_temperature_sensor(sens))
    
    if CONF_AVG_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_AVG_TEMPERATURE])
        cg.add(hub.set_avg_temperature_sensor(sens))
    
    if CONF_MAX_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_MAX_TEMPERATURE])
        cg.add(hub.set_max_temperature_sensor(sens))

    # Capacity sensors
    if CONF_REMAINING_CAPACITY in config:
        sens = await sensor.new_sensor(config[CONF_REMAINING_CAPACITY])
        cg.add(hub.set_remaining_capacity_sensor(sens))
    
    if CONF_FULL_CAPACITY in config:
        sens = await sensor.new_sensor(config[CONF_FULL_CAPACITY])
        cg.add(hub.set_full_capacity_sensor(sens))

    if CONF_DESIGNED_CAPACITY in config:
        sens = await sensor.new_sensor(config[CONF_DESIGNED_CAPACITY])
        cg.add(hub.set_designed_capacity_sensor(sens))
    
    # State sensors
    if CONF_STATE_OF_CHARGE in config:
        sens = await sensor.new_sensor(config[CONF_STATE_OF_CHARGE])
        cg.add(hub.set_state_of_charge_sensor(sens))
    
    if CONF_STATE_OF_HEALTH in config:
        sens = await sensor.new_sensor(config[CONF_STATE_OF_HEALTH])
        cg.add(hub.set_state_of_health_sensor(sens))
    
    if CONF_CYCLE_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_CYCLE_COUNT])
        cg.add(hub.set_cycle_count_sensor(sens))
    
    if CONF_MAX_CHARGE_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_MAX_CHARGE_CURRENT])
        cg.add(hub.set_max_charge_current_sensor(sens))
    
    if CONF_CELL_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_CELL_COUNT])
        cg.add(hub.set_cell_count_sensor(sens))
    
    # Bitmask sensors
    if CONF_ERRORS_BITMASK in config:
        sens = await sensor.new_sensor(config[CONF_ERRORS_BITMASK])
        cg.add(hub.set_errors_bitmask_sensor(sens))
    
    if CONF_WARNINGS_BITMASK in config:
        sens = await sensor.new_sensor(config[CONF_WARNINGS_BITMASK])
        cg.add(hub.set_warnings_bitmask_sensor(sens))
    
    if CONF_PROTECTION_BITMASK in config:
        sens = await sensor.new_sensor(config[CONF_PROTECTION_BITMASK])
        cg.add(hub.set_protection_bitmask_sensor(sens))

    # Diagnostic sensors
    if CONF_REJECTED_FRAME_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_REJECTED_FRAME_COUNT])
        cg.add(hub.set_rejected_frame_count_sensor(sens))

    # Parameter block
    for key, (register, multiplier, offset, _, _, _) in SETTINGS.items():
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(hub.add_setting_sensor(register, multiplier, offset, sens))
            cg.add(hub.set_read_settings(True))
