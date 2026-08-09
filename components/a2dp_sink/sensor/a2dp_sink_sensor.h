#pragma once

#include "esphome/core/defines.h"

#include "../a2dp_sink_hub.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome::a2dp_sink {

class A2DPSinkNumericSensor : public A2DPSinkChild, public sensor::Sensor {
 public:
  void dump_config() override;
  void setup() override {};

 protected:
  void publish_if_changed_(float value);

};

#ifdef USE_A2DP_METADATA

class A2DPMetadataNumericSensor : public A2DPSinkNumericSensor {
 public:
  void setup() override;
  void set_metadata_type(A2DPSinkMetadataTypes metadata_type) { this->metadata_type_ = metadata_type; }
 protected:
   A2DPSinkMetadataTypes metadata_type_;

};

#endif

#ifdef USE_A2DP_POS

class A2DPSinkTrackPositionSensor : public A2DPSinkNumericSensor
{
 public:
  void setup() override;
};

#endif

#ifdef USE_A2DP_RSSI

class A2DPSinkRssiSensor : public A2DPSinkNumericSensor
{
 public:
  void setup() override;
};

#endif

#ifdef USE_A2DP_SAMPLE_RATE

class A2DPSinkSampleRateSensor : public A2DPSinkNumericSensor
{
 public:
  void setup() override;
};

class A2DPSinkChannelsSensor : public A2DPSinkNumericSensor
{
 public:
  void setup() override;
};

#endif

}  // namespace esphome::a2dp_sink