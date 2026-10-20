import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_CONFIG

from .. import CONF_EG4_BMS_ID, EG4_BMS_COMPONENT_SCHEMA, eg4_bms_ns

DEPENDENCIES = ["eg4_bms"]

# Arms settings writes (numbers and the clock button). Always starts off, so a
# reboot cannot leave the BMS writable; restore_mode is ignored.
CONF_ALLOW_SETTINGS_WRITES = "allow_settings_writes"

EG4WriteEnableSwitch = eg4_bms_ns.class_("EG4WriteEnableSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = EG4_BMS_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_ALLOW_SETTINGS_WRITES): switch.switch_schema(
            EG4WriteEnableSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="ALWAYS_OFF",
            block_inverted=True,
        ).extend(cv.COMPONENT_SCHEMA),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_EG4_BMS_ID])
    if CONF_ALLOW_SETTINGS_WRITES in config:
        var = await switch.new_switch(config[CONF_ALLOW_SETTINGS_WRITES])
        await cg.register_component(var, config[CONF_ALLOW_SETTINGS_WRITES])
        cg.add(var.set_parent(hub))
