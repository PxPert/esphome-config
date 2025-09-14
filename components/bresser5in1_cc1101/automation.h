
#pragma once

#include <utility>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "bresser5in1_cc1101.h"

namespace esphome {
namespace bresser5in1_cc1101 {

class StateTrigger : public Trigger<const BresserReading*> {
 public:
  explicit StateTrigger(Bresser5in1CC1101Component *parent) {
    parent->add_on_state_callback([this](const BresserReading* reading) {
        this->trigger(reading);
    });

  }

};

}
}
