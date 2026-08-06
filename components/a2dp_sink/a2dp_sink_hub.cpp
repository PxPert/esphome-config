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

  #ifdef USE_A2DP_METADATA
        void A2DPSinkHub::avrc_metadata_callback(uint8_t id, const uint8_t *text) {
            ESP_LOGD(TAG, "AVRC metadata rsp: attribute id 0x%x, %s", id, text);

            A2DPSinkMetadata m(id, text );
            this->metadata_update_callbacks_.call(m);

        }
#endif

#ifdef USE_A2DP_PLAYBACK_STATUS
        void A2DPSinkHub::avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback) {
            ESP_LOGD(TAG, "AVRC playstatus rsp: %d", (uint8_t)playback);
            this->playback_status_callbacks_.call(playback);
        }
#endif

#ifdef USE_A2DP_POS
        void A2DPSinkHub::avrc_rn_play_pos_callback(uint32_t play_pos) {
            ESP_LOGD(TAG, "AVRC playposition: %d", play_pos);
            this->playback_position_callbacks_.call(play_pos);
        }
#endif

#ifdef USE_A2DP_VOLUME
        void A2DPSinkHub::avrc_rn_volumechange_callback(int volume) {
            ESP_LOGD(TAG, "AVRC volume change: %d", volume);
            this->volume_change_callbacks_.call(volume);
        }
#endif

#ifdef USE_A2DP_CONNECTION_STATE
        void A2DPSinkHub::on_connection_state_changed(esp_a2d_connection_state_t state, void *user_data) {
            ESP_LOGD(TAG, "A2DP connection state changed: %d", (uint8_t)state);
            this->connection_state_callbacks_.call(state, user_data);
        }
#endif

#ifdef USE_A2DP_PEER_NAME
        void A2DPSinkHub::peer_name_callback(const char* name) {
            this->peer_name_callbacks_.call(name);
        }
#endif

#ifdef USE_A2DP_RSSI
        void A2DPSinkHub::rssi_callback(esp_bt_gap_cb_param_t::read_rssi_delta_param& rssi) {
            ESP_LOGD(TAG, "RSSI delta: %d", rssi.rssi_delta);
            this->rssi_callbacks_.call(rssi);
        }
#endif

        void A2DPSinkHub::setup()
        {
            ESP_LOGW(TAG, "%s", "A2DPSink initializing");

            g_a2dp_hub_instance = this;

            this->disable_loop();

#ifdef USE_A2DP_METADATA
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
#endif

#ifdef USE_A2DP_PLAYBACK_STATUS
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
#endif

#ifdef USE_A2DP_POS
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
#endif

#ifdef USE_A2DP_CONNECTION_STATE
            /* 
             * Connection state callback
             */
            a2dp_sink_.set_on_connection_state_changed(
                [](esp_a2d_connection_state_t state, void *user_data) {
                    if (g_a2dp_hub_instance != nullptr) {
                        g_a2dp_hub_instance->on_connection_state_changed(state, user_data);
                    }
                }
            );
#endif

#ifdef USE_A2DP_PEER_NAME
            a2dp_sink_.set_peer_name_callback(
                [](char* name) {
                    if (g_a2dp_hub_instance != nullptr) {
                        g_a2dp_hub_instance->peer_name_callback(name);
                    }
                }
            );
#endif

#ifdef USE_A2DP_RSSI
            /*
             * RSSI callback
             */
            a2dp_sink_.set_rssi_active(true);
            a2dp_sink_.set_rssi_callback(
                [](esp_bt_gap_cb_param_t::read_rssi_delta_param& rssi) {
                    if (g_a2dp_hub_instance != nullptr) {
                        g_a2dp_hub_instance->rssi_callback(rssi);
                    }
                }
            );
#endif

#ifdef USE_A2DP_VOLUME
            /*
             * Volume change callback
             */
            a2dp_sink_.set_avrc_rn_volumechange(
                [](int volume) {
                    if (g_a2dp_hub_instance != nullptr) {
                        g_a2dp_hub_instance->avrc_rn_volumechange_callback(volume);
                    }
                }
            );

            // Same callback to nofity when the local volume change has been completed (after the remote device has acknowledged it)
            a2dp_sink_.set_avrc_rn_volumechange_completed(
                [](int volume) {
                    if (g_a2dp_hub_instance != nullptr) {
                        g_a2dp_hub_instance->avrc_rn_volumechange_callback(volume);
                    }
                }
            );
#endif

            ESP_LOGW(TAG, "%s", "A2DPSink is initialized");
        }

        void A2DPSinkHub::start() {
            a2dp_sink_.start(this->name().c_str(), this->auto_reconnect());
        }

        void A2DPSinkHub::stop() {
            a2dp_sink_.end();
        }

        void A2DPSinkHub::loop()
        {
        }

        BluetoothA2DPSink* A2DPSinkHub::a2dp_sink() const {
            return &a2dp_sink_;
        }

        void A2DPSinkHub::set_volume(uint8_t volume) {
            ESP_LOGD(TAG, "Setting volume to: %d", volume);
            a2dp_sink_.set_volume(volume);
        }

        void A2DPSinkHub::set_stream_reader(void (*callBack)(const uint8_t*, uint32_t)) {
            a2dp_sink_.set_stream_reader(callBack, false);
        }

        void A2DPSinkHub::stop_a2dp() {
            a2dp_sink_.stop();
        }

        void A2DPSinkHub::pause_a2dp() {
            a2dp_sink_.pause();
        }

        void A2DPSinkHub::play_a2dp() {
            a2dp_sink_.play();
        }

        void A2DPSinkHub::next_track() {
            a2dp_sink_.next();
        }

        void A2DPSinkHub::prev_track() {
            a2dp_sink_.previous();
        }

        esp_a2d_connection_state_t A2DPSinkHub::get_connection_state() {
            return a2dp_sink_.get_connection_state();
        }

        void A2DPSinkHub::set_connected(bool connected) {
            a2dp_sink_.set_connected(connected);
        }

        const char* A2DPSinkHub::get_peer_name() {
            return a2dp_sink_.get_peer_name();
        }

        esp_bd_addr_t* A2DPSinkHub::get_current_peer_address() {
            return a2dp_sink_.get_current_peer_address();
        }

        void A2DPSinkHub::dump_config()
        {
            ESP_LOGCONFIG(TAG, "A2DP Sink");
        }

        std::string A2DPSinkHub::bd_addr_to_string(const esp_bd_addr_t& addr) {
            char buf[18]; // "XX:XX:XX:XX:XX:XX\0" = 17 chars max
            snprintf(buf, sizeof(buf), ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(addr));
            return std::string(buf);
        }



    } // namespace a2dp_sink
} // namespace esphome
