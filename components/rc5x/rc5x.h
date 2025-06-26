#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"
// #include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace rc5x {

// class RC5x : public binary_sensor::BinarySensor, public Component {
class RC5x : public Component {
 public:
  void set_pin(GPIOPin *pin) { pin_ = pin; }
  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)
  /// Setup pin
  void setup() override;
  void dump_config() override;
  /// Hardware priority
  float get_setup_priority() const override;
  /// Check sensor
  void loop() override;

  void add_on_command_callback(std::function<void(unsigned char, unsigned char, unsigned char)> &&callback){
    this->command_callback_.add(std::move(callback));
  }


 protected:
  GPIOPin *pin_;
  CallbackManager<void(unsigned char, unsigned char, unsigned char)> command_callback_{};

 private:
  unsigned char state;
  unsigned int bits;
  unsigned int command;
  unsigned long time0;
  unsigned long lastValue;

  void reset();
  bool read(unsigned int *message);
  void decodeEvent(unsigned char event);
  void decodePulse(unsigned char signal, unsigned long period);

};

}  // namespace rc5x
}  // namespace esphome
