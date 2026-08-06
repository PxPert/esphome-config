#pragma once

#include "../a2dp_sink_hub.h"
#include "esphome/components/number/number.h"

namespace esphome::a2dp_sink {

class A2DPSinkVolumeNumber : public A2DPSinkChild, public number::Number {
 public:
  void setup() override;
  void dump_config() override;


 protected:
  void control(float value) override;

};

}  // namespace esphome::a2dp_sink