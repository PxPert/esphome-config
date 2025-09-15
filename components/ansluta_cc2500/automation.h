#pragma once
#include "esphome/core/log.h"
#include "esphome/core/automation.h"
#include "esphome/components/light/light_state.h"
#include "ansluta_cc2500.h"
#include "light/light.h"

namespace esphome {
namespace ansluta_cc2500 {
class OnRemoteClickTrigger : public Trigger<uint8_t> {
 public:
  OnRemoteClickTrigger(AnslutaCC2500Component *parent) {
    parent->add_on_remote_click_callback(
        [this](uint8_t command) {
          if (this->debounce_.has_value() && millis() < (this->last_trigger_ + this->debounce_.value())) {
            return;
          }
        });
  }
  void set_debounce(uint16_t delay) { this->debounce_ = delay; }
 protected:
  optional<uint16_t> debounce_{};
  uint32_t last_trigger_{0};
};

}  // namespace ikea_ansluta
}  // namespace esphome
