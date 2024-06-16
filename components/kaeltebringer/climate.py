import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import (
    climate,
    uart
)
from esphome.const import CONF_ID

DEPENDENCIES = ["uart", "climate"]
CODEOWNERS = ["@KG3RK3N"]

kaeltebringer_ns = cg.esphome_ns.namespace("kaeltebringer")
KaeltebringerClimate = kaeltebringer_ns.class_(
    "KaeltebringerClimate", cg.PollingComponent, climate.Climate, uart.UARTDevice
)

CONFIG_SCHEMA = (
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(KaeltebringerClimate),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("60s"))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await climate.register_climate(var, config)