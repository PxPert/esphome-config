#pragma once

#include "esphome/core/component.h"
#include "BluetoothA2DPSink.h"
#include <atomic>


namespace esphome
{
    namespace a2dp_sink
    {

        namespace a2dp_sink_priority {
        // AFTER_WIFI so the hub runs after the wifi/ethernet drivers are up and we can read the active
        // interface's MAC for client_id.
        inline constexpr float HUB = esphome::setup_priority::AFTER_WIFI;
        inline constexpr float CHILD = HUB - 1.0f;
        }  // namespace sendspin_priority

        enum class A2DPSinkTextMetadataTypes {
            TITLE = ESP_AVRC_MD_ATTR_TITLE,
            ARTIST = ESP_AVRC_MD_ATTR_ARTIST,
            ALBUM = ESP_AVRC_MD_ATTR_ALBUM,
            GENRE = ESP_AVRC_MD_ATTR_GENRE,
            TRACKNUM = ESP_AVRC_MD_ATTR_TRACK_NUM,
            PLAYINGTIME = ESP_AVRC_MD_ATTR_PLAYING_TIME,
            NUM_TRACKS = ESP_AVRC_MD_ATTR_NUM_TRACKS,
            PEERNAME,
            PEERADDR,
            TRACKPOSITION,
            RSSI,
        };

        class A2DPSinkMetadata
        {
        public:
            A2DPSinkMetadata(uint8_t type, const uint8_t *text) {
                type_ = type;
                text_ = text;
            }

            uint8_t type() const { return type_; }
            const uint8_t *text() const { return text_; }

        private:
            uint8_t type_;
            const uint8_t *text_;
        };

        class A2DPSinkHub : public Component
        {
        public:
            float get_setup_priority() const override { return a2dp_sink_priority::HUB; }
            void setup() override;
            void loop() override;
            void dump_config() override;

            void set_name(std::string &&v) { this->name_ = std::move(v); }
            std::string& name() { return this->name_; };

            void set_auto_reconnect(bool v) { this->auto_reconnect_ = v; }
            bool auto_reconnect() { return this->auto_reconnect_; };

            template<typename F> void add_metadata_update_callback(F &&callback) {
                this->metadata_update_callbacks_.add(std::forward<F>(callback));
            }

            template<typename F> void add_playback_status_callbacks(F &&callback) {
                this->playback_status_callbacks_.add(std::forward<F>(callback));
            }

            template<typename F> void add_playback_position_callbacks(F &&callback) {
                this->playback_position_callbacks_.add(std::forward<F>(callback));
            }

            template<typename F> void add_connection_state_callbacks(F &&callback) {
                this->connection_state_callbacks_.add(std::forward<F>(callback));
            }

            template<typename F> void add_peer_name_callback(F &&callback) {
                this->peer_name_callbacks_.add(std::forward<F>(callback));
            }

            template<typename F> void add_rssi_callback(F &&callback) {
                this->rssi_callbacks_.add(std::forward<F>(callback));
            }

            BluetoothA2DPSink* a2dp_sink() const;

            void start();

            void stop();

            /// Converts an esp_bd_addr_t (6-byte Bluetooth address) to a human-readable
            /// string like "AA:BB:CC:DD:EE:FF".
            static std::string bd_addr_to_string(const esp_bd_addr_t& addr);

        protected:
            std::string name_;
            bool auto_reconnect_;

            // Metadata Callback
            void avrc_metadata_callback(uint8_t id, const uint8_t *text);

            // Playback status callbacks
            void avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback);
            void avrc_rn_play_pos_callback(uint32_t play_pos);
            void on_connection_state_changed(esp_a2d_connection_state_t state, void *user_data);

            void peer_name_callback(const char* name);

            void rssi_callback(esp_bt_gap_cb_param_t::read_rssi_delta_param& rssi);

            // Callback fan-out to child components; they filter as needed
            CallbackManager<void(const A2DPSinkMetadata &)> metadata_update_callbacks_{};

            CallbackManager<void(esp_avrc_playback_stat_t playback)> playback_status_callbacks_{};
            CallbackManager<void(uint32_t pos)> playback_position_callbacks_{};

            CallbackManager<void(esp_a2d_connection_state_t, void*)> connection_state_callbacks_{};

            CallbackManager<void(const char*)> peer_name_callbacks_{};

            CallbackManager<void(esp_bt_gap_cb_param_t::read_rssi_delta_param&)> rssi_callbacks_{};


        }; // class A2DPSinkHub

        /// @brief Base class for all sendspin subcomponents.
        ///
        /// Consolidates the Component + Parented<A2DPSinkHub> inheritance and pins the setup
        /// priority so the hub's setup() always runs before any child. Subcomponents should
        /// inherit from this instead of listing Component/Parented individually and must not
        /// override get_setup_priority().
        class A2DPSinkChild : public Component, public Parented<A2DPSinkHub> {
        public:
            float get_setup_priority() const override { return a2dp_sink_priority::CHILD; }

        };


    } // namespace a2dp_sink
} // namespace esphome
