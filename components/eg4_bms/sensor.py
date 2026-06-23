import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CURRENT,
    CONF_ID,
    CONF_POWER,
    CONF_TEMPERATURE,
    CONF_VOLTAGE,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
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

UNIT_AMPERE_HOURS = "Ah"

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
        ),
        cv.Optional(CONF_MAX_VOLTAGE_CELL): sensor.sensor_schema(
            accuracy_decimals=0,
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
        cv.Optional(CONF_REMAINING_CAPACITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE_HOURS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FULL_CAPACITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE_HOURS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DESIGNED_CAPACITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE_HOURS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_MEASUREMENT,
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
            icon="mdi:alert-circle-outline",
        ),
        cv.Optional(CONF_WARNINGS_BITMASK): sensor.sensor_schema(
            accuracy_decimals=0,
            icon="mdi:alert-outline",
        ),
        cv.Optional(CONF_PROTECTION_BITMASK): sensor.sensor_schema(
            accuracy_decimals=0,
            icon="mdi:shield-alert-outline",
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
