#include "a2dp_sink_sensor.h"

#include <string>

namespace esphome::a2dp_sink {

static const char *const TAG = "a2dp_sink.sensor";

void A2DPSinkNumericSensor::dump_config() { LOG_SENSOR("", "A2DPSink Numeric", this); }


// Dedup to avoid frontend churn; Sensor::publish_state already dedups the float assign but still notifies.
void A2DPSinkNumericSensor::publish_if_changed_(float value) {
  if (this->get_raw_state() != value) {
    this->publish_state(value);
  }
}

#ifdef USE_A2DP_METADATA

void A2DPMetadataNumericSensor::setup() {
  this->parent_->add_metadata_update_callback([this](const A2DPSinkMetadata &metadata) {
    if (metadata.type() == (uint8_t)this->metadata_type_) {
      // Metadata values come as strings, parse them to float
      float value = 0.0f;
      if (sscanf((const char*)metadata.text(), "%f", &value) == 1) {
        this->publish_if_changed_(value);
      }
    }
  });
}
#endif

#ifdef USE_A2DP_POS

void A2DPSinkTrackPositionSensor::setup() {
  this->parent_->add_playback_position_callbacks([this](uint32_t pos) {
    this->publish_if_changed_((float)pos);
  });
}

#endif

#ifdef USE_A2DP_RSSI

void A2DPSinkRssiSensor::setup() {
  this->parent_->add_rssi_callback([this](esp_bt_gap_cb_param_t::read_rssi_delta_param& rssi) {
    this->publish_if_changed_((float)rssi.rssi_delta);
  });
}

#endif

}  // namespace esphome::a2dp_sink