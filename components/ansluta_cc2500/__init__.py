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


ansluta_cc2500_ns = cg.esphome_ns.namespace("ansluta_cc2500")
AnslutaCC2500Component = ansluta_cc2500_ns.class_(
    "AnslutaCC2500Component", cg.Component, spi.SPIDevice
)

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(AnslutaCC2500Component),
        })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
