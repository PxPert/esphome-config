#include "a2dp_sink_switch.h"

namespace esphome::a2dp_sink {

static const char *const TAG = "a2dp_sink.switch";

// ---------------------------------------------------------------------------
// A2DPSwitchConnection
// ---------------------------------------------------------------------------

void A2DPSwitchConnection::dump_config() {
  LOG_SWITCH("", "A2DPSink Connection Switch", this);
}

void A2DPSwitchConnection::setup() {
  this->parent_->add_connection_state_callbacks([this](esp_a2d_connection_state_t state, void *user_data) {
    bool is_connected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
    this->publish_state_if_changed_(is_connected);
  });
}

void A2DPSwitchConnection::write_state(bool state) {
  if (state != this->state) {
    ESP_LOGI(TAG, "A2DP connection switch state changed: %s", state ? "ON" : "OFF");
    if (state) {
      this->parent_->a2dp_sink()->set_connected(true);
    } else {
      this->parent_->a2dp_sink()->set_connected(false);
    }    
  }
}

void A2DPSwitchConnection::publish_state_if_changed_(bool state) {
  if (this->state != state) {
    this->publish_state(state);
  }
}

// ---------------------------------------------------------------------------
// A2DPSwitchBluetooth
// ---------------------------------------------------------------------------

void A2DPSwitchBluetooth::dump_config() {
  LOG_SWITCH("", "A2DPSink Bluetooth Switch", this);
}

void A2DPSwitchBluetooth::setup() {
  this->disable_loop();
  bool initial_state = this->get_initial_state_with_restore_mode().value_or(false);

  // write state before setup
  if (initial_state) {
    this->turn_on();
  } else {
    this->turn_off();
  }
}

void A2DPSwitchBluetooth::write_state(bool state) {
  if (state) {
    this->parent_->start();
  } else {
    this->parent_->stop();
  }
  this->publish_state_if_changed_(state);
}

void A2DPSwitchBluetooth::publish_state_if_changed_(bool state) {
  if (this->state != state) {
    this->publish_state(state);
  }
}

}  // namespace esphome::a2dp_sink