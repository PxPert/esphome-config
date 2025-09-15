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
AUTO_LOAD = [ "binary_sensor", "sensor"]

CONF_BRESSER_5_IN_1_ID = "bresser5in1_cc1101_id"


bresser5in1_cc1101_ns = cg.esphome_ns.namespace("bresser5in1_cc1101")
Bresser5in1Component = bresser5in1_cc1101_ns.class_(
    "Bresser5in1CC1101Component", cg.Component, spi.SPIDevice
)

# Triggers
StateTrigger = bresser5in1_cc1101_ns.class_("StateTrigger", automation.Trigger.template())

BresserReading = bresser5in1_cc1101_ns.class_("BresserReading")
BresserReadingConstPtr = BresserReading.operator("ptr").operator("const")

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(Bresser5in1Component),
        cv.Required(CONF_RX_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_ON_STATE): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(StateTrigger),
            }
        ),

        })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    pin = await gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(pin))

    for conf in config.get(CONF_ON_STATE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(BresserReadingConstPtr, "x")], conf)

    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
