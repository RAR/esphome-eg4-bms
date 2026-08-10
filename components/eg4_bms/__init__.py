import esphome.codegen as cg
from esphome.components import eg4_modbus
import esphome.config_validation as cv
from esphome.const import CONF_ID

AUTO_LOAD = ["eg4_modbus", "binary_sensor", "sensor", "text_sensor"]
CODEOWNERS = ["@rar"]
MULTI_CONF = True

CONF_EG4_BMS_ID = "eg4_bms_id"
CONF_MAX_NO_RESPONSE_COUNT = "max_no_response_count"

DEFAULT_ADDRESS = 0x10
DEFAULT_MAX_NO_RESPONSE_COUNT = 5

eg4_bms_ns = cg.esphome_ns.namespace("eg4_bms")
EG4Bms = eg4_bms_ns.class_("EG4Bms", cg.PollingComponent, eg4_modbus.EG4ModbusDevice)

EG4_BMS_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_EG4_BMS_ID): cv.use_id(EG4Bms),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EG4Bms),
            cv.Optional(
                CONF_MAX_NO_RESPONSE_COUNT, default=DEFAULT_MAX_NO_RESPONSE_COUNT
            ): cv.int_range(min=1, max=255),
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(eg4_modbus.eg4_modbus_device_schema(DEFAULT_ADDRESS))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await eg4_modbus.register_eg4_modbus_device(var, config)
    cg.add(var.set_max_no_response_count(config[CONF_MAX_NO_RESPONSE_COUNT]))
