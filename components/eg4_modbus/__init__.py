import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import uart
from esphome.const import CONF_FLOW_CONTROL_PIN, CONF_ID

CODEOWNERS = ["@rar"]
DEPENDENCIES = ["uart"]
MULTI_CONF = True

CONF_EG4_MODBUS_ID = "eg4_modbus_id"
CONF_TURNAROUND_TIME = "turnaround_time"

eg4_modbus_ns = cg.esphome_ns.namespace("eg4_modbus")
EG4Modbus = eg4_modbus_ns.class_("EG4Modbus", cg.Component, uart.UARTDevice)
EG4ModbusDevice = eg4_modbus_ns.class_("EG4ModbusDevice")

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EG4Modbus),
            cv.Optional(CONF_FLOW_CONTROL_PIN): pins.gpio_output_pin_schema,
            # Quiet time after a reply before the next request goes out
            cv.Optional(
                CONF_TURNAROUND_TIME, default="50ms"
            ): cv.All(cv.positive_time_period_milliseconds, cv.Range(max=cv.TimePeriod(milliseconds=1000))),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_turnaround_time(config[CONF_TURNAROUND_TIME]))

    if CONF_FLOW_CONTROL_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_FLOW_CONTROL_PIN])
        cg.add(var.set_flow_control_pin(pin))


def eg4_modbus_device_schema(default_address):
    schema = {
        cv.GenerateID(CONF_EG4_MODBUS_ID): cv.use_id(EG4Modbus),
    }
    # 0x00 is the Modbus broadcast address and 0xF8-0xFF are reserved, so
    # neither can ever address a single BMS.
    schema[cv.Optional("address", default=default_address)] = cv.All(
        cv.hex_uint8_t, cv.Range(min=0x01, max=0xF7)
    )
    return cv.Schema(schema)


async def register_eg4_modbus_device(var, config):
    parent = await cg.get_variable(config[CONF_EG4_MODBUS_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_address(config["address"]))
    cg.add(parent.register_device(var))
