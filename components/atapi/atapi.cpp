#include "esphome/core/log.h"
#include "esphome/components/i2c/i2c_bus.h"

#include "atapi.h"

namespace esphome {
namespace atapi {

static const char *TAG = "atapi.component";


// I/O expander addresses:
static const int DataL = 0x20;            // IDE DD0-DD7
static const int DataH = 0x21;            // IDE DD8-DD15
static const int RegSel = 0x22;           // IDE register

/* Note that the 'pins' in the following LCD definitions are not IC pins numbers
but rather specify the PCF8574 I/O port numbers: P0 to P7. E.g. LCD 'En' pin is
connected to port P2 of PCF8574 so En_pin = 2. */

static const uint8_t BACKLIGHT_PIN = 3;
static const uint8_t En_pin = 2;
static const uint8_t Rw_pin = 1;
static const uint8_t Rs_pin = 0;
static const uint8_t D4_pin = 4;
static const uint8_t D5_pin = 5;
static const uint8_t D6_pin = 6;
static const uint8_t D7_pin = 7;

// IDE Register addresses
static const uint8_t DataReg = 0xF0;         // Addr. Data register of IDE device.
static const uint8_t ErrFReg = 0xF1;         // Addr. Error/Feature (rd/wr) register of IDE device.
static const uint8_t SecCReg = 0xF2;         // Addr. Sector Count register of IDE device.
static const uint8_t SecNReg = 0xF3;         // Addr. Sector Number register of IDE device.
static const uint8_t CylLReg = 0xF4;         // Addr. Cylinder Low register of IDE device.
static const uint8_t CylHReg = 0xF5;         // Addr. Cylinder High register of IDE device.
static const uint8_t HeadReg = 0xF6;         // Addr. Device/Head register of IDE device.
static const uint8_t ComSReg = 0xF7;         // Addr. Command/Status (wr/rd) register of IDE device.
static const uint8_t AStCReg = 0xEE;         // Addr. Alternate Status/Device Control (rd/wr) register of IDE device.


void Atapi::setup() {
  ESP_LOGCONFIG(TAG, "CD Reader setup");
  _device_ready = 0;
  _busy_status = 0;
  _disc_state = DISC_STATUS_NODISC;
  _audio_status = AUDIOSTATUS_NODISC;

}

bool Atapi::dequeue_command() {
  if (_enqueuedCommand.first) {
    _currentCommand.first = _enqueuedCommand.first;
    _currentCommand.second = _enqueuedCommand.second;
    _currentCommandStep = 0;
    _currentFunction_call_time = 0;
    _currentfunction_first_try = true;
    _enqueuedCommand.first = nullptr;
    return true;
  }

  // No command in queue
  return false;
}

bool Atapi::enqueue_command(const char* name, AsyncAtapiCommand cmd) {
  _enqueuedCommand.first = name;
  _enqueuedCommand.second = cmd;
  return true;

}
void Atapi::loop() {

  if (_currentCommand.first) {
    if (_currentFunction_call_time == 0) {
      _currentfunction_first_try = true;
      _currentFunction_call_time = millis();
    } else {
      _currentfunction_first_try = false;
    }
    AsyncAtapiCommand c = _currentCommand.second;
    if (c(_currentCommandStep)) {
      _currentCommand.first = nullptr;
      _currentCommandStep = 0;
      _currentFunction_call_time = 0;
    }
  } else {
    dequeue_command();
  }


}

void Atapi::set_busy_status(uint8_t busy_status) {
  if (_busy_status != busy_status) {
    ESP_LOGI(TAG,"Set busy status to %d", busy_status);
    _busy_status = busy_status;
    lock_callback_.call(_busy_status);
  }
}

void Atapi::dump_config(){
    ESP_LOGCONFIG(TAG, "Atapi dump_config");
//    reset_all();
}



void Atapi::update() {
  if (
      (_device_ready < 2)
    ||
      (_currentCommand.first)
  )
  {
    ESP_LOGD(TAG, "Device busy, skip polling");
    return;
  }

  ESP_LOGD(TAG, "Polling... Busy state: %d - Disc state: %d - Play state: %d", _busy_status, _disc_state, get_status());
  enqueue_check_disk();

}

bool Atapi::cmd_play_track(uint8_t step) {
  static uint8_t atapi_fnc_start_play[16] = {0x47, 0x00, 0x00, 0x10, 0x28, 0x05, 0x4C, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  // Start PLAY
  // from from MSF location stored at indexes 3 to 8.
  // See also doc. sff8020i table 76

  if (step == 0) {
    set_busy_status(BUSYSTATUS_PLAY);
    if (_requested_position > 0) {
      atapi_fnc_start_play[3] = (_tracks[_requested_track].toSeconds() + _requested_position) / 60;
      atapi_fnc_start_play[4] = (_tracks[_requested_track].toSeconds() + _requested_position) % 60;
      atapi_fnc_start_play[5] = 0;
    } else {
      atapi_fnc_start_play[3] = _tracks[_requested_track].minutes;
      atapi_fnc_start_play[4] = _tracks[_requested_track].seconds;
      atapi_fnc_start_play[5] =  (_requested_position > 0)?0:_tracks[_requested_track].frames;
    }
    atapi_fnc_start_play[6] = _end_position.minutes;
    atapi_fnc_start_play[7] = _end_position.seconds;
    atapi_fnc_start_play[8] = _end_position.frames;

    ESP_LOGD(TAG,"Set play command to this array: %02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d:%02d",
             atapi_fnc_start_play[0],
             atapi_fnc_start_play[1],
             atapi_fnc_start_play[2],
             atapi_fnc_start_play[3],
             atapi_fnc_start_play[4],
             atapi_fnc_start_play[5],
             atapi_fnc_start_play[6],
             atapi_fnc_start_play[7],
             atapi_fnc_start_play[8],
             atapi_fnc_start_play[9],
             atapi_fnc_start_play[10],
             atapi_fnc_start_play[11],
             atapi_fnc_start_play[12],
             atapi_fnc_start_play[13],
             atapi_fnc_start_play[14],
             atapi_fnc_start_play[15]
    );
    step = set_next_command_step();
  }

  if (step == 1) {
    if (sendPac(atapi_fnc_start_play, _currentfunction_first_try)) {
      ESP_LOGD(TAG,"Play command complete");
      step = set_next_command_step();
    }
  }

  if (step == 2) {
    ESP_LOGD(TAG,"Play command delay complete");
    return true;
  }

  return false;

}

bool Atapi::async_delay(unsigned int delay) {
  return async_delay(delay, _currentFunction_call_time);
}

bool Atapi::async_delay(unsigned int delay, unsigned long start_millis) {
//  if (millis() - start_millis > delay) {
//    ESP_LOGD(TAG,"async_delay done");
//  }
  return  (millis() - _currentFunction_call_time > delay);
}

uint8_t Atapi::set_next_command_step() {
  _currentCommandStep++;
  _currentFunction_call_time = millis();
  _currentfunction_first_try = true;
  return _currentCommandStep;

}

bool Atapi::cmd_reset(uint8_t step) {
  bool cmd_complete = false;

  set_busy_status(BUSYSTATUS_RESET);

  if (step == 0) {
    _device_ready = 1;
    this->status_clear_error();
    error_callback_.call(0);

    ESP_LOGCONFIG(TAG, "Setting up ports expander...");
    highZ();
    reset_IDE();                              // Do hard reset
    step = set_next_command_step();
  }

  if (step == 1) {
    if (async_delay(3000)) {
      step = set_next_command_step();
    }
  }

  if (step == 2) {
    ESP_LOGD(TAG, "BSY_clear_wait_async...");
    if (BSY_clear_wait_async()) {
      ESP_LOGD(TAG, "BSY_clear_wait_async done");
      step = set_next_command_step();
    }
  }

  if (step == 3) {
    if (DRY_set_wait_async()) {
      ESP_LOGD(TAG, "DRY_set_wait_async done");
      step = set_next_command_step();
    }
  }

  if (step == 4) {
    uint8_t lVal;
    readIDE(CylLReg, &lVal, nullptr);

    if(lVal == 0x14){
      readIDE(CylHReg, &lVal, nullptr);
      if(lVal == 0xEB){
          ESP_LOGCONFIG(TAG, "Found ATAPI Device");
      }
    }else{
          ESP_LOGCONFIG(TAG, "No ATAPI Device!");
          this->status_set_error("No ATAPI Device!");
          error_callback_.call(255);
          return true;

    }
    writeIDE(HeadReg, 0x00, 0xFF);            // Set Device to Master (Device 0)
    step = set_next_command_step();
  }

  if (step == 5) {
    writeIDE(ErrFReg, 0x00, 0xFF);            // Set Feature register = 0 (no overlapping and no DMA)
    writeIDE(CylHReg, 0x02, 0xFF);            // Set PIO buffer to max. transfer length (= 200h)
    writeIDE(CylLReg, 0x00, 0xFF);
    writeIDE(AStCReg, 0x02, 0xFF);            // Set nIEN, we don't care about the INTRQ signal
    step = set_next_command_step();

  }

  if (step == 6) {
    if (BSY_clear_wait_async()) {
      step = set_next_command_step();
    }
  }

  if (step == 7) {
    if (DRQ_clear_wait_async()) {
      step = set_next_command_step();
    }
  }

  if (step == 8) {
    if (async_delay(3000)) {
      step = set_next_command_step();
    }
  }

  if (step == 9) {
    ESP_LOGCONFIG(TAG, "Self Diag. ");

    writeIDE(ComSReg, 0x90, 0xFF);            // Issue Run Self Diagnostic Command
    uint8_t lVal;
    readIDE(ErrFReg, &lVal, nullptr);

    if(lVal == 0x01){
      ESP_LOGCONFIG(TAG, "OK");
    }else{
      ESP_LOGE(TAG, "Self diag fail. Read value: %d",lVal);            // Units failing this may still work fine
      this->status_set_error("Self diag fail.");
      error_callback_.call(1);
      return true;
    }
    step = set_next_command_step();
  }

  if (step == 10) {
    if (async_delay(3000)) {
      step = set_next_command_step();
    }
  }


  if (step == 11) {
    writeIDE (ComSReg, 0xA1, 0xFF);           // Issue Identify Device Command
    step = set_next_command_step();
  }

  if (step == 12) {
    if (async_delay(500)) {
      step = set_next_command_step();
    }
  }

  if (step == 13) {
    uint8_t lVal, hVal;
    uint8_t i = 0;
    do{
      readIDE(DataReg, &lVal, &hVal);
      if (i == 0){                                // Get supported packet lenght
        if(lVal & (1<<0)){                      // contained in lower byte of first word
          _packet_length = 16;                              // 1st bit set -> use 16 byte packets
        }
      }
      if((i > 26) & (i < 47)){                      // Read Model
        ESP_LOGI(TAG, "Model: %d-%d",lVal,hVal);
      }
      i++;
      readIDE(ComSReg, &lVal, nullptr);             // Read Status Register and check DRQ,
    } while(lVal & (1<<3));                         // skip rest of data until DRQ=0
    readIDE(AStCReg,nullptr,nullptr);
    step = set_next_command_step();
  }

  if (step == 14) {
    if (DRQ_clear_wait_async()) {
      step = set_next_command_step();
    }
  }

  if (step == 15) {
    if (unit_ready(_currentfunction_first_try)) {
      step = set_next_command_step();
    }
  }

  if (step == 16) {
    if (req_sense(_currentfunction_first_try)) {
      step = set_next_command_step();
    }
  }

  if (step == 17) {
    if (_additional_sense_code != 0x04) {
      ESP_LOGI(TAG, "Reset complete");
      _device_ready = 2;
      return true;
    } else {
      ESP_LOGI(TAG, "Still resetting");
      step = 15; // Go back to step 15
    }
  }

  return false;
}

// ##################################
// Auxiliary functions User Interface
// ##################################

void Atapi::enqueue_play(){
  enqueue_play_track(_start_track);
}

void Atapi::enqueue_stop(){
  if (get_status() > AUDIOSTATUS_STOPPED) {
    enqueue_command("stop_unit", [this](uint8_t step) {
      if (step == 0) {
       if (sendPac(atapi_fnc_stop_unit, _currentfunction_first_try)) {
          step = set_next_command_step();
        }
      }

      if (step == 1) {
        _audio_status = 0x15;
        state_callback_.call(get_status());
        return true;
      }

      return false;
    });
  }
}
void Atapi::enqueue_eject(){
  enqueue_command("eject", [this](uint8_t step) {
    set_busy_status(BUSYSTATUS_EJECT);
    if (sendPac(atapi_fnc_open_tray, _currentfunction_first_try)) {
      return true;
    }
    return false;
  });
}
void Atapi::enqueue_load(){
  enqueue_command("load", [this](uint8_t step) {
    set_busy_status(BUSYSTATUS_LOAD);
    if (sendPac(atapi_fnc_close_tray, _currentfunction_first_try)) {
      return true;
    }
    return false;
  });
}
void Atapi::enqueue_pause(){
  enqueue_command("pause", [this](uint8_t step) {
    return sendPac(atapi_fnc_pause_play, _currentfunction_first_try);
  });
}
void Atapi::enqueue_resume(){
  enqueue_command("resume", [this](uint8_t step) {
    return sendPac(atapi_fnc_resume_play, _currentfunction_first_try);
  });
}
void Atapi::enqueue_stop_disc(){
  enqueue_command("stop_disc", [this](uint8_t step) {
    return sendPac(atapi_fnc_stop_disk, _currentfunction_first_try);
  });
}

// ###########################
// Auxiliary functions PCF8475
// ###########################

// Set to high impedance all ports of PCF8475 interfacing to IDE.
void Atapi::highZ() {
  static const uint8_t highv_setting = (uint8_t)255;
  esphome::i2c::ErrorCode e;

//  ESP_LOGD(TAG, "highZ sending");
  e = bus_->write(RegSel, &highv_setting, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "highZ RegSel error: %d", e);
    this->status_set_error("highZ RegSel error");
    error_callback_.call(2);
    return;
  }
  e = bus_->write(DataH, &highv_setting, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "highZ DataH error: %d", e);
    this->status_set_error("highZ DataH error");
    error_callback_.call(3);
    return;
  }
  e = bus_->write(DataL, &highv_setting, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "highZ DataL error: %d", e);
    this->status_set_error("highZ DataL error");
    error_callback_.call(4);
    return;
  }
//  ESP_LOGD(TAG, "highZ sent");

}

// Reset Device
void Atapi::reset_IDE(){
  static const uint8_t bit5_low = (uint8_t)B11011111; // Bit 5 LOW to reset IDE via nRESET
  static const uint8_t bit5_high = (uint8_t)B11111111; // Bit 5 HIGH to release reset

  _toc_read = false;

  esphome::i2c::ErrorCode e = bus_->write(RegSel, &bit5_low, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "reset_IDE low error: %d", e);
    this->status_set_error("reset_IDE low error");
    error_callback_.call(5);
    return;
  }
  delay(40);
  e = bus_->write(RegSel, &bit5_high, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "reset_IDE high error: %d", e);
    this->status_set_error("reset_IDE high error");
    error_callback_.call(6);

    return;
  }
  delay(20);
}

// Read one word from IDE register
void Atapi::readIDE (uint8_t regval, uint8_t* toDataLval, uint8_t* toDataHval){
  uint8_t reg = regval & B01111111;     // set nDIOR bit LOW preserving register address

  esphome::i2c::ErrorCode e = bus_->write(RegSel, &reg, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "readIDE RegSel error: %d", e);
    this->status_set_error("readIDE RegSel error");
    error_callback_.call(7);
    return;
  }

  if (toDataHval) {
    e = bus_->read(DataH, toDataHval, 1);

    if (e != esphome::i2c::ERROR_OK) {
      ESP_LOGE(TAG, "readIDE DataH error: %d", e);
      this->status_set_error("readIDE DataH error");
      error_callback_.call(8);
      return;
    }
  }

  if (toDataLval) {
    e = bus_->read(DataL, toDataLval, 1);
    if (e != esphome::i2c::ERROR_OK) {
      this->status_set_error("readIDE DataL error");
      error_callback_.call(9);
      return;
    }
  }

  highZ();                              // set all I/O pins to HIGH -> impl. nDIOR release

}

// Write one word to IDE register
void Atapi::writeIDE (uint8_t regval, uint8_t dataLval, uint8_t dataHval){
  uint8_t reg = regval | B01000000;             // set nDIOW bit HIGH preserving register address

//  ESP_LOGD(TAG, "writeIDE %d %d %d", reg,dataHval,dataLval);

  esphome::i2c::ErrorCode e = bus_->write(RegSel, &reg, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "writeIDE RegSel1 error: %d", e);
    this->status_set_error("writeIDE RegSel1 error");
    error_callback_.call(10);
    return;
  }

  e = bus_->write(DataH, &dataHval, 1);  // send data for IDE D8-D15
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "writeIDE DataH error: %d", e);
    this->status_set_error("writeIDE DataH error");
    error_callback_.call(11);
    return;
  }

  e = bus_->write(DataL, &dataLval, 1);  // send data for IDE D0-D7
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "writeIDE DataL error: %d", e);
    this->status_set_error("writeIDE DataL error");
    error_callback_.call(12);
    return;
  }

  reg = regval & B10111111;             // set nDIOW LOW preserving register address
  e = bus_->write(RegSel, &reg, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "writeIDE RegSel2 error: %d", e);
    this->status_set_error("writeIDE RegSel2 error");
    error_callback_.call(13);
    return;
  }

  highZ();                              // All I/O pins to high impedance -> impl. nDIOW release

}

// #################################################
// Auxiliary functions ATAPI Status Register related
// #################################################

// Wait for BSY clear
bool Atapi::BSY_clear_wait_async(){
  uint8_t lVal;

  readIDE(ComSReg, &lVal,nullptr);
  return ! (lVal & (1<<7));
}

// Wait for DRQ clear
bool Atapi::DRQ_clear_wait_async(){
  uint8_t lVal;

  readIDE(ComSReg, &lVal,nullptr);
  return ! (lVal & (1<<3));
}

// Wait for DRQ set
bool Atapi::DRQ_set_wait_async(){
  uint8_t lVal;

  readIDE(ComSReg, &lVal,nullptr);
  return ! ((lVal & ~(1<<3)) == true);
}

// Wait for DRY set
bool Atapi::DRY_set_wait_async(){
  uint8_t lVal;

  readIDE(ComSReg, &lVal,nullptr);
  return ! ((lVal & ~(1<<6)) == true);
}

// ##################################
// Auxiliary functions Packet related
// ##################################
bool Atapi::sendPac(const uint8_t* packet, bool firstCall) {
  static uint8_t internal_step = 0;
  static unsigned long internal_millis = millis();

  if (firstCall) {
    internal_step = 0;
    internal_millis = millis();
  }
  if (internal_step == 0) {
     writeIDE (AStCReg, B00001010, 0xFF);     // Set nIEN before you send the PACKET command!
     writeIDE(ComSReg, 0xA0, 0xFF);           // Write Packet Command Opcode
     internal_step++;
     internal_millis = millis();
  }

  if (internal_step == 1) {
    if (async_delay(100, internal_millis)) { // was 400
      internal_step++;
    }
  }

  if (internal_step == 2) {
    for (uint8_t i=0;i<_packet_length;i=i+2){        // Send packet with length of '_packet_length'
      // ESP_LOGD(TAG,"Write ide, %02d:%02d", packet[i], packet[i+1]);
      writeIDE(DataReg, packet[i], packet[i + 1]);
//      uint8_t lVal, hVal;
      readIDE(AStCReg,nullptr,nullptr);                         // Read alternate stat reg.
      // ESP_LOGD(TAG,"RESPONSE 1, %02d:%02d", lVal,hVal);
      readIDE(AStCReg,nullptr,nullptr);                         // Read alternate stat reg.
      // ESP_LOGD(TAG,"RESPONSE 2, %02d:%02d", lVal,hVal);
    }
    internal_step++;
  }

  if (internal_step == 3) {
    return BSY_clear_wait_async();
  }


  return false;

}

bool Atapi::cmd_get_toc(uint8_t step){
  // Pointer to Read TOC Packet
  // Send read TOC command packet
  if (step == 0) {
    if (sendPac(atapi_fnc_read_toc, _currentfunction_first_try)) { // used to check_unit_ready
      step = set_next_command_step();
    }
  }

  if (step == 1) {
    if (async_delay(10)) {
      step = set_next_command_step();
    }
  }

  if (step == 2) {
    if (DRQ_set_wait_async()) {
      step = set_next_command_step();
    }
  }



  if (step == 3) {
    uint8_t lVal, hVal;

    readIDE(DataReg, nullptr, nullptr);                      // TOC Data Length not needed, don't care
    readIDE(DataReg,&lVal, &hVal);                      // Read first and last session
    _start_track = lVal - 1;
    _total_tracks = hVal;
    step = set_next_command_step();
  };

  if (step == 4) {
    uint8_t lVal, hVal, current_track;
    do {
      AudioTrack track;

      readIDE(DataReg,nullptr,nullptr);                    // Skip Session no. ADR and control fields
      readIDE(DataReg,&lVal,nullptr);                              // Read curent track number
      current_track = lVal;
      readIDE(DataReg, nullptr, &hVal);                     // Read M
      track.minutes = hVal;                                     // Store M of curent track
      readIDE(DataReg, &lVal, &hVal);                      // Read S and F
      track.seconds = lVal;                                     // Store S of current track
      track.frames = hVal;                                     // Store F of current track

      if (current_track < MAX_TRACKS) {
        ESP_LOGD(TAG,"Track: %d/(index %d) - Time: %d:%d:%d",current_track,current_track - 1,track.minutes,track.seconds, track.frames);
        _tracks[current_track - 1] = track;
      } else if (current_track == 0xAA) {
        ESP_LOGD(TAG,"End position time: %d:%d:%d",track.minutes,track.seconds, track.frames);
        _end_position = track;
      }
      readIDE(ComSReg, &lVal, nullptr);
    } while (lVal & (1<<3));
    step = set_next_command_step();
  }

  if (step == 5) { // TOC Read correctly
    _toc_read=true;
    ESP_LOGD(TAG,"TOC Read ok. Tracks: %d/%d - Time: %d:%d",_start_track,_total_tracks,_end_position.minutes,_end_position.seconds);
    toc_callback_.call();
    set_busy_status(BUSYSTATUS_NONE);
    return true;
  }

  return false;


}

bool Atapi::cmd_read_subch_cmd(uint8_t step) {
  if (step == 0) {
    if (sendPac(atapi_fnc_read_subchannel, _currentfunction_first_try)) { // used to check_unit_ready
//      ESP_LOGD(TAG,"Packet read. Proceeding");
      step = set_next_command_step();
    }
  }

  if (step == 1) {
    uint8_t lVal, hVal;
    bool ignore = false;

    readIDE(DataReg, &lVal, &hVal);                      // Get Audio Status
//    ESP_LOGD(TAG,"Packet value: %d / %d, %d", lVal, hVal, hVal == 0x15);

    if(hVal==0x13){                                      // Play operation successfully completed
      hVal=0x15;                                         // means drive is neither paused nor in play
    }                                                    // so treat as stopped

    if(hVal==0x10){                                      // hVal Not recognized, ignore
      ignore = true;
    }

    if (!ignore) {
      if((hVal==0x11)|                                     // playing
          (hVal==0x12)|                                    // paused
          (hVal==0x15))                                    // stopped
      {
//        ESP_LOGD(TAG,"SETTING HVAL WITH REQ VALUE");
        _audio_status=hVal;                                  //
      }else{
        ESP_LOGW(TAG,"hVal value %d not recognized", hVal);
        _audio_status=0;                                      // all other values will report "NO DISC"
      }

    }

    readIDE(DataReg,nullptr,nullptr);                    // Get (ignore) Subchannel Data Length
    readIDE(DataReg,nullptr,nullptr);                    // Get (ignore) Format Code, ADR and Control
    readIDE(DataReg,&lVal,nullptr);                      // Get actual track

    if (! ignore) {
      _current_track = lVal - 1;
    }

    readIDE(DataReg,nullptr,&hVal);                      // Get M field of actual MFS data and

    if (! ignore) {
      _current_track_position.minutes = hVal;              // store M it
    }
    readIDE(DataReg,&lVal,nullptr);                      // get S and F fields

    if (! ignore) {
      _current_track_position.seconds = lVal;              // Store S value

      ESP_LOGD(TAG,"Currentt track: %d - Time: %d:%d",_current_track,_current_track_position.minutes,_current_track_position.seconds);
    }

    step = set_next_command_step();
  }

  if (step == 2) {
//    ESP_LOGD(TAG,"Step 2");
    uint8_t lVal;

    readIDE(DataReg, nullptr, nullptr);
    readIDE(ComSReg, &lVal, nullptr);
    if (! (lVal & (1<<3))) {          // Read rest of data from Data Reg. until DRQ=0
      step = set_next_command_step();
    }
  }

  if (step == 3) {
//    ESP_LOGD(TAG,"Step 3. audio status: %d, toc read: %d", _audio_status, _toc_read);
    state_callback_.call(get_status());

    if(_audio_status==0x11){ // Playing
      ESP_LOGD(TAG, "Play");
      update_callback_.call();
    }

    if(_audio_status==0x00){                         // Audio status 0 covers all other posible
      ESP_LOGD(TAG, "No Disc");
      _toc_read = false;
    }

    if ((_audio_status==0x15) && (!_toc_read)) { // Stopped
      enqueue_get_TOC();
    }

    if ((_audio_status == 0x11) || (_busy_status != BUSYSTATUS_PLAY)) {
      set_busy_status(BUSYSTATUS_NONE);
    }

    return true;

  }

  return false;

}

bool Atapi::cmd_check_disk(uint8_t step) {
  if (step == 0) {
    if (sendPac(atapi_fnc_mode_sense, _currentfunction_first_try)) { // used to check_unit_ready
      step = set_next_command_step();
    }
  }

  if (step == 1) {
    if (async_delay(10)) {
      step = set_next_command_step();
    }
  }

  if (step == 2) {
    if (DRQ_set_wait_async()) {
      step = set_next_command_step();
    }
  }

  if (step == 3) {
    uint8_t lVal;

    readIDE(DataReg,nullptr,nullptr);                           // Read and discard Mode Sense data length
    readIDE(DataReg, &lVal, nullptr);                           // Get Medium Type byte

    if ((lVal == 0x02) |
      (lVal == 0x06) |
      (lVal == 0x12) |
      (lVal == 0x16) |
      (lVal == 0x22) |
      (lVal == 0x26))                                           // If valid audio disk present disk_ok=0x00
    {
      ESP_LOGD(TAG, "Disc present");
      _disc_state = DISC_STATUS_DISC_PRESENT;
    } else if (lVal == 0x71){                      // Note if door open
      ESP_LOGD(TAG, "Tray open");
      _disc_state = DISC_STATUS_TRAY_OPENED;
    } else {
      if (! (
          (_disc_state == DISC_STATUS_DISC_PRESENT)
          &&
          (_busy_status == BUSYSTATUS_PLAY)
        ))
        {
        ESP_LOGD(TAG, "No disc");

        _toc_read = false;
        _disc_state = DISC_STATUS_NODISC;
      }
    }

    step = set_next_command_step();

  }

  if (step == 4) {
    uint8_t lVal;

    do {
      readIDE(DataReg,nullptr,nullptr);
      readIDE(ComSReg,&lVal,nullptr);
    } while(lVal & (1<<3));          // Read rest of data from Data Reg. until DRQ=0

    step = set_next_command_step();
  }

  if (step == 5) {
//    ESP_LOGD(TAG,"LAST STEP. disc state: %d", _disc_state);
    if ((_disc_state == DISC_STATUS_DISC_PRESENT) &&
      (
        (_busy_status == BUSYSTATUS_PLAY)
        ||
        (get_status() == AUDIOSTATUS_PLAYING)
        ||
        (! _toc_read)
      )) {
//      ESP_LOGD(TAG,"enqueue_read_subch_cmd");
      enqueue_read_subch_cmd();
    } else {
      set_busy_status(BUSYSTATUS_NONE);
    }
    return true;
  }

  return false;

}


bool Atapi::unit_ready(bool firstCall){               // Reuests unit to report status
  return sendPac(atapi_fnc_unit_ready, firstCall);                     // used to check_unit_ready
}

bool Atapi::req_sense(bool firstCall){                // Request Sense Command is used to check
                                                      // used to check_unit_ready
                                                      // the result of the Unit Ready command.
                                                      // The Additional Sense Code is used,
                                                      // see table 71 in sff8020i documentation
  static uint8_t internal_step = 0;
  static unsigned long internal_millis = millis();

  if (firstCall) {
    internal_step = 0;
    internal_millis = millis();
  }

  if (internal_step == 0) {
    if (sendPac(atapi_fnc_mode_sense,firstCall)) {
      internal_step++;
      internal_millis = millis();
    }
  }

  if (internal_step == 1) {
    if (async_delay(10, internal_millis)) {
      internal_step++;
    }
  }

  if (internal_step == 2) {
    if (DRQ_set_wait_async()) {
      internal_step++;
    }
  }

  if (internal_step == 3) {
    uint8_t lVal;
    uint8_t i = 0;

    do {
    readIDE(DataReg,&lVal,nullptr);
    if (i == 6){
        _additional_sense_code=lVal;                          // Store Additional Sense Code
    }
    i++;
    readIDE(AStCReg,nullptr,nullptr);
    readIDE(ComSReg,&lVal,nullptr);

    } while (lVal & (1<<3));                  // Skip rest of packet
    return true;

  }

  return false;

}

void Atapi::enqueue_check_disk(){
  enqueue_command("CheckDisk", [this](uint8_t val) { return cmd_check_disk(val); });
}

void Atapi::enqueue_read_subch_cmd(){
  enqueue_command("ReadSubch", [this](uint8_t val) { return cmd_read_subch_cmd(val); });
}

void Atapi::enqueue_reset(){
  enqueue_command("Reset", [this](uint8_t val) { return cmd_reset(val); });
}

void Atapi::enqueue_get_TOC(){
  enqueue_command("getToc", [this](uint8_t val) { return cmd_get_toc(val); });
}
void Atapi::enqueue_play_track(uint8_t trck, uint16_t position) {
  if ( (trck < _start_track)  || (trck > (_total_tracks - 1)) ) {
    return;
  }
  _requested_track = trck;
  _requested_position = position;
  enqueue_play_selected_track();

}
void Atapi::enqueue_play_track(uint8_t trck) {
  enqueue_play_track(trck, 0);
}

void Atapi::enqueue_play_selected_track() {
  enqueue_command("play", [this](uint8_t step) { return cmd_play_track(step); });

}

}  // namespace atapi
}  // namespace esphome
