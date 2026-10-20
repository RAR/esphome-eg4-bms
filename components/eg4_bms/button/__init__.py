import esphome.codegen as cg
from esphome.components import button, time
import esphome.config_validation as cv
from esphome.const import CONF_TIME_ID, ENTITY_CATEGORY_CONFIG

from .. import CONF_EG4_BMS_ID, EG4_BMS_COMPONENT_SCHEMA, eg4_bms_ns

DEPENDENCIES = ["eg4_bms"]

# Writes the ESP's local time to the BMS clock (0x2D-0x2E), as the BMS tool's
# time sync does. Needs the write-enable switch on.
CONF_SYNC_CLOCK = "sync_clock"

EG4SyncClockButton = eg4_bms_ns.class_("EG4SyncClockButton", button.Button)

CONFIG_SCHEMA = EG4_BMS_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_SYNC_CLOCK): button.button_schema(
            EG4SyncClockButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ).extend(
            {
                cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            }
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_EG4_BMS_ID])
    if CONF_SYNC_CLOCK in config:
        conf = config[CONF_SYNC_CLOCK]
        var = await button.new_button(conf)
        cg.add(var.set_parent(hub))
        clock = await cg.get_variable(conf[CONF_TIME_ID])
        cg.add(var.set_time(clock))
        cg.add(hub.set_read_settings(True))
