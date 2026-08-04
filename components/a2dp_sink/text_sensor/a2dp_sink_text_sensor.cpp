#include "a2dp_sink_text_sensor.h"

#include <string>

namespace esphome::a2dp_sink {

static const char *const TAG = "sendspin.text_sensor";

void A2DPSinkTextSensor::dump_config() { LOG_TEXT_SENSOR("", "A2DPSink", this); }


// THREAD CONTEXT: Main loop. The registered metadata callback also fires on the main loop
// (SendspinHub dispatches metadata from client_->loop()).
void A2DPSinkTextSensor::setup() {
  this->parent_->add_metadata_update_callback([this](const A2DPSinkMetadata &metadata) {
    ESP_LOGW(TAG, "Callback Text sensor!!");
    if (metadata.type() == (uint8_t)this->metadata_type_ ) {
      this->publish_if_changed_((const char*)metadata.text());
    }
  });
}


// Dedup to avoid frontend churn; TextSensor::publish_state already dedups the string assign but still notifies.
void A2DPSinkTextSensor::publish_if_changed_(const char *value) {
  if (this->get_raw_state() != value) {
    this->publish_state(value);
  }
}

}  // namespace esphome::a2dp_sink

