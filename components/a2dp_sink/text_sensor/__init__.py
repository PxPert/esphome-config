import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TYPE
from esphome.types import ConfigType


from .. import (
    CONF_A2DP_SINK_ID,
    A2DPSinkHub,
    a2dp_sink_ns,
)


CODEOWNERS = ["@PxPert"]

A2DPSinkTextSensor = a2dp_sink_ns.class_(
    "A2DPSinkTextSensor",
    text_sensor.TextSensor,
    cg.Component,
)

A2DPSinkTextMetadataTypes = a2dp_sink_ns.enum("A2DPSinkTextMetadataTypes", is_class=True)
A2DPSINK_TEXT_METADATA_TYPES = {
    "title": A2DPSinkTextMetadataTypes.TITLE,
    "artist": A2DPSinkTextMetadataTypes.ARTIST,
    "album": A2DPSinkTextMetadataTypes.ALBUM,
    "genre": A2DPSinkTextMetadataTypes.GENRE,
}


CONFIG_SCHEMA = cv.All(
    text_sensor.text_sensor_schema().extend(
        {
            cv.GenerateID(): cv.declare_id(A2DPSinkTextSensor),
            cv.GenerateID(CONF_A2DP_SINK_ID): cv.use_id(A2DPSinkHub),
            cv.Required(CONF_TYPE): cv.enum(A2DPSINK_TEXT_METADATA_TYPES),
        }
    ),
    cv.only_on_esp32,
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_A2DP_SINK_ID])
    await text_sensor.register_text_sensor(var, config)

    cg.add(var.set_metadata_type(config[CONF_TYPE]))
