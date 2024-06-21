import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart
import esphome.const as espconst

DEPENDENCIES = ["uart"]

CONF_ON_DESK_EVENT = "on_desk_event"

ufo_desk_ns = cg.esphome_ns.namespace("ufo_desk")
UfoDesk = ufo_desk_ns.class_("UfoDesk", cg.PollingComponent, uart.UARTDevice)

UfoDeskTrigger = ufo_desk_ns.class_("UfoDeskTrigger", automation.Trigger.template())
UfoDeskEvent = ufo_desk_ns.class_("UfoDeskEvent")
UfoDeskEventType = ufo_desk_ns.class_("UfoDeskEventType")

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(UfoDesk),
            cv.Optional(CONF_ON_DESK_EVENT): automation.validate_automation(
                {
                    cv.GenerateID(espconst.CONF_TRIGGER_ID): cv.declare_id(
                        UfoDeskTrigger
                    ),
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


def to_code(config):
    the_desk = cg.new_Pvariable(config[espconst.CONF_ID])
    yield cg.register_component(the_desk, config)
    yield uart.register_uart_device(the_desk, config)

    # On event trigger
    for conf in config.get(CONF_ON_DESK_EVENT, []):
        trigger = cg.new_Pvariable(conf[espconst.CONF_TRIGGER_ID], the_desk)
        yield automation.build_automation(trigger, [(UfoDeskEvent, "x")], conf)
