#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include <queue>

#define MAX_TRACKS 99

namespace esphome {
namespace atapi {

const uint8_t atapi_fnc_open_tray[16]       = {0x1B, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Open tray
const uint8_t atapi_fnc_close_tray[16]      = {0x1B, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Close tray
const uint8_t atapi_fnc_stop_unit[16]       = {0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Stop unit
const uint8_t atapi_fnc_start_play[16]      = {0x47, 0x00, 0x00, 0x10, 0x28, 0x05, 0x4C, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Start PLAY
const uint8_t atapi_fnc_pause_play[16]      = {0x4B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // PAUSE play
const uint8_t atapi_fnc_resume_play[16]     = {0x4B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // RESUME play
const uint8_t atapi_fnc_read_toc[16]        = {0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Read TOC
const uint8_t atapi_fnc_unit_ready[16]      = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // unit ready
const uint8_t atapi_fnc_mode_sense[16]      = {0x5A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // mode sense
const uint8_t atapi_fnc_read_subchannel[16] = {0x42, 0x02, 0x40, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // rd subch.
const uint8_t atapi_fnc_request_sense[16]   = {0x03, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // req. sense
const uint8_t atapi_fnc_stop_disk[16]       = {0x4E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Stop disk

typedef std::function<bool(uint8_t)> AsyncAtapiCommand;
typedef std::pair<const char*, AsyncAtapiCommand > AsyncAtapiCommandPair;

class AudioTrack {
public:
  uint8_t minutes;
  uint8_t seconds;
  uint8_t frames;
};

class Atapi : public i2c::I2CDevice, public PollingComponent {
 public:


  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  inline bool is_device_ready() { return _device_ready; }
  // ##################################
  // Auxiliary functions User Interface
  // ##################################


  void enqueue_reset();



  void enqueue_play();
  void enqueue_stop();
  void enqueue_eject();
  void enqueue_load();
  void enqueue_pause();
  void enqueue_resume();
  void enqueue_stop_disc();

  bool set_command(const char* name, AsyncAtapiCommand cmd);
  inline void enqueue_next() { enqueue_goto_track(_current_track + 1); }
  inline void enqueue_previous(){enqueue_goto_track(_current_track - 1);}
  inline void enqueue_restart_track(){enqueue_goto_track(_current_track);}
  void enqueue_goto_track(uint8_t trck);

  void add_on_state_callback(std::function<void(int)> &&callback){
    this->state_callback_.add(std::move(callback));
  }
  void add_on_update_callback(std::function<void()> &&callback){
    this->update_callback_.add(std::move(callback));
  }
  void add_on_toc_callback(std::function<void()> &&callback){
    this->toc_callback_.add(std::move(callback));
  }
  void add_on_lock_callback(std::function<void(bool)> &&callback){
    this->lock_callback_.add(std::move(callback));
  }
  void add_on_error_callback(std::function<void(int)> &&callback){
    this->error_callback_.add(std::move(callback));
  }

  uint8_t get_tracks() {
    return _total_tracks;
  }

  uint8_t get_current_track() {
    return _current_track;
  }

  uint16_t get_total_time() {
    return 0;
    // return (fnc[54] * 60) + fnc[55];
  }

  uint16_t get_current_time() {
    return 0;
    // return (MFS_M * 60) + MFS_S;
  }

  uint16_t get_current_track_time() {
    return 0;
    // return get_current_time() - ((fnc[51] * 60) + fnc[52]);
  }

  uint8_t get_disc_state() {
    return _disc_state;
  }

  uint8_t get_status() {
    switch (_audio_status) {
      case 0x00: // No disc
        return 0;
      case 0x15: // Stopped
        return 1;
      case 0x11: // Playing
        return 2;
      case 0x12: // Paused
        return 3;
    }
    return 0;
  }


 protected:
  CallbackManager<void(int)> state_callback_{};
  CallbackManager<void()> update_callback_{};
  CallbackManager<void()> toc_callback_{};
  CallbackManager<void(bool)> lock_callback_{};
  CallbackManager<void(int)> error_callback_{};

 private:
  AsyncAtapiCommandPair _currentCommand;
  uint8_t _currentCommandStep;
  unsigned long _currentFunction_call_time;
  bool _currentfunction_first_try;
  void set_next_command_step();

  bool cmd_reset(uint8_t step);
  bool cmd_check_disk(uint8_t step);
  bool cmd_read_subch_cmd(uint8_t step);
  bool cmd_get_toc(uint8_t step);


  /* Internal substep commands */
  bool async_delay(unsigned int delay, unsigned long start_millis);
  bool async_delay(unsigned int delay);
  bool unit_ready(bool firstCall);
  bool req_sense(bool firstCall);
  bool sendPac(const uint8_t* packet, bool firstCall);

  // #################################################
  // Auxiliary functions ATAPI Status Register related
  // #################################################

  // Wait for DRQ set
  bool DRQ_set_wait_async();

  // Wait for DRY set
  bool DRY_set_wait_async();
  // Wait for DRQ clear
  bool DRQ_clear_wait_async();

  // Wait for BSY clear
  bool BSY_clear_wait_async();


  AudioTrack _tracks[MAX_TRACKS];
  AudioTrack _end_position;
  AudioTrack _current_track_position;
  uint8_t _start_track;
  uint8_t _total_tracks;
  uint8_t _current_track;
  uint8_t _disc_state;
  uint8_t _additional_sense_code;
  uint8_t _packet_length = 12;                  // Default packet length
  uint8_t _audio_status = 0xFF;              // subchannel data: 0x11=play, 0x12=pause, 0x15=stop
  bool _toc_read;
  bool _device_ready;


  // ###########################
  // Auxiliary functions PCF8475
  // ###########################

  // Set to high impedance all ports of PCF8475 interfacing to IDE.
  void highZ();

  // Reset Device
  void reset_IDE();

  // Read one word from IDE register
  void readIDE (uint8_t regval, uint8_t* toDataLval, uint8_t* toDataHval);

  // Write one word to IDE register
  void writeIDE (uint8_t regval, uint8_t dataLval, uint8_t dataHval);


  void enqueue_get_TOC();
  void enqueue_read_subch_cmd();
  void enqueue_check_disk();

};


}  // namespace atapi
}  // namespace esphome
