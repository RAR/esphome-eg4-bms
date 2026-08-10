import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv

from . import EG4_BMS_COMPONENT_SCHEMA, CONF_EG4_BMS_ID

DEPENDENCIES = ["eg4_bms"]

CONF_ONLINE_STATUS = "online_status"
CONF_HEATING = "heating"
CONF_CHARGING = "charging"
CONF_DISCHARGING = "discharging"

CONFIG_SCHEMA = EG4_BMS_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_ONLINE_STATUS): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_HEATING): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_CHARGING): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_DISCHARGING): binary_sensor.binary_sensor_schema(),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_EG4_BMS_ID])

    if CONF_ONLINE_STATUS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ONLINE_STATUS])
        cg.add(hub.set_online_status_binary_sensor(sens))

    if CONF_HEATING in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_HEATING])
        cg.add(hub.set_heating_binary_sensor(sens))

    if CONF_CHARGING in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CHARGING])
        cg.add(hub.set_charging_binary_sensor(sens))

    if CONF_DISCHARGING in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_DISCHARGING])
        cg.add(hub.set_discharging_binary_sensor(sens))
