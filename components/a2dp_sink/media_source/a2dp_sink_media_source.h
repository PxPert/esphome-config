#pragma once

#include "esphome/components/audio/audio.h"
#include "esphome/components/media_source/media_source.h"
#include "esphome/core/component.h"
#include "../a2dp_sink_hub.h"


namespace esphome::a2dp_sink {


        class A2DPSinkMediaSource : public A2DPSinkChild, public media_source::MediaSource
        {
        public:
            void setup() override;
            void loop() override;
            void dump_config() override;

            // MediaSource interface implementation
            virtual bool play_uri(const std::string &uri) override;
            virtual void handle_command(media_source::MediaSourceCommand command) override;
            virtual bool can_handle(const std::string &uri) const override;
            virtual bool has_internal_playlist() const { return true; }


        protected:
            void a2dp_data_stream(const uint8_t *data, uint32_t length);

            audio::AudioStreamInfo stream_info_;

        }; // class A2DPSink

} // namespace esphome::a2dp_sink
