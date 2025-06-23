
#pragma once

#include <utility>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "atapi.h"

namespace esphome {
namespace atapi {

class StateTrigger : public Trigger<int> {
 public:
  explicit StateTrigger(Atapi *parent) {
    last_state = -1; // Unset

    parent->add_on_state_callback([this](int val) {
      if (last_state != val) {
        last_state = val;
        this->trigger(val);
      }
    });

  }
 protected:
   int last_state;

};

class UpdateTrigger : public Trigger<> {
 public:
  explicit UpdateTrigger(Atapi *parent) {
    parent->add_on_update_callback([this]() {
        this->trigger();
    });
  }
};

class TocTrigger : public Trigger<> {
 public:
  explicit TocTrigger(Atapi *parent) {
    parent->add_on_toc_callback([this]() {
        this->trigger();
    });
  }
};

class LockTrigger : public Trigger<bool> {
 public:
  explicit LockTrigger(Atapi *parent) {
    parent->add_on_lock_callback([this](bool val) {
        this->trigger(val);
    });
  }
};

class ErrorTrigger : public Trigger<int> {
 public:
  explicit ErrorTrigger(Atapi *parent) {
    parent->add_on_error_callback([this](int val) {
        this->trigger(val);
    });
  }
};

}
}
