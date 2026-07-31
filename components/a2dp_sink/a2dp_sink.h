#pragma once

#include "esphome/components/audio/audio.h"
#include "esphome/components/media_source/media_source.h"
#include "esphome/core/component.h"
#include "BluetoothA2DPSink.h"
#include <atomic>


namespace esphome
{
    namespace a2dp_sink
    {


        class A2DPSink : public Component, public media_source::MediaSource
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
            BluetoothA2DPSink a2dp_sink_;
            std::atomic<bool> pause_{false};


        }; // class A2DPSink

    } // namespace a2dp_sink
} // namespace esphome
