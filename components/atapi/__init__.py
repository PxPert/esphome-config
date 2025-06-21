import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, core
from esphome.automation import Condition, maybe_simple_id
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_TRIGGER_ID,
    CONF_ON_CLICK,
    )

DEPENDENCIES = ["i2c"]

atapi_ns = cg.esphome_ns.namespace("atapi")
Atapi = atapi_ns.class_(
    "Atapi", cg.PollingComponent, i2c.I2CDevice
)

# CONFIG_SCHEMA = (
#     cv.Schema({cv.GenerateID(): cv.declare_id(Atapi)})
#     .extend(cv.polling_component_schema("1s"))
#     .extend(i2c.i2c_device_schema(0x01))
#
# )

# Triggers
ClickTrigger = atapi_ns.class_("ClickTrigger", automation.Trigger.template())


CONFIG_SCHEMA = (
        cv.polling_component_schema("1s")
        .extend(i2c.i2c_device_schema(0x01))
        .extend(
        {
            cv.GenerateID(): cv.declare_id(Atapi)
            ,
            cv.Optional(CONF_ON_CLICK): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ClickTrigger),
                }
            )
        })
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
