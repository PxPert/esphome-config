#pragma once

#include "esphome/components/audio/audio.h"
#include "esphome/components/media_source/media_source.h"
#include "esphome/core/component.h"
#include "../a2dp_sink_hub.h"

#include <atomic>


namespace esphome::a2dp_sink {


        class A2DPSinkMediaSource : public A2DPSinkChild, public media_source::MediaSource
        {
        public:
            void setup() override;
            void loop() override;
            void dump_config() override;

            // MediaSource interface implementation
            bool play_uri(const std::string &uri) override;
            void handle_command(media_source::MediaSourceCommand command) override;
            bool can_handle(const std::string &uri) const override;

            void a2dp_data_stream(const uint8_t *data, uint32_t length);



        protected:
            std::atomic<bool> pause_{false};


        }; // class A2DPSink

} // namespace esphome::a2dp_sink
