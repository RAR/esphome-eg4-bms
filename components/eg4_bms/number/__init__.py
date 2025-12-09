import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_MODE,
    CONF_STEP,
    CONF_UNIT_OF_MEASUREMENT,
    ENTITY_CATEGORY_CONFIG,
    ICON_EMPTY,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_SECOND,
    UNIT_VOLT,
)

from .. import CONF_EG4_BMS_ID, EG4_BMS_COMPONENT_SCHEMA, eg4_bms_ns

DEPENDENCIES = ["eg4_bms"]

CODEOWNERS = ["@syssi"]

# Balancing Configuration
CONF_BALANCE_STARTING_VOLTAGE = "balance_starting_voltage"
CONF_BALANCE_VOLTAGE_DIFFERENCE = "balance_voltage_difference"
CONF_LOW_CAPACITY_WARNING = "low_capacity_warning"

# Cell Voltage Thresholds
CONF_CELL_UNDERVOLTAGE_WARNING = "cell_undervoltage_warning"
CONF_CELL_UNDERVOLTAGE_PROTECTION = "cell_undervoltage_protection"
CONF_CELL_UNDERVOLTAGE_RELEASE = "cell_undervoltage_release"
CONF_CELL_OVERVOLTAGE_WARNING = "cell_overvoltage_warning"
CONF_CELL_OVERVOLTAGE_PROTECTION = "cell_overvoltage_protection"
CONF_CELL_OVERVOLTAGE_RELEASE = "cell_overvoltage_release"

# Pack Voltage Thresholds
CONF_PACK_UNDERVOLTAGE_WARNING = "pack_undervoltage_warning"
CONF_PACK_UNDERVOLTAGE_PROTECTION = "pack_undervoltage_protection"
CONF_PACK_UNDERVOLTAGE_RELEASE = "pack_undervoltage_release"
CONF_PACK_OVERVOLTAGE_WARNING = "pack_overvoltage_warning"
CONF_PACK_OVERVOLTAGE_PROTECTION = "pack_overvoltage_protection"
CONF_PACK_OVERVOLTAGE_RELEASE = "pack_overvoltage_release"

# Charge Over-Current Protection
CONF_CHARGE_OC1_PROTECTION = "charge_overcurrent_1_protection"
CONF_CHARGE_OC1_DELAY = "charge_overcurrent_1_delay"
CONF_CHARGE_OC2_PROTECTION = "charge_overcurrent_2_protection"
CONF_CHARGE_OC2_DELAY = "charge_overcurrent_2_delay"
CONF_CHARGE_OC_RELEASE_DELAY = "charge_overcurrent_release_delay"
CONF_CHARGE_OC_TIMES = "charge_overcurrent_times"

# Discharge Over-Current Protection
CONF_DISCHARGE_OC1_PROTECTION = "discharge_overcurrent_1_protection"
CONF_DISCHARGE_OC1_DELAY = "discharge_overcurrent_1_delay"
CONF_DISCHARGE_OC2_PROTECTION = "discharge_overcurrent_2_protection"
CONF_DISCHARGE_OC2_DELAY = "discharge_overcurrent_2_delay"
CONF_DISCHARGE_OC_RELEASE_DELAY = "discharge_overcurrent_release_delay"
CONF_DISCHARGE_OC_TIMES = "discharge_overcurrent_times"
CONF_LOAD_SHORT_CURRENT = "load_short_current"

# Temperature Protection - Charge Undertemp
CONF_CHARGE_UNDERTEMP_WARNING = "charge_undertemperature_warning"
CONF_CHARGE_UNDERTEMP_PROTECTION = "charge_undertemperature_protection"
CONF_CHARGE_UNDERTEMP_RELEASE = "charge_undertemperature_release"

# Temperature Protection - Charge Overtemp
CONF_CHARGE_OVERTEMP_WARNING = "charge_overtemperature_warning"
CONF_CHARGE_OVERTEMP_PROTECTION = "charge_overtemperature_protection"
CONF_CHARGE_OVERTEMP_RELEASE = "charge_overtemperature_release"

# Temperature Protection - Discharge Undertemp
CONF_DISCHARGE_UNDERTEMP_WARNING = "discharge_undertemperature_warning"
CONF_DISCHARGE_UNDERTEMP_PROTECTION = "discharge_undertemperature_protection"
CONF_DISCHARGE_UNDERTEMP_RELEASE = "discharge_undertemperature_release"

# Temperature Protection - Discharge Overtemp
CONF_DISCHARGE_OVERTEMP_WARNING = "discharge_overtemperature_warning"
CONF_DISCHARGE_OVERTEMP_PROTECTION = "discharge_overtemperature_protection"
CONF_DISCHARGE_OVERTEMP_RELEASE = "discharge_overtemperature_release"

# Temperature Protection - PCB Overtemp
CONF_PCB_OVERTEMP_WARNING = "pcb_overtemperature_warning"
CONF_PCB_OVERTEMP_PROTECTION = "pcb_overtemperature_protection"
CONF_PCB_OVERTEMP_RELEASE = "pcb_overtemperature_release"

# Temperature Protection - Ambient Undertemp
CONF_AMBIENT_UNDERTEMP_WARNING = "ambient_undertemperature_warning"
CONF_AMBIENT_UNDERTEMP_PROTECTION = "ambient_undertemperature_protection"
CONF_AMBIENT_UNDERTEMP_RELEASE = "ambient_undertemperature_release"

# Temperature Protection - Ambient Overtemp
CONF_AMBIENT_OVERTEMP_WARNING = "ambient_overtemperature_warning"
CONF_AMBIENT_OVERTEMP_PROTECTION = "ambient_overtemperature_protection"
CONF_AMBIENT_OVERTEMP_RELEASE = "ambient_overtemperature_release"

# Heating Control
CONF_HEATING_ON_TEMPERATURE = "heating_on_temperature"
CONF_HEATING_OFF_TEMPERATURE = "heating_off_temperature"

UNIT_AMPERE_HOUR = "Ah"

# Register addresses mapped to configuration parameter names
# Format: register_address: [factor, unit_of_measurement]
# factor: multiply user value by this to get register value
NUMBERS = {
    # Balancing (mV units)
    CONF_BALANCE_STARTING_VOLTAGE: [0x0038, 1000.0, UNIT_VOLT],
    CONF_BALANCE_VOLTAGE_DIFFERENCE: [0x0039, 1000.0, UNIT_VOLT],
    CONF_LOW_CAPACITY_WARNING: [0x003A, 1.0, UNIT_AMPERE_HOUR],
    
    # Cell Voltage Thresholds (mV units)
    CONF_CELL_UNDERVOLTAGE_WARNING: [0x003D, 1000.0, UNIT_VOLT],
    CONF_CELL_UNDERVOLTAGE_PROTECTION: [0x003E, 1000.0, UNIT_VOLT],
    CONF_CELL_UNDERVOLTAGE_RELEASE: [0x003F, 1000.0, UNIT_VOLT],
    CONF_CELL_OVERVOLTAGE_WARNING: [0x0043, 1000.0, UNIT_VOLT],
    CONF_CELL_OVERVOLTAGE_PROTECTION: [0x0044, 1000.0, UNIT_VOLT],
    CONF_CELL_OVERVOLTAGE_RELEASE: [0x0045, 1000.0, UNIT_VOLT],
    
    # Pack Voltage Thresholds (0.1V units)
    CONF_PACK_UNDERVOLTAGE_WARNING: [0x0040, 10.0, UNIT_VOLT],
    CONF_PACK_UNDERVOLTAGE_PROTECTION: [0x0041, 10.0, UNIT_VOLT],
    CONF_PACK_UNDERVOLTAGE_RELEASE: [0x0042, 10.0, UNIT_VOLT],
    CONF_PACK_OVERVOLTAGE_WARNING: [0x0046, 10.0, UNIT_VOLT],
    CONF_PACK_OVERVOLTAGE_PROTECTION: [0x0047, 10.0, UNIT_VOLT],
    CONF_PACK_OVERVOLTAGE_RELEASE: [0x0048, 10.0, UNIT_VOLT],
    
    # Charge Over-Current (0.01A units)
    CONF_CHARGE_OC1_PROTECTION: [0x0050, 100.0, UNIT_AMPERE],
    CONF_CHARGE_OC1_DELAY: [0x0055, 1.0, UNIT_SECOND],
    CONF_CHARGE_OC2_PROTECTION: [0x0053, 100.0, UNIT_AMPERE],
    CONF_CHARGE_OC2_DELAY: [0x0056, 1.0, UNIT_SECOND],
    CONF_CHARGE_OC_RELEASE_DELAY: [0x004E, 1.0, UNIT_SECOND],
    CONF_CHARGE_OC_TIMES: [0x004C, 1.0, ""],
    
    # Discharge Over-Current (0.01A units)
    CONF_DISCHARGE_OC1_PROTECTION: [0x0051, 100.0, UNIT_AMPERE],
    CONF_DISCHARGE_OC1_DELAY: [0x0057, 1.0, UNIT_SECOND],
    CONF_DISCHARGE_OC2_PROTECTION: [0x0052, 100.0, UNIT_AMPERE],
    CONF_DISCHARGE_OC2_DELAY: [0x0058, 1.0, UNIT_SECOND],
    CONF_DISCHARGE_OC_RELEASE_DELAY: [0x004F, 1.0, UNIT_SECOND],
    CONF_DISCHARGE_OC_TIMES: [0x004D, 1.0, ""],
    CONF_LOAD_SHORT_CURRENT: [0x0054, 100.0, UNIT_AMPERE],
    
    # Temperature Protection - Charge Undertemp (+50 offset, stored as byte)
    CONF_CHARGE_UNDERTEMP_WARNING: [0x005A, 1.0, UNIT_CELSIUS],
    CONF_CHARGE_UNDERTEMP_PROTECTION: [0x005B, 1.0, UNIT_CELSIUS],
    CONF_CHARGE_UNDERTEMP_RELEASE: [0x005C, 1.0, UNIT_CELSIUS],
    
    # Temperature Protection - Charge Overtemp (+50 offset, stored as byte)
    CONF_CHARGE_OVERTEMP_WARNING: [0x005D, 1.0, UNIT_CELSIUS],
    CONF_CHARGE_OVERTEMP_PROTECTION: [0x005E, 1.0, UNIT_CELSIUS],
    CONF_CHARGE_OVERTEMP_RELEASE: [0x005F, 1.0, UNIT_CELSIUS],
    
    # Temperature Protection - Discharge Undertemp (+50 offset, stored as byte)
    CONF_DISCHARGE_UNDERTEMP_WARNING: [0x0060, 1.0, UNIT_CELSIUS],
    CONF_DISCHARGE_UNDERTEMP_PROTECTION: [0x0061, 1.0, UNIT_CELSIUS],
    CONF_DISCHARGE_UNDERTEMP_RELEASE: [0x0062, 1.0, UNIT_CELSIUS],
    
    # Temperature Protection - Discharge Overtemp (+50 offset, stored as byte)
    CONF_DISCHARGE_OVERTEMP_WARNING: [0x0063, 1.0, UNIT_CELSIUS],
    CONF_DISCHARGE_OVERTEMP_PROTECTION: [0x0064, 1.0, UNIT_CELSIUS],
    CONF_DISCHARGE_OVERTEMP_RELEASE: [0x0065, 1.0, UNIT_CELSIUS],
    
    # Temperature Protection - PCB Overtemp (+50 offset, stored as byte)
    CONF_PCB_OVERTEMP_WARNING: [0x0066, 1.0, UNIT_CELSIUS],
    CONF_PCB_OVERTEMP_PROTECTION: [0x0067, 1.0, UNIT_CELSIUS],
    CONF_PCB_OVERTEMP_RELEASE: [0x0068, 1.0, UNIT_CELSIUS],
    
    # Heating Control (+50 offset, stored as byte)
    CONF_HEATING_ON_TEMPERATURE: [0x0080, 1.0, UNIT_CELSIUS],
    CONF_HEATING_OFF_TEMPERATURE: [0x0081, 1.0, UNIT_CELSIUS],
    
    # Temperature Protection - Ambient Undertemp (+50 offset, stored as byte)
    CONF_AMBIENT_UNDERTEMP_WARNING: [0x0082, 1.0, UNIT_CELSIUS],
    CONF_AMBIENT_UNDERTEMP_PROTECTION: [0x0083, 1.0, UNIT_CELSIUS],
    CONF_AMBIENT_UNDERTEMP_RELEASE: [0x0084, 1.0, UNIT_CELSIUS],
    
    # Temperature Protection - Ambient Overtemp (+50 offset, stored as byte)
    CONF_AMBIENT_OVERTEMP_WARNING: [0x0085, 1.0, UNIT_CELSIUS],
    CONF_AMBIENT_OVERTEMP_PROTECTION: [0x0086, 1.0, UNIT_CELSIUS],
    CONF_AMBIENT_OVERTEMP_RELEASE: [0x0087, 1.0, UNIT_CELSIUS],
}

Eg4Number = eg4_bms_ns.class_("Eg4Number", number.Number, cg.Component)

EG4_NUMBER_SCHEMA = (
    number.number_schema(
        Eg4Number,
        icon=ICON_EMPTY,
        entity_category=ENTITY_CATEGORY_CONFIG,
    )
    .extend(
        {
            cv.Optional(CONF_MODE, default="BOX"): cv.enum(
                number.NUMBER_MODES, upper=True
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

CONFIG_SCHEMA = EG4_BMS_COMPONENT_SCHEMA.extend(
    {
        # Balancing Configuration
        cv.Optional(CONF_BALANCE_STARTING_VOLTAGE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=2.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=4.5): cv.float_,
                cv.Optional(CONF_STEP, default=0.001): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_BALANCE_VOLTAGE_DIFFERENCE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=0.001): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=0.500): cv.float_,
                cv.Optional(CONF_STEP, default=0.001): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_LOW_CAPACITY_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_AMPERE_HOUR): cv.string_strict,
            }
        ),
        
        # Cell Voltage Thresholds
        cv.Optional(CONF_CELL_UNDERVOLTAGE_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=2.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=3.65): cv.float_,
                cv.Optional(CONF_STEP, default=0.001): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CELL_UNDERVOLTAGE_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=2.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=3.65): cv.float_,
                cv.Optional(CONF_STEP, default=0.001): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CELL_UNDERVOLTAGE_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=2.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=3.65): cv.float_,
                cv.Optional(CONF_STEP, default=0.001): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CELL_OVERVOLTAGE_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=2.5): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=4.35): cv.float_,
                cv.Optional(CONF_STEP, default=0.001): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CELL_OVERVOLTAGE_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=2.5): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=4.35): cv.float_,
                cv.Optional(CONF_STEP, default=0.001): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CELL_OVERVOLTAGE_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=2.5): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=4.35): cv.float_,
                cv.Optional(CONF_STEP, default=0.001): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        
        # Pack Voltage Thresholds
        cv.Optional(CONF_PACK_UNDERVOLTAGE_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=65.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_PACK_UNDERVOLTAGE_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=65.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_PACK_UNDERVOLTAGE_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=65.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_PACK_OVERVOLTAGE_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=40.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=70.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_PACK_OVERVOLTAGE_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=40.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=70.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        cv.Optional(CONF_PACK_OVERVOLTAGE_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=40.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=70.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_VOLT): cv.string_strict,
            }
        ),
        
        # Charge Over-Current Protection
        cv.Optional(CONF_CHARGE_OC1_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=500.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_AMPERE): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CHARGE_OC1_DELAY): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=300): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_SECOND): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CHARGE_OC2_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=500.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_AMPERE): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CHARGE_OC2_DELAY): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=300): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_SECOND): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CHARGE_OC_RELEASE_DELAY): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=600): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_SECOND): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CHARGE_OC_TIMES): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=20): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=""): cv.string_strict,
            }
        ),
        
        # Discharge Over-Current Protection
        cv.Optional(CONF_DISCHARGE_OC1_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=500.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_AMPERE): cv.string_strict,
            }
        ),
        cv.Optional(CONF_DISCHARGE_OC1_DELAY): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=300): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_SECOND): cv.string_strict,
            }
        ),
        cv.Optional(CONF_DISCHARGE_OC2_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=500.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_AMPERE): cv.string_strict,
            }
        ),
        cv.Optional(CONF_DISCHARGE_OC2_DELAY): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=300): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_SECOND): cv.string_strict,
            }
        ),
        cv.Optional(CONF_DISCHARGE_OC_RELEASE_DELAY): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=600): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_SECOND): cv.string_strict,
            }
        ),
        cv.Optional(CONF_DISCHARGE_OC_TIMES): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=20): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=""): cv.string_strict,
            }
        ),
        cv.Optional(CONF_LOAD_SHORT_CURRENT): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=1.0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=500.0): cv.float_,
                cv.Optional(CONF_STEP, default=0.1): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_AMPERE): cv.string_strict,
            }
        ),
        
        # Temperature Protection - Charge Undertemp
        cv.Optional(CONF_CHARGE_UNDERTEMP_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CHARGE_UNDERTEMP_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CHARGE_UNDERTEMP_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        
        # Temperature Protection - Charge Overtemp
        cv.Optional(CONF_CHARGE_OVERTEMP_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CHARGE_OVERTEMP_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_CHARGE_OVERTEMP_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        
        # Temperature Protection - Discharge Undertemp
        cv.Optional(CONF_DISCHARGE_UNDERTEMP_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_DISCHARGE_UNDERTEMP_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_DISCHARGE_UNDERTEMP_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        
        # Temperature Protection - Discharge Overtemp
        cv.Optional(CONF_DISCHARGE_OVERTEMP_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_DISCHARGE_OVERTEMP_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_DISCHARGE_OVERTEMP_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        
        # Temperature Protection - PCB Overtemp
        cv.Optional(CONF_PCB_OVERTEMP_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=120): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_PCB_OVERTEMP_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=120): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_PCB_OVERTEMP_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=120): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        
        # Heating Control
        cv.Optional(CONF_HEATING_ON_TEMPERATURE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_HEATING_OFF_TEMPERATURE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        
        # Temperature Protection - Ambient Undertemp
        cv.Optional(CONF_AMBIENT_UNDERTEMP_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_AMBIENT_UNDERTEMP_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_AMBIENT_UNDERTEMP_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=-40): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=25): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        
        # Temperature Protection - Ambient Overtemp
        cv.Optional(CONF_AMBIENT_OVERTEMP_WARNING): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_AMBIENT_OVERTEMP_PROTECTION): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
        cv.Optional(CONF_AMBIENT_OVERTEMP_RELEASE): EG4_NUMBER_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=30): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1.0): cv.float_,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_CELSIUS): cv.string_strict,
            }
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_EG4_BMS_ID])
    for key, address_config in NUMBERS.items():
        if key in config:
            conf = config[key]
            var = cg.new_Pvariable(conf[CONF_ID])
            await cg.register_component(var, conf)
            await number.register_number(
                var,
                conf,
                min_value=conf[CONF_MIN_VALUE],
                max_value=conf[CONF_MAX_VALUE],
                step=conf[CONF_STEP],
            )
            cg.add(getattr(hub, f"set_{key}_number")(var))
            cg.add(var.set_parent(hub))
            cg.add(var.set_holding_register(address_config[0]))
            cg.add(var.set_factor(address_config[1]))
