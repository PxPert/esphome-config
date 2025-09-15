#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/light/light_traits.h"
#include "esphome/components/light/light_output.h"
#include "../ansluta_cc2500.h"

namespace esphome {
namespace ansluta_cc2500 {

class AnslutaCC2500Light : public Component, public light::LightOutput {
 public:
  void setup() override;
  void dump_config() override;
  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;
  light::LightTraits get_traits() override;
  void set_parent(AnslutaCC2500Component *parent) { this->parent_ = parent; };


 protected:
  AnslutaCC2500Component *parent_;
  light::LightState *state_{nullptr};


};


}  // namespace empty_spi_component
}  // namespace esphome
