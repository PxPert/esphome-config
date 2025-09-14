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
  volatile bool invert_receive{false};
  volatile uint32_t message;

};

// class RC5x : public binary_sensor::BinarySensor, public Component {
class RC5x : public Component {
 public:
  void set_receivePin(InternalGPIOPin *pin) { _receivePin = pin; }
  void set_invertReceivePin(bool invert) { _store.invert_receive = invert; }
  void set_sendPin(GPIOPin *pin) { _sendPin = pin; }
  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)
  /// Setup pin
  void setup() override;
  void dump_config() override;
  /// Hardware priority
  float get_setup_priority() const override;
  /// Check sensor
  void loop() override;

  void add_on_command_press_callback(std::function<void(unsigned char, unsigned char, unsigned char, unsigned char)> &&callback){
    this->command_press_callback_.add(std::move(callback));
  }
  void add_on_command_release_callback(std::function<void(unsigned char, unsigned char, unsigned char, unsigned char)> &&callback){
    this->command_release_callback_.add(std::move(callback));
  }

  void send_rc5x(uint32_t irData, unsigned char repetitions);
  void send_rc5(uint16_t irData, unsigned char repetitions);
  void send_rc5x(uint8_t aAddress, uint8_t aCommand, uint8_t aExt, uint8_t aNumberOfRepeats);
  void send_rc5(uint8_t aAddress, uint8_t aCommand, uint8_t aNumberOfRepeats);


 protected:
  CallbackManager<void(unsigned char, unsigned char, unsigned char, unsigned char)> command_press_callback_{};
  CallbackManager<void(unsigned char, unsigned char, unsigned char, unsigned char)> command_release_callback_{};

 private:
  InternalGPIOPin *_receivePin{nullptr};

  GPIOPin *_sendPin{nullptr};

  RC5xComponentStore _store;
  uint32_t _active_message;
  bool _active_toggle;
  unsigned char _active_address, _active_command, _active_extCode;
  unsigned long _active_command_press_time;

  void send_0();
  void send_1();
  uint8_t _lastSendToggleValue{0};


};

}  // namespace rc5x
}  // namespace esphome
