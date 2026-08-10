#include "esphome/core/log.h"
#include "a2dp_sink_media_source.h"
#include "AudioTools.h"
// #include "AudioTools/AudioCodecs/CodecSBC.h"
// #include "AudioTools/AudioCodecs/CodecAACHelix.h"
#include "BluetoothA2DPSink.h"
// #include "A2DPDecoderSBC.h"
// #include "A2DPDecoderAAC.h"

namespace esphome::a2dp_sink {

        static constexpr uint32_t PAUSE_POLL_DELAY_MS = 20;
        // static constexpr uint32_t AUDIO_WRITE_TIMEOUT_MS = 50;
        static constexpr uint32_t AUDIO_WRITE_TIMEOUT_MS = 10;
        static constexpr const char *const URI_PREFIX = "a2dp://";
        static const char *TAG = "a2dp_sink_media_source";

        static A2DPSinkMediaSource *g_a2dp_sink_instance = nullptr;

        void A2DPSinkMediaSource::loop() {
            /*
            static unsigned long int last_stub_time_ = 0;

            if (millis() - last_stub_time_ >= 5000) {
                // --- comando stub qui ---
                ESP_LOGI(TAG, "Stub executed %d - audio state: %d", (int)this->get_state(),this->parent_->a2dp_sink()->get_audio_state());
                // -------------------------
                last_stub_time_ = millis();
            }
            */

        }
        void A2DPSinkMediaSource::setup()
        {
            ESP_LOGW(TAG, "A2DPSink initializing");

            this->disable_loop();

            g_a2dp_sink_instance = this;

            this->parent_->set_stream_reader(
                [](const uint8_t *data, uint32_t length) {
                    if (g_a2dp_sink_instance != nullptr) {
                        g_a2dp_sink_instance->a2dp_data_stream(data, length);
                    }
                }
            );

            this->parent_->add_avrc_connection_state_callback([this](bool connected) {
                if (connected) {
                    this->parent_->play_a2dp();
                }
            });

            this->parent_->add_audio_state_callback([this](esp_a2d_audio_state_t state) {
                ESP_LOGI(TAG, "audio_state_callback: %d", state);
                if (state == ESP_A2D_AUDIO_STATE_STARTED) {
                    this->set_state_(media_source::MediaSourceState::PLAYING);
                } else {
                    if (this->get_state() == media_source::MediaSourceState::PLAYING) {
                    this->set_state_(media_source::MediaSourceState::IDLE);
                    }
                }
            });

            this->parent_->add_playback_status_callbacks([this](esp_avrc_playback_stat_t playback) {
                ESP_LOGE(TAG, "Play status: %d", playback);
                switch (playback) {
                    case ESP_AVRC_PLAYBACK_PLAYING:
                        //this->request_play_uri_(URI_PREFIX);
                        this->set_state_(media_source::MediaSourceState::PLAYING);
                        break;
                    case ESP_AVRC_PLAYBACK_STOPPED:
                        this->set_state_(media_source::MediaSourceState::IDLE);
                        break;
                    case ESP_AVRC_PLAYBACK_PAUSED:
                        this->set_state_(media_source::MediaSourceState::PAUSED);
                        break;
                }
            });

            this->parent_->add_connection_state_callbacks([this](esp_a2d_connection_state_t state, void *user_data) {
                this->set_state_(media_source::MediaSourceState::IDLE);
                if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
                    this->request_play_uri_(URI_PREFIX);
                }
            });

            this->parent_->add_sample_rate_callback([this](uint16_t sample_rate) {
                this->stream_info_ = audio::AudioStreamInfo(
                    16, 
                    this->parent_->a2dp_sink()->channels(), 
                    sample_rate
                );                
            });

            ESP_LOGW(TAG, "A2DPSink is initialized");
        }

        void A2DPSinkMediaSource::dump_config()
        {
            ESP_LOGCONFIG(TAG, "A2DP Sink Media Source");
        }

        bool A2DPSinkMediaSource::play_uri(const std::string &uri) {
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
            return true;
        }

        void A2DPSinkMediaSource::handle_command(media_source::MediaSourceCommand command) {
            ESP_LOGE(TAG, "handle_command requested: %d", command);
            switch (command) {
                case media_source::MediaSourceCommand::STOP:
                    ESP_LOGD(TAG, "Stop requested");
                    if (
                        (this->get_state() != media_source::MediaSourceState::PLAYING) 
                        &&
                        (this->get_state() != media_source::MediaSourceState::PAUSED)
                    )
                    {
                        ESP_LOGW(TAG, "Cannot stop: source is not playing playing or paused");
                        return;
                    }
                    this->parent_->set_connected(false);
                    this->set_state_(media_source::MediaSourceState::IDLE);
                break;
                case media_source::MediaSourceCommand::PAUSE:
                    ESP_LOGD(TAG, "Pause requested");
                    if (this->get_state() != media_source::MediaSourceState::PLAYING) {
                        ESP_LOGW(TAG, "Cannot pause: source is not playing");
                        return;
                    }
                    this->parent_->pause_a2dp();
                    // this->set_state_(media_source::MediaSourceState::PAUSED);
                break;
                case media_source::MediaSourceCommand::PLAY:
                    ESP_LOGD(TAG, "Play requested");
                    if (this->parent_->get_connection_state() != ESP_A2D_CONNECTION_STATE_CONNECTED) {
                        ESP_LOGW(TAG, "Cannot play: A2DP is not connected");
                    } else {
                        this->parent_->play_a2dp();
                    }
                    // this->set_state_(media_source::MediaSourceState::PLAYING);
                break;
                case media_source::MediaSourceCommand::NEXT:
                    ESP_LOGD(TAG, "Next requested");
                    if (this->parent_->get_connection_state() != ESP_A2D_CONNECTION_STATE_CONNECTED) {
                        ESP_LOGW(TAG, "Cannot go to next: A2DP is not connected");
                        return;
                    }
                    this->parent_->next_track();
                break;
                case media_source::MediaSourceCommand::PREVIOUS:
                    ESP_LOGD(TAG, "Previous requested");
                    if (this->parent_->get_connection_state() != ESP_A2D_CONNECTION_STATE_CONNECTED) {
                        ESP_LOGE(TAG, "Cannot go to previous: A2DP is not connected");
                        return;
                    }
                    this->parent_->prev_track();
                break;
                default:
                    ESP_LOGE(TAG, "Unhandled command requested: %d", command);
                break;
            }
        }

        bool A2DPSinkMediaSource::can_handle(const std::string &uri) const {
            // ESP_LOGE(TAG, "Check URI: '%s'", uri.c_str());
            return uri.starts_with(URI_PREFIX);
        }

        void A2DPSinkMediaSource::a2dp_data_stream(const uint8_t *data, uint32_t length) {

            if (this->get_state() == media_source::MediaSourceState::PLAYING) {
                this->write_output(data, length, AUDIO_WRITE_TIMEOUT_MS, this->stream_info_);
            }

        }

} // namespace esphome::a2dp_sink
