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

// LiquidCrystal_I2C lcd(LCD_I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

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


static Atapi* atapi_ = nullptr;

void Atapi::setup() {
  atapi_ = this;
  ESP_LOGCONFIG(TAG, "Empty I2C component3");
  _device_ready = false;

}
bool Atapi::set_command(const char* name, AsyncAtapiCommand cmd) {
  _currentCommand.first = name;
  _currentCommand.second = cmd;
  _currentCommandStep = 0;
  _currentFunction_call_time = 0;
  _currentfunction_first_try = true;
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
  }

}

void Atapi::dump_config(){
    ESP_LOGCONFIG(TAG, "Atapi dump_config");
//    reset_all();
}



void Atapi::update() {
  if (
      (! _device_ready)
    ||
      (_currentCommand.first)
  )
  {
    return;
  }

  ESP_LOGCONFIG(TAG, "Polling...");
  enqueue_check_disk();

}


void Atapi::enqueue_goto_track(uint8_t trck) {
#ifdef TODO
  a_trck = trck;
  if(a_trck > _total_tracks){(a_trck = _start_track);}      // over last track? -> point to start track
  ESP_LOGI(TAG,"Requested track: %d - going to: %d/%d", trck, a_trck, _total_tracks);
  reset_queue();
  // enqueue_get_TOC();                                   // Get MSF for a_trck
  _loopfunctions.emplace(AsyncFunction( "enqueue_goto_track",[&]() {
//    fnc[51] = d_trck_m;                          // Store new play start position
//    fnc[52] = d_trck_s;                          // in play packet and start play
//    fnc[53] = d_trck_f;
    enqueue_play();
    if((_audio_status == 0x12) |                        // If paused or stopped -> pause
        (_audio_status == 0x15))
      {
      enqueue_pause();
    }
    return true;
  }));
#endif
}


bool Atapi::async_delay(unsigned int delay) {
  return async_delay(delay, _currentFunction_call_time);
}

bool Atapi::async_delay(unsigned int delay, unsigned long start_millis) {
  if (millis() - start_millis > delay) {
    ESP_LOGD(TAG,"async_delay done");
  }
  return  (millis() - _currentFunction_call_time > delay);
}

void Atapi::set_next_command_step() {
  _currentCommandStep++;
  _currentFunction_call_time = millis();
  _currentfunction_first_try = true;
}

bool Atapi::cmd_reset(uint8_t step) {
  bool cmd_complete = false;
  if (step == 0) {
    _device_ready = false;
    this->status_clear_error();
    error_callback_.call(0);

    ESP_LOGCONFIG(TAG, "Setting up ports expander...");
    highZ();
    reset_IDE();                              // Do hard reset
    set_next_command_step();
  }

  if (step == 1) {
    if (async_delay(3000)) {
      set_next_command_step();
    }
  }

  if (step == 2) {
    ESP_LOGD(TAG, "BSY_clear_wait_async...");
    if (BSY_clear_wait_async()) {
      ESP_LOGD(TAG, "BSY_clear_wait_async done");
      set_next_command_step();
    }
  }

  if (step == 3) {
    if (DRY_set_wait_async()) {
      ESP_LOGD(TAG, "DRY_set_wait_async done");
      set_next_command_step();
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
    set_next_command_step();
  }

  if (step == 5) {
    writeIDE(ErrFReg, 0x00, 0xFF);            // Set Feature register = 0 (no overlapping and no DMA)
    writeIDE(CylHReg, 0x02, 0xFF);            // Set PIO buffer to max. transfer length (= 200h)
    writeIDE(CylLReg, 0x00, 0xFF);
    writeIDE(AStCReg, 0x02, 0xFF);            // Set nIEN, we don't care about the INTRQ signal
    set_next_command_step();

  }

  if (step == 6) {
    if (BSY_clear_wait_async()) {
      set_next_command_step();
    }
  }

  if (step == 7) {
    if (DRQ_clear_wait_async()) {
      set_next_command_step();
    }
  }

  if (step == 8) {
    if (async_delay(3000)) {
      set_next_command_step();
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
    set_next_command_step();
  }

  if (step == 10) {
    if (async_delay(3000)) {
      set_next_command_step();
    }
  }


  if (step == 11) {
    writeIDE (ComSReg, 0xA1, 0xFF);           // Issue Identify Device Command
    set_next_command_step();
  }

  if (step == 12) {
    if (async_delay(500)) {
      set_next_command_step();
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
    set_next_command_step();
  }

  if (step == 14) {
    if (DRQ_clear_wait_async()) {
      set_next_command_step();
    }
  }

  if (step == 15) {
    if (unit_ready(_currentfunction_first_try)) {
      set_next_command_step();
    }
  }

  if (step == 16) {
    if (req_sense(_currentfunction_first_try)) {
      set_next_command_step();
    }
  }

  if (step == 17) {
    if (_additional_sense_code != 0x04) {
      ESP_LOGI(TAG, "Reset complete");
      _device_ready = true;
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
//    reset_queue();
                                                  // pointer to play function and Play
//    enqueue_sendPac(48);                          // from MSF location stored at idx=(51-56)
}                                                 // See also doc. sff8020i table 76

void Atapi::enqueue_stop(){
//    reset_queue();
//    enqueue_sendPac(32);                          // pointer to stop unit function
}
void Atapi::enqueue_eject(){
//    reset_queue();
//    enqueue_sendPac(0);                          // pointer to eject function
}
void Atapi::enqueue_load(){
//    reset_queue();
//    enqueue_sendPac(16);                          // pointer to load
}
void Atapi::enqueue_pause(){
//    reset_queue();
//    enqueue_sendPac(64);                          // pointer to hold
}
void Atapi::enqueue_resume(){
//    reset_queue();
//    enqueue_sendPac(80);                          // pointer to resume
}
void Atapi::enqueue_stop_disc(){
//    reset_queue();
//    enqueue_sendPac(176);                          // pointer to stop disk function
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
    if (async_delay(400, internal_millis)) {
      internal_step++;
    }
  }

  if (internal_step == 2) {
    for (uint8_t i=0;i<_packet_length;i=i+2){        // Send packet with length of '_packet_length'
      writeIDE(DataReg, packet[i], packet[i + 1]);
      readIDE(AStCReg,nullptr,nullptr);                         // Read alternate stat reg.
      readIDE(AStCReg,nullptr,nullptr);                         // Read alternate stat reg.
    }
    return true;
  }


  return false;

}

bool Atapi::cmd_get_toc(uint8_t step){
  // Pointer to Read TOC Packet
  // Send read TOC command packet
  if (step == 0) {
    if (sendPac(atapi_fnc_read_toc, _currentfunction_first_try)) { // used to check_unit_ready
      set_next_command_step();
    }
  }

  if (step == 1) {
    if (async_delay(10)) {
      set_next_command_step();
    }
  }

  if (step == 2) {
    if (DRQ_set_wait_async()) {
      set_next_command_step();
    }
  }



  if (step == 3) {
    uint8_t lVal, hVal;

    readIDE(DataReg, nullptr, nullptr);                      // TOC Data Length not needed, don't care
    readIDE(DataReg,&lVal, &hVal);                      // Read first and last session
    _start_track = lVal;
    _total_tracks = hVal;
    set_next_command_step();
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
        _tracks[current_track] = track;
      } else if (current_track == 0xAA) {
        _end_position = track;
      }
      readIDE(ComSReg, &lVal, nullptr);
    } while (lVal & (1<<3));
    set_next_command_step();
  }

  if (step == 5) { // TOC Read correctly
    _toc_read=true;
    ESP_LOGD(TAG,"TOC Read ok. Tracks: %d/%d - Time: %d:%d",_start_track,_total_tracks,_end_position.minutes,_end_position.seconds);
    toc_callback_.call();
    return true;
  }

  return false;


}

bool Atapi::cmd_read_subch_cmd(uint8_t step) {
  if (step == 0) {
    if (sendPac(atapi_fnc_read_subchannel, _currentfunction_first_try)) { // used to check_unit_ready
      ESP_LOGD(TAG,"Packet read. Proceeding");
      set_next_command_step();
    }
  }

  if (step == 1) {
    uint8_t lVal, hVal;

    readIDE(DataReg, &lVal, &hVal);                      // Get Audio Status
    ESP_LOGD(TAG,"Packet value: %d / %d, %d", lVal, hVal, hVal == 0x15);

    if(hVal==0x13){                                      // Play operation successfully completed
      hVal=0x15;                                         // means drive is neither paused nor in play
    }                                                    // so treat as stopped

    if((hVal==0x11)|                                     // playing
        (hVal==0x12)|                                    // paused
        (hVal==0x15))                                    // stopped
    {
      ESP_LOGD(TAG,"SETTING HVAL WITH REQ VALUE");
      _audio_status=hVal;                                  //
    }else{
      ESP_LOGD(TAG,"hVal value %d not recognided", hVal);
      _audio_status=0;                                      // all other values will report "NO DISC"
    }

    readIDE(DataReg,nullptr,nullptr);                    // Get (ignore) Subchannel Data Length
    readIDE(DataReg,nullptr,nullptr);                    // Get (ignore) Format Code, ADR and Control
    readIDE(DataReg,&lVal,nullptr);                    // Get actual track

    _current_track = lVal;
    readIDE(DataReg,nullptr,&hVal);                    // Get M field of actual MFS data and
    _current_track_position.minutes = hVal;                    // store M it
    readIDE(DataReg,&lVal,nullptr);                    // get S and F fields
    _current_track_position.seconds = lVal;                    // Store S value

    set_next_command_step();
  }

  if (step == 2) {
    ESP_LOGD(TAG,"Step 2");
    uint8_t lVal;

    readIDE(DataReg, nullptr, nullptr);
    readIDE(ComSReg, &lVal, nullptr);
    if (! (lVal & (1<<3))) {          // Read rest of data from Data Reg. until DRQ=0
      set_next_command_step();
    }
  }

  if (step == 3) {
    ESP_LOGD(TAG,"Step 3. audio status: %d, toc read: %d", _audio_status, _toc_read);
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
    } else {
      return true;
    }

  }

  return false;

}

bool Atapi::cmd_check_disk(uint8_t step) {
  if (step == 0) {
    if (sendPac(atapi_fnc_mode_sense, _currentfunction_first_try)) { // used to check_unit_ready
      set_next_command_step();
    }
  }

  if (step == 1) {
    if (async_delay(10)) {
      set_next_command_step();
    }
  }

  if (step == 2) {
    if (DRQ_set_wait_async()) {
      set_next_command_step();
    }
  }

  if (step == 3) {
    uint8_t lVal;

    _disc_state = 0;
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
      _disc_state = 1;
    } else if (lVal == 0x71){                      // Note if door open
      ESP_LOGD(TAG, "Door open");
      _disc_state = 2;
    } else {
      ESP_LOGD(TAG, "No disc");
    }

    set_next_command_step();

  }

  if (step == 4) {
    uint8_t lVal;

    readIDE(DataReg,nullptr,nullptr);
    readIDE(ComSReg,&lVal,nullptr);
    if (! (lVal & (1<<3))) {          // Read rest of data from Data Reg. until DRQ=0
      set_next_command_step();
    }
  }

  if (step == 5) {
    ESP_LOGD(TAG,"LAST STEP. disc state: %d", _disc_state);
    if (_disc_state == 1) { // Disc inserted, set next command to read TOC
      ESP_LOGD(TAG,"Disc present.. replacing with enqueue_read_subch_cmd");
      enqueue_read_subch_cmd();
    } else {
      return true;
    }
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
  set_command("CheckDisk", [this](uint8_t val) { return cmd_check_disk(val); });
}

void Atapi::enqueue_read_subch_cmd(){
  set_command("ReadSubch", [this](uint8_t val) { return cmd_read_subch_cmd(val); });
}

void Atapi::enqueue_reset(){
  set_command("Reset", [this](uint8_t val) { return cmd_reset(val); });
}

void Atapi::enqueue_get_TOC(){
  set_command("getToc", [this](uint8_t val) { return cmd_get_toc(val); });
}























}  // namespace atapi
}  // namespace esphome
