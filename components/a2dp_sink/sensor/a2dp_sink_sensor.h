#pragma once

#include "esphome/core/defines.h"


#include "../a2dp_sink_hub.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome::a2dp_sink {

class A2DPSinkTextSensor final : public A2DPSinkChild, public text_sensor::TextSensor {
 public:
  void dump_config() override;
  void setup() override;

  void set_metadata_type(uint8_t metadata_type) { this->metadata_type_ = metadata_type; }

 protected:
//  const char *extract_value_(const sendspin::ServerMetadataStateObject &metadata) const;
  void publish_if_changed_(const char *value);

  uint8_t metadata_type_;
};

}  // namespace esphome::a2dp_sink

