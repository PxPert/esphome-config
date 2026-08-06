#include "a2dp_sink_volume.h"

namespace esphome::a2dp_sink {

static const char *const TAG = "a2dp_sink.number";

void A2DPSinkVolumeNumber::setup() {
  // Subscribe to volume change callbacks from the hub
  this->parent_->add_volume_change_callback([this](int volume) {
    this->publish_state(static_cast<float>(volume));
  });
}

void A2DPSinkVolumeNumber::dump_config() { 
  LOG_NUMBER("", "A2DPSink Volume", this); 
}

void A2DPSinkVolumeNumber::control(float value) {
  // Call set_volume on the A2DP sink through the parent hub
  // The value is already validated by ESPHome (0-127 range)
  this->parent_->a2dp_sink()->set_volume(static_cast<uint8_t>(value));
}


}  // namespace esphome::a2dp_sink