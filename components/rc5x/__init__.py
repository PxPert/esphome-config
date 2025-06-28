import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, core, pins
from esphome.const import CONF_PIN
from esphome.const import (
    CONF_ID,
    CONF_TRIGGER_ID,
    CONF_PIN,
    CONF_ON_MESSAGE
    )

rc5x_ns = cg.esphome_ns.namespace("rc5x")

RC5x = rc5x_ns.class_(
    "RC5x", cg.Component
)

CommandTrigger = rc5x_ns.class_("CommandTrigger", automation.Trigger.template())

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RC5x),
        cv.Optional(CONF_ON_MESSAGE): automation.validate_automation(
          {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(CommandTrigger),
          }
        ),
        cv.Required(CONF_PIN): pins.gpio_input_pin_schema,
    }
).extend(cv.COMPONENT_SCHEMA)




async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    pin = await cg.gpio_pin_expression(config[CONF_PIN])

    cg.add(var.set_pin(pin))
    for conf in config.get(CONF_ON_MESSAGE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(bool, "t"),(cg.uint32, "x")], conf)
