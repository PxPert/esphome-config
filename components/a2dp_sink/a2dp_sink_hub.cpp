#include "esphome/core/log.h"
#include "a2dp_sink_hub.h"
#include "BluetoothA2DPSink.h"

namespace esphome
{
    namespace a2dp_sink
    {
        static const char *TAG = "a2dp_sink_hub";

        static A2DPSinkHub *g_a2dp_hub_instance = nullptr;
        static BluetoothA2DPSink a2dp_sink_;

//        static SBCDecoder sbc_decoder;
//        static A2DPDecoderSBC a2dp_sbc(sbc_decoder);

//        static AACDecoderHelix aac_decoder;
//        static A2DPDecoderAAC a2dp_aac(aac_decoder);

        void A2DPSinkHub::avrc_metadata_callback(uint8_t id, const uint8_t *text) {
            ESP_LOGD(TAG, "AVRC metadata rsp: attribute id 0x%x, %s", id, text);

            A2DPSinkMetadata m(id, text );
            this->metadata_update_callbacks_.call(m);

        }

        void A2DPSinkHub::avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback) {
            ESP_LOGD(TAG, "AVRC playstatus rsp: %d", (uint8_t)playback);
            this->playback_status_callbacks_.call(playback);
        }

        void A2DPSinkHub::avrc_rn_play_pos_callback(uint32_t play_pos) {
            ESP_LOGD(TAG, "AVRC playposition: %d", play_pos);
            this->playback_position_callbacks_.call(play_pos);
        }


        void A2DPSinkHub::setup()
        {
            ESP_LOGW(TAG, "%s", "A2DPSink initializing");

            g_a2dp_hub_instance = this;

            this->disable_loop();

            /*
             * Metadata subscription and callbacks
             */
            a2dp_sink_.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_PLAYING_TIME );
            a2dp_sink_.set_avrc_metadata_callback(
                [](uint8_t id, const uint8_t *text) {
                    if (g_a2dp_hub_instance != nullptr) {
                        g_a2dp_hub_instance->avrc_metadata_callback(id, text);
                    }
                }
            );

            /*
             * Playback status callback
             */
            a2dp_sink_.set_avrc_rn_playstatus_callback(
                [](esp_avrc_playback_stat_t playback) {
                    if (g_a2dp_hub_instance != nullptr) {
                        g_a2dp_hub_instance->avrc_rn_playstatus_callback(playback);
                    }
                }
            );

            /*
             * Playback position callback
             */
            a2dp_sink_.set_avrc_rn_play_pos_callback(
                [](uint32_t play_pos) {
                    if (g_a2dp_hub_instance != nullptr) {
                        g_a2dp_hub_instance->avrc_rn_play_pos_callback(play_pos);
                    }
                }
            );



            a2dp_sink_.start(this->name().c_str(), this->auto_reconnect());




            ESP_LOGW(TAG, "%s", "A2DPSink is initialized");
        }

        void A2DPSinkHub::loop()
        {
        }

        BluetoothA2DPSink* A2DPSinkHub::a2dp_sink() const {
            return &a2dp_sink_;
        }

        void A2DPSinkHub::dump_config()
        {
            ESP_LOGCONFIG(TAG, "A2DP Sink");
        }



    } // namespace a2dp_sink
} // namespace esphome
