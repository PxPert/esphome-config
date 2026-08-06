#pragma once

#include "esphome/core/defines.h"


#include "../a2dp_sink_hub.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome::a2dp_sink {

class A2DPSinkTextSensor : public A2DPSinkChild, public text_sensor::TextSensor {
 public:
  void dump_config() override;
  void setup() override {};

  void set_metadata_type(A2DPSinkMetadataTypes metadata_type) { this->metadata_type_ = metadata_type; }

 protected:
//  const char *extract_value_(const sendspin::ServerMetadataStateObject &metadata) const;
  void publish_if_changed_(const char *value);

  A2DPSinkMetadataTypes metadata_type_;
};


class A2DPMetadataTextSensor : public A2DPSinkTextSensor {
 public:
  void setup() override;
};

class A2DPSinkPeerTextSensor : public A2DPSinkTextSensor
{
 public:
  void setup() override;
};


}  // namespace esphome::a2dp_sink

