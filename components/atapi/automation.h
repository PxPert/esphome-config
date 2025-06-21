
#pragma once

#include <utility>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "atapi.h"

namespace esphome {
namespace atapi {

class ClickTrigger : public Trigger<> {
 public:
  explicit ClickTrigger(Atapi *parent, uint32_t min_length, uint32_t max_length)
      : min_length_(min_length), max_length_(max_length) {
    parent->add_on_state_callback([this](bool state) {
      if (state) {
        this->start_time_ = millis();
      } else {
        const uint32_t length = millis() - this->start_time_;
        this->trigger();
      }
    });
  }

 protected:
  uint32_t start_time_{0};  /// The millis() time when the click started.
  uint32_t min_length_;     /// Minimum length of click. 0 means no minimum.
  uint32_t max_length_;     /// Maximum length of click. 0 means no maximum.
};

}
}
