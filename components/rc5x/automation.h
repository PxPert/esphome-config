#pragma once

#include <utility>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "rc5x.h"

namespace esphome {
namespace rc5x {

class CommandTrigger : public Trigger<bool, uint32_t> {
 public:
  explicit CommandTrigger(RC5x *parent) {
    parent->add_on_command_callback([this](unsigned char toggle, unsigned char address, unsigned char command, unsigned char extcode) {
      this->trigger(toggle,(address << 16) | (command << 8)  | extcode);
    });

  }
};

}
}
