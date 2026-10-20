import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_MODE, ENTITY_CATEGORY_CONFIG

from .. import CONF_EG4_BMS_ID, EG4_BMS_COMPONENT_SCHEMA, eg4_bms_ns
from ..settings_table import LIMITS, ORDER, SETTINGS

DEPENDENCIES = ["eg4_bms"]

# One number per parameter block setting (RS232 service port only). Writes
# also need the eg4_bms write-enable switch; see EG4Bms::write_setting().
EG4SettingNumber = eg4_bms_ns.class_("EG4SettingNumber", number.Number)


def setting_number_schema(unit, device_class):
    kwargs = {"entity_category": ENTITY_CATEGORY_CONFIG}
    if unit is not None:
        kwargs["unit_of_measurement"] = unit
    if device_class is not None:
        kwargs["device_class"] = device_class
    # A box, not a slider: a drag or mis-tap must not write a protection limit.
    return number.number_schema(EG4SettingNumber, **kwargs).extend(
        {cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True)}
    )


CONFIG_SCHEMA = EG4_BMS_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(key): setting_number_schema(unit, device_class)
        for key, (_, _, _, unit, device_class, _) in SETTINGS.items()
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_EG4_BMS_ID])
    any_number = False
    for key, (register, multiplier, offset, _, _, _) in SETTINGS.items():
        if key not in config:
            continue
        min_value, max_value, step = LIMITS[key]
        var = await number.new_number(
            config[key], min_value=min_value, max_value=max_value, step=step
        )
        cg.add(var.set_parent(hub))
        cg.add(var.set_register(register))
        cg.add(hub.add_setting_number(register, multiplier, offset, min_value, max_value, var))
        any_number = True

    if any_number:
        cg.add(hub.set_read_settings(True))
        for lower, upper, strict in ORDER:
            cg.add(hub.add_setting_order(SETTINGS[lower][0], SETTINGS[upper][0], strict))
