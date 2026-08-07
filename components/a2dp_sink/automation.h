#pragma once

#include <utility>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "a2dp_sink_hub.h"

namespace esphome {
namespace a2dp_sink {

#ifdef USE_A2DP_CONNECTION_STATE
class ConnectionStateTrigger : public Trigger<esp_a2d_connection_state_t> {
 public:
  explicit ConnectionStateTrigger(A2DPSinkHub *parent) {
    parent->add_connection_state_callbacks([this](esp_a2d_connection_state_t state, void *user_data) {
      this->trigger(state);
    });
  }
};
#endif

#ifdef USE_A2DP_PLAYBACK_STATUS
class PlaybackStatusTrigger : public Trigger<esp_avrc_playback_stat_t> {
 public:
  explicit PlaybackStatusTrigger(A2DPSinkHub *parent) {
    parent->add_playback_status_callbacks([this](esp_avrc_playback_stat_t playback) {
      this->trigger(playback);
    });
  }
};
#endif

#ifdef USE_A2DP_POS
class PlaybackPositionTrigger : public Trigger<uint32_t> {
 public:
  explicit PlaybackPositionTrigger(A2DPSinkHub *parent) {
    parent->add_playback_position_callbacks([this](uint32_t pos) {
      this->trigger(pos);
    });
  }
};
#endif

#ifdef USE_A2DP_RSSI
class RssiTrigger : public Trigger<int> {
 public:
  explicit RssiTrigger(A2DPSinkHub *parent) {
    parent->add_rssi_callback([this](esp_bt_gap_cb_param_t::read_rssi_delta_param& rssi) {
      this->trigger(rssi.rssi_delta);
    });
  }
};
#endif

#ifdef USE_A2DP_METADATA
class MetadataUpdateTrigger : public Trigger<uint8_t, std::string> {
 public:
  explicit MetadataUpdateTrigger(A2DPSinkHub *parent) {
    parent->add_metadata_update_callback([this](const A2DPSinkMetadata &meta) {
      this->trigger(meta.type(), std::string(meta.text()));
    });
  }
};
#endif

#ifdef USE_A2DP_PEER_NAME
class PeerNameTrigger : public Trigger<std::string> {
 public:
  explicit PeerNameTrigger(A2DPSinkHub *parent) {
    parent->add_peer_name_callback([this](const char* name) {
      this->trigger(std::string(name));
    });
  }
};
#endif

#ifdef USE_A2DP_VOLUME
class VolumeChangeTrigger : public Trigger<uint8_t> {
 public:
  explicit VolumeChangeTrigger(A2DPSinkHub *parent) {
    parent->add_volume_change_callback([this](int volume) {
      this->trigger((uint8_t)volume);
    });
  }
};
#endif

}  // namespace a2dp_sink
}  // namespace esphome
