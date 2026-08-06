#pragma once

#include "esphome/core/component.h"
#include "BluetoothA2DPSink.h"
#include <atomic>
#include <functional>


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

#ifdef USE_A2DP_CONNECTION_STATE
            template<typename F> void add_connection_state_callbacks(F &&callback) {
                this->connection_state_callbacks_.add(std::forward<F>(callback));
            }
#endif

#ifdef USE_A2DP_PLAYBACK_STATUS
            template<typename F> void add_playback_status_callbacks(F &&callback) {
                this->playback_status_callbacks_.add(std::forward<F>(callback));
            }
#endif

#ifdef USE_A2DP_POS
            template<typename F> void add_playback_position_callbacks(F &&callback) {
                this->playback_position_callbacks_.add(std::forward<F>(callback));
            }

            template<typename F> void add_rssi_callback(F &&callback) {
                this->rssi_callbacks_.add(std::forward<F>(callback));
            }
#endif

#ifdef USE_A2DP_METADATA
            template<typename F> void add_metadata_update_callback(F &&callback) {
                this->metadata_update_callbacks_.add(std::forward<F>(callback));
            }
#endif

#ifdef USE_A2DP_PEER_NAME
            template<typename F> void add_peer_name_callback(F &&callback) {
                this->peer_name_callbacks_.add(std::forward<F>(callback));
            }
#endif

#ifdef USE_A2DP_VOLUME
            template<typename F> void add_volume_change_callback(F &&callback) {
                this->volume_change_callbacks_.add(std::forward<F>(callback));
            }
#endif

            BluetoothA2DPSink* a2dp_sink() const;

            void start();

            void stop();

            /// Set the volume of the A2DP sink (0-100)
            void set_volume(uint8_t volume);

            /// Set the stream reader callback for A2DP audio data
            void set_stream_reader(void (*callBack)(const uint8_t*, uint32_t));

            /// Stop the A2DP stream (does not disconnect)
            void stop_a2dp();

            /// Pause the A2DP stream
            void pause_a2dp();

            /// Resume playing the A2DP stream
            void play_a2dp();

            /// Skip to next track
            void next_track();

            /// Skip to previous track
            void prev_track();

            /// Get the current A2DP connection state
            esp_a2d_connection_state_t get_connection_state();

            /// Set the connection state (connect/disconnect)
            void set_connected(bool connected);

            /// Get the peer device name
            const char* get_peer_name();

            /// Get the current peer Bluetooth address
            esp_bd_addr_t* get_current_peer_address();

            /// Converts an esp_bd_addr_t (6-byte Bluetooth address) to a human-readable
            /// string like "AA:BB:CC:DD:EE:FF".
            static std::string bd_addr_to_string(const esp_bd_addr_t& addr);

        protected:
            std::string name_;
            bool auto_reconnect_;

#ifdef USE_A2DP_CONNECTION_STATE
            void on_connection_state_changed(esp_a2d_connection_state_t state, void *user_data);

            CallbackManager<void(esp_a2d_connection_state_t, void*)> connection_state_callbacks_{};
#endif

#ifdef USE_A2DP_PLAYBACK_STATUS
            void avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback);

            CallbackManager<void(esp_avrc_playback_stat_t playback)> playback_status_callbacks_{};
#endif

#ifdef USE_A2DP_POS
            void avrc_rn_play_pos_callback(uint32_t play_pos);

            CallbackManager<void(uint32_t pos)> playback_position_callbacks_{};

#endif

#ifdef USE_A2DP_RSSI
            void rssi_callback(esp_bt_gap_cb_param_t::read_rssi_delta_param& rssi);

            CallbackManager<void(esp_bt_gap_cb_param_t::read_rssi_delta_param&)> rssi_callbacks_{};
#endif

#ifdef USE_A2DP_METADATA
            void avrc_metadata_callback(uint8_t id, const uint8_t *text);

            CallbackManager<void(const A2DPSinkMetadata &)> metadata_update_callbacks_{};

#endif

#ifdef USE_A2DP_PEER_NAME
            void peer_name_callback(const char* name);

            CallbackManager<void(const char*)> peer_name_callbacks_{};
#endif

#ifdef USE_A2DP_VOLUME
            void avrc_rn_volumechange_callback(int volume);

            CallbackManager<void(int)> volume_change_callbacks_{};
#endif


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
