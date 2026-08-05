#pragma once

#include "esphome/core/defines.h"

#include "../a2dp_sink_hub.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome::a2dp_sink {

class A2DPSinkNumericSensor : public A2DPSinkChild, public sensor::Sensor {
 public:
  void dump_config() override;
  void setup() override {};

  void set_metadata_type(A2DPSinkTextMetadataTypes metadata_type) { this->metadata_type_ = metadata_type; }

 protected:
  void publish_if_changed_(float value);

  A2DPSinkTextMetadataTypes metadata_type_;
};


class A2DPMetadataNumericSensor : public A2DPSinkNumericSensor {
 public:
  void setup() override;
};

class A2DPSinkTrackPositionSensor : public A2DPSinkNumericSensor
{
 public:
  void setup() override;
};

class A2DPSinkRssiSensor : public A2DPSinkNumericSensor
{
 public:
  void setup() override;
};


}  // namespace esphome::a2dp_sink