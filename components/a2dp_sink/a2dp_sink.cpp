#include "esphome/core/log.h"
#include "a2dp_sink.h"
#include "AudioTools.h"
// #include "AudioTools/AudioCodecs/CodecSBC.h"
// #include "AudioTools/AudioCodecs/CodecAACHelix.h"
#include "BluetoothA2DPSink.h"
// #include "A2DPDecoderSBC.h"
// #include "A2DPDecoderAAC.h"

namespace esphome
{
    namespace a2dp_sink
    {

        static constexpr uint32_t PAUSE_POLL_DELAY_MS = 20;
        // static constexpr uint32_t AUDIO_WRITE_TIMEOUT_MS = 50;
        static constexpr uint32_t AUDIO_WRITE_TIMEOUT_MS = 10;
        static constexpr const char *const URI_PREFIX = "a2dp://";
        static const char *TAG = "a2dp_sink";

        static A2DPSink *g_a2dp_sink_instance = nullptr;
        static BluetoothA2DPSink a2dp_sink_;

//        static SBCDecoder sbc_decoder;
//        static A2DPDecoderSBC a2dp_sbc(sbc_decoder);

//        static AACDecoderHelix aac_decoder;
//        static A2DPDecoderAAC a2dp_aac(aac_decoder);

        void A2DPSink::setup()
        {
            ESP_LOGW("log", "%s", "A2DPSink initializing");

            this->disable_loop();

            if (g_a2dp_sink_instance != nullptr) {
                ESP_LOGE(TAG, "Solo un'istanza di A2DPSink è supportata (limite hardware Bluetooth)");
                this->mark_failed();
                return;
            }
            g_a2dp_sink_instance = this;

            this->a2dp_sink_.set_stream_reader(
                [](const uint8_t *data, uint32_t length) {
                    if (g_a2dp_sink_instance != nullptr) {
                    g_a2dp_sink_instance->a2dp_data_stream(data, length);
                    }
                },
                false  // i2s_output: false = gestisci tu l'audio, la libreria non fa output I2S automatico
            );

            ESP_LOGW("log", "%s", "A2DPSink is initialized");
            this->pause_.store(false, std::memory_order_relaxed);
//            a2dp_sink_.add_decoder(a2dp_sbc);
//            a2dp_sink_.add_decoder(a2dp_aac);
            a2dp_sink_.start("MyMusic");
        }

        void A2DPSink::loop()
        {
        }


        void A2DPSink::dump_config()
        {
            ESP_LOGCONFIG(TAG, "A2DP Sink");
        }

        bool A2DPSink::play_uri(const std::string &uri) {
            ESP_LOGE(TAG, "Play URI: '%s'", uri.c_str());
            if (!this->is_ready() || this->is_failed() || this->status_has_error()) {
                return false;
            }

            // Check if source is already playing
            if (this->get_state() != media_source::MediaSourceState::IDLE) {
                ESP_LOGE(TAG, "Cannot play '%s': source is busy", uri.c_str());
                return false;
            }

            // Validate URI starts with "http://" or "https://"
            if (!this->can_handle(uri)) {
                ESP_LOGE(TAG, "Invalid URI: '%s'", uri.c_str());
                return false;
            }
            ESP_LOGE(TAG, "Play URI: '%s' OK!", uri.c_str());
                        // a2dp_sink_.start("MyMusic");
            this->set_state_(media_source::MediaSourceState::PLAYING);

            return true;
        }

        void A2DPSink::handle_command(media_source::MediaSourceCommand command) {
            ESP_LOGE(TAG, "handle_command requested: %d", command);
            switch (command) {
                case media_source::MediaSourceCommand::STOP:
                    ESP_LOGE(TAG, "Stop requested");
                    this->pause_.store(false, std::memory_order_relaxed);
                    this->set_state_(media_source::MediaSourceState::IDLE);
                break;
                case media_source::MediaSourceCommand::PAUSE:
                    ESP_LOGE(TAG, "Pause requested");
                    this->pause_.store(true, std::memory_order_relaxed);
                    this->set_state_(media_source::MediaSourceState::PAUSED);
                break;
                case media_source::MediaSourceCommand::PLAY:
                    ESP_LOGE(TAG, "Play requested");
                    this->set_state_(media_source::MediaSourceState::PLAYING);
                    this->pause_.store(false, std::memory_order_relaxed);
                break;
                default:
                    ESP_LOGE(TAG, "Unhandled command requested: %d", command);
                break;
            }
        }

        bool A2DPSink::can_handle(const std::string &uri) const {
            ESP_LOGE(TAG, "Check URI: '%s'", uri.c_str());
            return uri.starts_with(URI_PREFIX);
        }

        void A2DPSink::a2dp_data_stream(const uint8_t *data, uint32_t length) {

            if (this->pause_.load(std::memory_order_relaxed)) {
                // vTaskDelay(pdMS_TO_TICKS(PAUSE_POLL_DELAY_MS));
                return;
            }

            static audio::AudioStreamInfo i(16, 2, 44100);
            this->write_output(data, length, AUDIO_WRITE_TIMEOUT_MS, i);

        }


    } // namespace a2dp_sink
} // namespace esphome
