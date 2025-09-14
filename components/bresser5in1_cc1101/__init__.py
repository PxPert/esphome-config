import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome import pins
from esphome.const import *
from esphome.cpp_helpers import gpio_pin_expression
from esphome.core import CORE
import os

DEPENDENCIES = ["spi"]

bresser5in1_cc1101_ns = cg.esphome_ns.namespace("bresser5in1_cc1101")
Bresser5in1Component = bresser5in1_cc1101_ns.class_(
    "Bresser5in1CC1101Component", cg.Component, spi.SPIDevice
)

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(Bresser5in1Component),
        cv.Optional(CONF_RX_PIN): pins.internal_gpio_input_pin_schema,
        })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    pin = await gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(pin))


    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
