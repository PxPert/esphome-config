from esphome.components import light
import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.automation as automation
from esphome.const import CONF_OUTPUT_ID
from .. import CONF_ANSLUTA_ID, AnslutaCC2500Component, ansluta_cc2500_ns

DEPENDENCIES = ['ansluta_cc2500']

AnslutaLightOutput = ansluta_cc2500_ns.class_('AnslutaCC2500Light', light.LightOutput, cg.Component)

CONFIG_SCHEMA = cv.All(light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend({
    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(AnslutaLightOutput),
    cv.GenerateID(CONF_ANSLUTA_ID): cv.use_id(AnslutaCC2500Component),
}).extend(cv.COMPONENT_SCHEMA))


def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    yield cg.register_component(var, config)
    yield light.register_light(var, config)

    par = yield cg.get_variable(config[CONF_ANSLUTA_ID])
    cg.add(var.set_parent(par))
