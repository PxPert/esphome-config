#include "a2dp_sink_text_sensor.h"

#include <string>

namespace esphome::a2dp_sink {

static const char *const TAG = "sendspin.text_sensor";

void A2DPSinkTextSensor::dump_config() { LOG_TEXT_SENSOR("", "A2DPSink", this); }


// Dedup to avoid frontend churn; TextSensor::publish_state already dedups the string assign but still notifies.
void A2DPSinkTextSensor::publish_if_changed_(const char *value) {
  if (this->get_raw_state() != value) {
    this->publish_state(value);
  }
}


// THREAD CONTEXT: Main loop. The registered metadata callback also fires on the main loop
// (SendspinHub dispatches metadata from client_->loop()).
void A2DPMetadataTextSensor::setup() {
  this->parent_->add_metadata_update_callback([this](const A2DPSinkMetadata &metadata) {
    ESP_LOGW(TAG, "Callback Text sensor!!");
    if (metadata.type() == (uint8_t)this->metadata_type_ ) {
      this->publish_if_changed_((const char*)metadata.text());
    }
  });
}

void A2DPSinkPeerTextSensor::setup() {

  this->parent_->add_connection_state_callbacks([this](esp_a2d_connection_state_t state, void *user_data) {
    if (state != ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
      this->publish_if_changed_("disconnected");
    }
  });

  this->parent_->add_peer_name_callback([this](const char *name) {
    if (this->parent_->a2dp_sink()->get_connection_state() == ESP_A2D_CONNECTION_STATE_CONNECTED) {
      if (this->metadata_type_ == A2DPSinkTextMetadataTypes::PEERNAME) {
        ESP_LOGW(TAG, "Peer text sensor callback fired, publishing peer name: %s", this->parent_->a2dp_sink()->get_peer_name());
        this->publish_if_changed_(name);
      } else if (this->metadata_type_ == A2DPSinkTextMetadataTypes::PEERADDR) {
        auto* peer_addr = this->parent_->a2dp_sink()->get_current_peer_address();
        this->publish_if_changed_(A2DPSinkHub::bd_addr_to_string(*peer_addr).c_str());
      } else {
        ESP_LOGW(TAG, "Peer text sensor callback fired but type is not PEERNAME or PEERADDR, ignoring");
        return;
      }
    } else {
      this->publish_if_changed_("");
    }

  });
}
}  // namespace esphome::a2dp_sink

