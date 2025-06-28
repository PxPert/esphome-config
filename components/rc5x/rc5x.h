#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"
// #include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace rc5x {


struct RC5xComponentStore {
  static void rc5x_read(RC5xComponentStore* store);
  static void decodeEvent(RC5xComponentStore* store, unsigned char event);
  static void reset(RC5xComponentStore* store);


  ISRInternalGPIOPin pin;
  // Used by ISR
  unsigned long time0;
  bool lastValue;
  unsigned char state;
  unsigned char bits;
  unsigned char messageLength;
  uint32_t command;

  // Shared with main loop
  volatile uint32_t message;

};
// class RC5x : public binary_sensor::BinarySensor, public Component {
class RC5x : public Component {
 public:
  void set_pin(InternalGPIOPin *pin) { _pin = pin; }
  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)
  /// Setup pin
  void setup() override;
  void dump_config() override;
  /// Hardware priority
  float get_setup_priority() const override;
  /// Check sensor
  void loop() override;

  void add_on_command_callback(std::function<void(unsigned char, unsigned char, unsigned char, unsigned char)> &&callback){
    this->command_callback_.add(std::move(callback));
  }


 protected:
  CallbackManager<void(unsigned char, unsigned char, unsigned char, unsigned char)> command_callback_{};

 private:
  InternalGPIOPin *_pin;

  RC5xComponentStore _store;


};

}  // namespace rc5x
}  // namespace esphome
