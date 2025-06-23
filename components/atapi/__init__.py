import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, core
from esphome.automation import Condition, maybe_simple_id
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_TRIGGER_ID,
    CONF_ON_STATE,
    CONF_ON_UPDATE,
    CONF_ON_LOCK,
    CONF_ON_ERROR,
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
StateTrigger = atapi_ns.class_("StateTrigger", automation.Trigger.template())
UpdateTrigger = atapi_ns.class_("UpdateTrigger", automation.Trigger.template())
TocTrigger = atapi_ns.class_("TocTrigger", automation.Trigger.template())
LockTrigger = atapi_ns.class_("LockTrigger", automation.Trigger.template())
ErrorTrigger = atapi_ns.class_("ErrorTrigger", automation.Trigger.template())


CONFIG_SCHEMA = (
        cv.polling_component_schema("750ms")
        .extend(i2c.i2c_device_schema(0x01))
        .extend(
        {
            cv.GenerateID(): cv.declare_id(Atapi)
            ,
            cv.Optional(CONF_ON_UPDATE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(UpdateTrigger),
                }
            ),
            cv.Optional(CONF_ON_STATE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(StateTrigger),
                }
            ),
            cv.Optional("on_toc"): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TocTrigger),
                }
            ),
            cv.Optional(CONF_ON_LOCK): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(LockTrigger),
                }
            ),
            cv.Optional(CONF_ON_ERROR): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ErrorTrigger),
                }
            ),
        })
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    for conf in config.get(CONF_ON_UPDATE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_STATE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(int, "x")], conf)
    for conf in config.get("on_toc", []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_LOCK, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(bool, "x")], conf)
    for conf in config.get(CONF_ON_ERROR, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(int, "x")], conf)
