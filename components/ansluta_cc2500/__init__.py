import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome import pins, automation, core
from esphome.automation import Condition, maybe_simple_id
from esphome.const import *
from esphome.cpp_helpers import gpio_pin_expression
from esphome.core import CORE
import os

DEPENDENCIES = ["spi"]

CONF_ANSLUTA_ID = "ansluta_cc2500_id"

CONF_ON_REMOTE_CLICK = 'on_remote_click'

ansluta_cc2500_ns = cg.esphome_ns.namespace("ansluta_cc2500")
AnslutaCC2500Component = ansluta_cc2500_ns.class_(
    "AnslutaCC2500Component", cg.Component, spi.SPIDevice
)

OnRemoteClickTrigger = ansluta_cc2500_ns.class_("OnRemoteClickTrigger", automation.Trigger.template())

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(AnslutaCC2500Component),
        cv.Optional(CONF_ON_REMOTE_CLICK): automation.validate_automation({
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OnRemoteClickTrigger),
            cv.Optional(CONF_DEBOUNCE, default=200): cv.uint16_t,
        }),

        })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    for conf in config.get(CONF_ON_REMOTE_CLICK, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        if CONF_DEBOUNCE in conf:
            cg.add(trigger.set_debounce(conf[CONF_DEBOUNCE]))
        await automation.build_automation(trigger, [(cg.uint8, "x")], conf)

    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
