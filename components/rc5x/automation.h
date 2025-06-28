#pragma once

#include <utility>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "rc5x.h"

namespace esphome {
namespace rc5x {

class CommandTrigger : public Trigger<int, int, int, int> {
 public:
  explicit CommandTrigger(RC5x *parent) {
    parent->add_on_command_callback([this](unsigned char toggle, unsigned char address, unsigned char command, unsigned char extcode) {
      this->trigger(toggle,address,command,extcode);
    });

  }
};

}
}
