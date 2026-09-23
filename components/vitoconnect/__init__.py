import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_PROTOCOL, CONF_UPDATE_INTERVAL

CODEOWNERS = ["@dannerph"]

DEPENDENCIES = ["uart"]

MULTI_CONF = True

vitoconnect_ns = cg.esphome_ns.namespace("vitoconnect")
VitoConnect = vitoconnect_ns.class_("VitoConnect", uart.UARTDevice, cg.PollingComponent)

CONF_VITOCONNECT_ID = "vitoconnect_id"

OPTOLINK_PROTOCOL = {
    "P300": "P300",
    "KW": "KW",
    "GWG": "GWG",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(VitoConnect),
        cv.Required(CONF_PROTOCOL): cv.enum(OPTOLINK_PROTOCOL, upper=True, space="_"),
        cv.Optional(
            CONF_UPDATE_INTERVAL, default="60s"
        ): cv.positive_time_period_milliseconds,
    }
).extend(uart.UART_DEVICE_SCHEMA)

# ESDN83: ersetzt check_uart_settings() aus vitoconnect.cpp (entfaellt mit
# ESPHome 2027.3.0). Alle drei Optolink-Protokolle laufen mit 4800 8E2.
FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "vitoconnect",
    baud_rate=4800,
    require_tx=True,
    require_rx=True,
    data_bits=8,
    parity="EVEN",
    stop_bits=2,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_protocol(config[CONF_PROTOCOL]))
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
