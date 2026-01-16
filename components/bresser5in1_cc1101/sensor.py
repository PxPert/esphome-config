import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_TEMPERATURE,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    CONF_WIND_DIRECTION_DEGREES,
    CONF_WIND_SPEED,
    CONF_HUMIDITY,
    ICON_SIGN_DIRECTION,
    ICON_WEATHER_WINDY,
    UNIT_DEGREES,
    DEVICE_CLASS_WIND_SPEED,
    DEVICE_CLASS_WIND_DIRECTION,
    DEVICE_CLASS_HUMIDITY,
    UNIT_EMPTY,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_MILLIMETER,
    DEVICE_CLASS_PRECIPITATION,
    ICON_WIFI,

)
from . import CONF_BRESSER_5_IN_1_ID, Bresser5in1Component

DEPENDENCIES = [ "bresser5in1_cc1101" ]


CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(CONF_BRESSER_5_IN_1_ID): cv.use_id(Bresser5in1Component),

        cv.Optional("rssi"): sensor.sensor_schema(
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=1,
            icon=ICON_WIFI,
            state_class=STATE_CLASS_MEASUREMENT,
        ),

        cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_WIND_SPEED): sensor.sensor_schema(
            unit_of_measurement="m/s",
            icon=ICON_WEATHER_WINDY,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_WIND_SPEED,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("wind_gusts_speed"): sensor.sensor_schema(
            unit_of_measurement="m/s",
            icon="mdi:weather-dust",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_WIND_SPEED,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_WIND_DIRECTION_DEGREES): sensor.sensor_schema(
            unit_of_measurement=UNIT_DEGREES,
            icon=ICON_SIGN_DIRECTION,
            device_class=DEVICE_CLASS_WIND_DIRECTION,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_HUMIDITY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),

        cv.Optional("rain_level"): sensor.sensor_schema(
            unit_of_measurement=UNIT_MILLIMETER,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_PRECIPITATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),

        cv.Optional("station_id"): sensor.sensor_schema(
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=1,
        ),


    })
)

async def to_code(config):
    var= await cg.get_variable(config[CONF_BRESSER_5_IN_1_ID])

    if "station_id" in config:
        conf = config["station_id"]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_station_id_sensor(sens))

    if "rssi" in config:
        conf = config["rssi"]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_rssi_sensor(sens))

    if temperature_config := config.get(CONF_TEMPERATURE):
        sens = await sensor.new_sensor(temperature_config)
        cg.add(var.set_temperature_sensor(sens))

    if CONF_HUMIDITY in config:
        sens = await sensor.new_sensor(config[CONF_HUMIDITY])
        cg.add(var.set_humidity(sens))

    if CONF_WIND_DIRECTION_DEGREES in config:
        conf = config[CONF_WIND_DIRECTION_DEGREES]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_wind_direction_degrees_sensor(sens))

    if "wind_gusts_speed" in config:
        conf = config["wind_gusts_speed"]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_wind_gusts_speed_sensor(sens))

    if CONF_WIND_SPEED in config:
        conf = config[CONF_WIND_SPEED]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_wind_speed_sensor(sens))

    if "rain_level" in config:
        conf = config["rain_level"]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_rain_level_sensor(sens))
