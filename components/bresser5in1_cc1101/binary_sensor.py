import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_BATTERY,
    ICON_BATTERY

)
from . import CONF_BRESSER_5_IN_1_ID, Bresser5in1Component

DEPENDENCIES = [ "bresser5in1_cc1101" ]


CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(CONF_BRESSER_5_IN_1_ID): cv.use_id(Bresser5in1Component),

        cv.Optional("battery_low"): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_BATTERY,
#            icon=ICON_BATTERY,
        ),

    })
)

async def to_code(config):
    var= await cg.get_variable(config[CONF_BRESSER_5_IN_1_ID])

    if "battery_low" in config:
        conf = config["battery_low"]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_battery_sensor(sens))
