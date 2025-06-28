import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, core, pins
from esphome.const import CONF_PIN
from esphome.const import (
    CONF_ID,
    CONF_TRIGGER_ID,
    CONF_PIN,
    CONF_ON_PRESS,
    CONF_ON_RELEASE
    )

rc5x_ns = cg.esphome_ns.namespace("rc5x")

RC5x = rc5x_ns.class_(
    "RC5x", cg.Component
)

CommandPressTrigger = rc5x_ns.class_("CommandPressTrigger", automation.Trigger.template())
CommandReleaseTrigger = rc5x_ns.class_("CommandReleaseTrigger", automation.Trigger.template())

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RC5x),
        cv.Optional(CONF_ON_PRESS): automation.validate_automation(
          {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(CommandPressTrigger),
          }
        ),
        cv.Optional(CONF_ON_RELEASE): automation.validate_automation(
          {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(CommandReleaseTrigger),
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
    for conf in config.get(CONF_ON_PRESS, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(bool, "t"),(cg.uint32, "x")], conf)
    for conf in config.get(CONF_ON_RELEASE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(bool, "t"),(cg.uint32, "x")], conf)
