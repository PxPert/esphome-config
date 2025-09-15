#include "esphome/core/log.h"
#include "light.h"

namespace esphome {
namespace ansluta_cc2500 {
static const char *TAG = "ansluta_cc2500.light";

void AnslutaCC2500Light::setup() {
  /*
  this->parent_->add_on_remote_click_callback(
      [this](uint16_t address, uint8_t command) {
        // Remote and light and has same addr, i.e not paired directly with the light
        if (this->address_ == address) {
          this->handle_remote_command_((Command) command);
        }
      });
      */
}

void AnslutaCC2500Light::dump_config() {
  ESP_LOGCONFIG(TAG, "Ansluta light");
}

void AnslutaCC2500Light::setup_state(light::LightState *state) {
  state_ = state;
  state_->set_gamma_correct(0);
  state_->set_default_transition_length(0);
}

light::LightTraits AnslutaCC2500Light::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
  return traits;
}


void AnslutaCC2500Light::write_state(light::LightState *state) {
  float brightness;
  state->current_values_as_brightness(&brightness);
  ESP_LOGD(TAG,"Requested set light to %.2f", brightness);

  if (brightness > 0) {
    if (brightness > 0.49) {
      this->parent_->setLight(2);
    } else {
      this->parent_->setLight(1);
    }
  } else {
    this->parent_->setLight(0);
  }
  /*
  on_change_callback_.call((uint8_t) command);
  this->parent_->queue_command(this->address_, command);
  */
}
/*
void AnslutaCC2500Light::add_on_change_callback(std::function<void(uint8_t)> &&change_callback) {
  this->on_change_callback_.add(std::move(change_callback));
}
*/

}  // namespace ikea_ansluta
}  // namespace esphome
