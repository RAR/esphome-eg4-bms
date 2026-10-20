import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import EG4_BMS_COMPONENT_SCHEMA, CONF_EG4_BMS_ID

DEPENDENCIES = ["eg4_bms"]

CONF_STATUS = "status"
CONF_WARNINGS = "warnings"
CONF_PROTECTION = "protection"
CONF_ERROR = "error"
CONF_MODEL = "model"
CONF_FIRMWARE_VERSION = "firmware_version"
CONF_SERIAL_NUMBER = "serial_number"
# BMS clock from the parameter block (RS232 service port); reading it enables
# the parameter block poll.
CONF_BMS_TIME = "bms_time"

CONFIG_SCHEMA = EG4_BMS_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_STATUS): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_WARNINGS): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_PROTECTION): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_ERROR): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_MODEL): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_FIRMWARE_VERSION): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_SERIAL_NUMBER): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_BMS_TIME): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_EG4_BMS_ID])

    if CONF_STATUS in config:
        sens = await text_sensor.new_text_sensor(config[CONF_STATUS])
        cg.add(hub.set_status_text_sensor(sens))

    if CONF_WARNINGS in config:
        sens = await text_sensor.new_text_sensor(config[CONF_WARNINGS])
        cg.add(hub.set_warnings_text_sensor(sens))

    if CONF_PROTECTION in config:
        sens = await text_sensor.new_text_sensor(config[CONF_PROTECTION])
        cg.add(hub.set_protection_text_sensor(sens))

    if CONF_ERROR in config:
        sens = await text_sensor.new_text_sensor(config[CONF_ERROR])
        cg.add(hub.set_error_text_sensor(sens))

    if CONF_MODEL in config:
        sens = await text_sensor.new_text_sensor(config[CONF_MODEL])
        cg.add(hub.set_model_text_sensor(sens))

    if CONF_FIRMWARE_VERSION in config:
        sens = await text_sensor.new_text_sensor(config[CONF_FIRMWARE_VERSION])
        cg.add(hub.set_firmware_version_text_sensor(sens))

    if CONF_SERIAL_NUMBER in config:
        sens = await text_sensor.new_text_sensor(config[CONF_SERIAL_NUMBER])
        cg.add(hub.set_serial_number_text_sensor(sens))

    if CONF_BMS_TIME in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BMS_TIME])
        cg.add(hub.set_bms_time_text_sensor(sens))
        cg.add(hub.set_read_settings(True))
