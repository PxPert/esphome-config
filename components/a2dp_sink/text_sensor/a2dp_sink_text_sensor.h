#pragma once

#include "esphome/core/defines.h"


#include "../a2dp_sink_hub.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome::a2dp_sink {

class A2DPSinkTextSensor : public A2DPSinkChild, public text_sensor::TextSensor {
 public:
  void dump_config() override;
  void setup() override {};

 protected:
  void publish_if_changed_(const char *value);

};

#ifdef USE_A2DP_METADATA

class A2DPMetadataTextSensor : public A2DPSinkTextSensor {
 public:
  void setup() override;
  void set_metadata_type(A2DPSinkMetadataTypes metadata_type) { this->metadata_type_ = metadata_type; }
 protected:
   A2DPSinkMetadataTypes metadata_type_;

};
#endif

#ifdef USE_A2DP_PEER_NAME
class A2DPSinkPeerTextSensor : public A2DPSinkTextSensor
{
 public:
  void setup() override;
  void set_peer_type(A2DPSinkPeerRequestTypes peer_type) { this->peer_type_ = peer_type; }
 protected:
  A2DPSinkPeerRequestTypes peer_type_;
};
#endif


}  // namespace esphome::a2dp_sink

