import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import audio, media_source
from esphome.const import (
    CONF_ID,
)

from esphome import automation

AUTO_LOAD = ["audio"]
CODEOWNERS = ["@PxPert"]
# DEPENDENCIES = ['uart']

# CONFIG-IDs


# ------------------------------
# ------------------------------

a2dp_sink_ns = cg.esphome_ns.namespace('a2dp_sink')


A2DPSink = a2dp_sink_ns.class_(
    'A2DPSink',
    cg.Component,
    media_source.MediaSource,
)


# ------------------------------
#  Parameter Config
# ------------------------------
CONFIG_SCHEMA = cv.All(
    media_source.media_source_schema(
        A2DPSink,
    )
    .extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
)


# ------------------------------
#  Actions
# ------------------------------

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await media_source.register_media_source(var, config)

