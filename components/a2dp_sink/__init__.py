import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_NAME
)

from esphome import automation

CODEOWNERS = ["@PxPert"]
# DEPENDENCIES = ['uart']

# CONFIG-IDs
CONF_A2DP_SINK_ID = "a2dp_sink_id" # Used by child components

CONF_A2DP_AUTO_RECONNECT = "auto_reconnect"



# ------------------------------
# ------------------------------

a2dp_sink_ns = cg.esphome_ns.namespace('a2dp_sink')


A2DPSinkHub = a2dp_sink_ns.class_(
    'A2DPSinkHub',
    cg.Component,
)


# ------------------------------
#  Parameter Config
# ------------------------------
CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(A2DPSinkHub),
            cv.Required(CONF_NAME): cv.string,
            cv.Optional(CONF_A2DP_AUTO_RECONNECT, default=True): cv.boolean,

        }
    ),
    cv.only_on_esp32,
)


# ------------------------------
#  Actions
# ------------------------------

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_name(config[CONF_NAME]))
    cg.add(var.set_auto_reconnect(config[CONF_A2DP_AUTO_RECONNECT]))


