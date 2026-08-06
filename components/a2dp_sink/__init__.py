from dataclasses import dataclass

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_NAME,
)
from esphome.core import CORE

from esphome import automation

CODEOWNERS = ["@PxPert"]
# DEPENDENCIES = ['uart']

DOMAIN = "a2dpsink"

# CONFIG-IDs
CONF_A2DP_SINK_ID = "a2dp_sink_id"  # Used by child components

CONF_A2DP_AUTO_RECONNECT = "auto_reconnect"


# ------------------------------
#  Namespace & Hub class
# ------------------------------

a2dp_sink_ns = cg.esphome_ns.namespace('a2dp_sink')


A2DPSinkHub = a2dp_sink_ns.class_(
    'A2DPSinkHub',
    cg.Component,
)


# ------------------------------
#  Feature flags — subcomponents call request_*_support() to opt in
# ------------------------------

@dataclass
class A2dpSinkConfiguration:
    position_support: bool = False
    rssi_support: bool = False
    metadata_support: bool = False
    peer_name_support: bool = False
    volume_support: bool = False
    playback_status_support: bool = False
    connection_state_support: bool = False


def _get_data() -> A2dpSinkConfiguration:
    if DOMAIN not in CORE.data:
        CORE.data[DOMAIN] = A2dpSinkConfiguration()
    return CORE.data[DOMAIN]

def request_position_support() -> None:
    """Request track position support for A2DP Sink."""
    _get_data().position_support = True

def request_rssi_support() -> None:
    """Request rssi signal quality support for A2DP Sink."""
    _get_data().rssi_support = True

def request_metadata_support() -> None:
    """Request metadata support for A2DP Sink."""
    _get_data().metadata_support = True

def request_peer_name_support() -> None:
    """Request peer name support for A2DP Sink."""
    _get_data().peer_name_support = True

def request_volume_support() -> None:
    """Request volume support for A2DP Sink."""
    _get_data().volume_support = True

def request_playback_status_support() -> None:
    """Request playback status support for A2DP Sink."""
    _get_data().playback_status_support = True

def request_connection_state_support() -> None:
    """Request connection state notifications support for A2DP Sink."""
    _get_data().connection_state_support = True



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

    # Emit USE_A2DP_* defines so the hub C++ can conditionally compile
    # callback infrastructure only for features the user actually needs.
    data = _get_data()
    if data.position_support:
        cg.add_define("USE_A2DP_POS", True)
    if data.rssi_support:
        cg.add_define("USE_A2DP_RSSI", True)
    if data.metadata_support:
        cg.add_define("USE_A2DP_METADATA", True)
    if data.peer_name_support:
        cg.add_define("USE_A2DP_PEER_NAME", True)
    if data.volume_support:
        cg.add_define("USE_A2DP_VOLUME", True)
    if data.playback_status_support:
        cg.add_define("USE_A2DP_PLAYBACK_STATUS", True)
    if data.connection_state_support:
        cg.add_define("USE_A2DP_CONNECTION_STATE", True)


