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



void Atapi::setup() {
  ESP_LOGCONFIG(TAG, "Empty I2C component3");
  device_ready = false;

}

void Atapi::loop() {

  if (_currentFunction) {
    if (_currentFunction()) {
      ESP_LOGD(TAG, "Function completed in %d msecs. Processing next loop function. %d still in queue", millis() - _currentFunction_call_time, _loopfunctions.size());
      _currentFunction = nullptr;
    }
    return;
  }

  if (! _loopfunctions.empty()) {
    _currentFunction = _loopfunctions.front();
    _loopfunctions.pop();
    _currentFunction_call_time = millis();
    return;
  }

}

void Atapi::dump_config(){
    ESP_LOGCONFIG(TAG, "Atapi dump_config");
//    reset_all();
}



void Atapi::update() {
  if (
      (! device_ready)
    ||
      (_currentFunction)
    ||
      (_loopfunctions.size())
  )
  {
    return;
  }

  ESP_LOGCONFIG(TAG, "Polling...");
  enqueue_read_subch_cmd();
  _loopfunctions.emplace([&]() {
                            // current audio status and update the display
    if(aud_stat==0x11){                         // accordingly.
  //      lcd.home();
      ESP_LOGD(TAG, "Play");
      curr_MSF();                               // Display pickup position
    }
    if(aud_stat==0x12){
  //      lcd.home();
      ESP_LOGD(TAG, "Pause");
      curr_MSF();
    }
    if((aud_stat==0x15) & !toc){                  // If stopped and TOC invalid
      enqueue_get_TOC();                          // try to read TOC
      _loopfunctions.emplace([&]() {
        disp_cd_data();                           // display TOC data and set TOC valid
        toc=true;                                 // to prevent reading over and over
        return true;
      });
    }
    if(aud_stat==0x00){                         // Audio status 0 covers all other posible
  //      lcd.clear();                              // states not decoded by this sketch and
      ESP_LOGD(TAG, "No Disc");
    }
    return true;
  });

}


void Atapi::enqueue_goto_track(uint8_t trck) {
  a_trck = trck;                               // a_track becomes next track
  if(a_trck > e_trck){(a_trck = s_trck);}      // over last track? -> point to start track
  enqueue_get_TOC();                                   // Get MSF for a_trck
  _loopfunctions.emplace([&]() {
    fnc[51] = d_trck_m;                          // Store new play start position
    fnc[52] = d_trck_s;                          // in play packet and start play
    fnc[53] = d_trck_f;
    enqueue_play();
    if((aud_stat == 0x12) |                        // If paused or stopped -> pause
        (aud_stat == 0x15))
      {
      enqueue_pause();
    }
    return true;
  });
}

bool Atapi::async_delay(unsigned int delay) {
  if (millis() - _currentFunction_call_time > delay) {
    ESP_LOGI(TAG,"async_delay done");
  }
  return  (millis() - _currentFunction_call_time > delay);
}


bool Atapi::reset_all() {

  device_ready = false;
  this->status_clear_error();
  ESP_LOGCONFIG(TAG, "Setting up ports expander...");
  highZ();
  reset_IDE();                              // Do hard reset

  _loopfunctions.emplace([&]() { return async_delay(3000); });
  _loopfunctions.emplace([&]() { return BSY_clear_wait_async(); }); // The ATAPI spec. allows drives to take up to
  _loopfunctions.emplace([&]() { return DRY_set_wait_async();  });  // 31 sec. but all tested where alright within 3s.
  _loopfunctions.emplace([&]() {                                    // Check device signature for ATAPI capability
    readIDE(CylLReg);

    if(dataLval == 0x14){
      readIDE(CylHReg);
      if(dataLval == 0xEB){
          ESP_LOGCONFIG(TAG, "Found ATAPI Device");
      }
    }else{
          ESP_LOGCONFIG(TAG, "No ATAPI Device!");
          this->status_set_error("No ATAPI Device!");
          reset_queue();
          return true;

    }
    writeIDE(HeadReg, 0x00, 0xFF);            // Set Device to Master (Device 0)
    return true;

  });

  // Initialise task file
  // ####################
    ESP_LOGCONFIG(TAG, "init_task_file... ");
    enqueue_init_task_file();

  _loopfunctions.emplace([&]() {return async_delay(3000); });

  _loopfunctions.emplace([&]() {
      // Run Self Diagnostic
      // ###################
      //  lcd.clear ();
      ESP_LOGCONFIG(TAG, "Self Diag. ");


      writeIDE(ComSReg, 0x90, 0xFF);            // Issue Run Self Diagnostic Command
      readIDE(ErrFReg);
      if(dataLval == 0x01){
        ESP_LOGCONFIG(TAG, "OK");
      }else{
        ESP_LOGE(TAG, "Self diag fail. Read value: %d",dataLval);            // Units failing this may still work fine
        this->status_set_error("Self diag fail.");
        reset_queue();
      }
      return true;
    });

  _loopfunctions.emplace([&]() { return async_delay(3000); });

  _loopfunctions.emplace([&]() {
  //  lcd.clear ();
    ESP_LOGCONFIG(TAG, "ATAPI Device:");
  //  lcd.setCursor (0,1);
  // Identify Device
  // ###############
    writeIDE (ComSReg, 0xA1, 0xFF);           // Issue Identify Device Command
    return true;
  });
  _loopfunctions.emplace([&]() {return async_delay(500); });
  _loopfunctions.emplace([&]() {
  //  readIDE(AStCReg); //
    do{
      readIDE(DataReg);
      if (cnt == 0){                                // Get supported packet lenght
        if(dataLval & (1<<0)){                      // contained in lower byte of first word
          paclen = 16;                              // 1st bit set -> use 16 byte packets
        }
      }
      if((cnt > 26) & (cnt < 47)){                      // Read Model
  //        lcd.write(dataHval);
  //        lcd.write(dataLval);
          ESP_LOGCONFIG(TAG, "Data: %d-%d",dataHval,dataLval);
  //        ESP_LOGCONFIG(TAG, dataLval);
      }
      cnt++;
      readIDE(ComSReg);                             // Read Status Register and check DRQ,
    } while(dataLval & (1<<3));                     // skip rest of data until DRQ=0
    readIDE(AStCReg);
    return true;
  });
  _loopfunctions.emplace([&]() {  return DRQ_clear_wait_async(); });

  // Check if unit ready
  // ###################
  enqueue_unit_ready();                                   // Send packet 'test unit ready'
  enqueue_req_sense();                                    // Send packet 'Request Sense'
  _loopfunctions.emplace([&]() {
      if(asc == 0x29){                                    // Req. Sense returns 'HW Reset'
        enqueue_unit_ready();                             // (ASC=29h) at first since we had one.
        enqueue_req_sense();                              // New Req. Sense returns if media
      }
      enqueue_wait_drive();
      return true;
  });

  return true;
}

void Atapi::enqueue_wait_drive(){
  _loopfunctions.emplace([&]() {
    enqueue_unit_ready();                                // Wait until drive is ready.
    enqueue_req_sense();                                 // Some devices take some time
    if (asc != 0x04) {
      ESP_LOGCONFIG(TAG, "Reset complete");
      device_ready = true;
      return true;
    } else {
      return false;
    }
  });
}

void Atapi::disp_cd_data(){                          // Used to display track range and
#ifdef TODO
//     lcd.clear();                              // Total playing time as recovered
     Serial.print("Tracks  ");                    // from reading the TOC
     Serial.print(s_trck, DEC);
     Serial.print("-");
     Serial.println(e_trck, DEC);
//     lcd.setCursor (0,1);
     Serial.print("Time   ");
     Serial.print(fnc[54], DEC);
     Serial.print(":");
      if(fnc[55] < 10)
      {
        Serial.print("0");                         // Print a leading 0 for seconds when below 10
      }
      Serial.println(fnc[55], DEC);
#endif
 }

void Atapi::curr_MSF(){                               // During PLAY or PAUSE operation show the pickup
#ifdef TODO

//      lcd.setCursor (2,1);                     // position as absolute playing time
      Serial.print(a_trck, DEC);
      Serial.print(" - ");
//      lcd.setCursor (11,1);
      Serial.print(MFS_M,DEC);
      Serial.print(":");
      if(MFS_S < 10)
      {
        Serial.print("0");                         // Print a leading 0 for seconds when below 10
      }
      Serial.println(MFS_S,DEC);
#endif
}
// ##################################
// Auxiliary functions User Interface
// ##################################

void Atapi::enqueue_play(){
                                                  // pointer to play function and Play
    enqueue_sendPac(48);                          // from MSF location stored at idx=(51-56)
}                                                 // See also doc. sff8020i table 76

void Atapi::enqueue_stop(){
    enqueue_sendPac(32);                          // pointer to stop unit function
}
void Atapi::enqueue_eject(){
    enqueue_sendPac(0);                          // pointer to eject function
}
void Atapi::enqueue_load(){
    enqueue_sendPac(16);                          // pointer to load
}
void Atapi::enqueue_pause(){
    enqueue_sendPac(64);                          // pointer to hold
}
void Atapi::enqueue_resume(){
    enqueue_sendPac(80);                          // pointer to resume
}
void Atapi::enqueue_stop_disc(){
    enqueue_sendPac(176);                          // pointer to stop disk function
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
    return;
  }
  e = bus_->write(DataH, &highv_setting, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "highZ DataH error: %d", e);
    this->status_set_error("highZ DataH error");
    return;
  }
  e = bus_->write(DataL, &highv_setting, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "highZ DataL error: %d", e);
    this->status_set_error("highZ DataL error");
    return;
  }
//  ESP_LOGD(TAG, "highZ sent");

}

// Reset Device
void Atapi::reset_IDE(){
  static const uint8_t bit5_low = (uint8_t)B11011111; // Bit 5 LOW to reset IDE via nRESET
  static const uint8_t bit5_high = (uint8_t)B11111111; // Bit 5 HIGH to release reset

  esphome::i2c::ErrorCode e = bus_->write(RegSel, &bit5_low, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "reset_IDE low error: %d", e);
    this->status_set_error("reset_IDE low error");
    return;
  }
  delay(40);
  e = bus_->write(RegSel, &bit5_high, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "reset_IDE high error: %d", e);
    this->status_set_error("reset_IDE high error");

    return;
  }
  delay(20);
}

// Read one word from IDE register
void Atapi::readIDE (uint8_t regval){
  uint8_t reg = regval & B01111111;     // set nDIOR bit LOW preserving register address

  esphome::i2c::ErrorCode e = bus_->write(RegSel, &reg, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "readIDE RegSel error: %d", e);
    this->status_set_error("readIDE RegSel error");
    return;
  }

  e = bus_->read(DataH, &dataHval, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "readIDE DataH error: %d", e);
    this->status_set_error("readIDE DataH error");

    return;
  }

  e = bus_->read(DataL, &dataLval, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "readIDE DataL error: %d", e);
    this->status_set_error("readIDE DataL error");
    return;
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
    return;
  }

  e = bus_->write(DataH, &dataHval, 1);  // send data for IDE D8-D15
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "writeIDE DataH error: %d", e);
    this->status_set_error("writeIDE DataH error");
    return;
  }

  e = bus_->write(DataL, &dataLval, 1);  // send data for IDE D0-D7
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "writeIDE DataL error: %d", e);
    this->status_set_error("writeIDE DataL error");
    return;
  }

  reg = regval & B10111111;             // set nDIOW LOW preserving register address
  e = bus_->write(RegSel, &reg, 1);
  if (e != esphome::i2c::ERROR_OK) {
    ESP_LOGE(TAG, "writeIDE RegSel2 error: %d", e);
    this->status_set_error("writeIDE RegSel2 error");
    return;
  }

  highZ();                              // All I/O pins to high impedance -> impl. nDIOW release

}

// #################################################
// Auxiliary functions ATAPI Status Register related
// #################################################

// Wait for BSY clear
bool Atapi::BSY_clear_wait_async(){
  readIDE(ComSReg);
  if (! (dataLval & (1<<7))) {
    ESP_LOGD(TAG,"BSY_clear_wait_async done");
  }
  return ! (dataLval & (1<<7));
}

// Wait for DRQ clear
bool Atapi::DRQ_clear_wait_async(){
  readIDE(ComSReg);
  return ! (dataLval & (1<<3));
}

// Wait for DRQ set
bool Atapi::DRQ_set_wait_async(){
  readIDE(ComSReg);
  return ! ((dataLval & ~(1<<3)) == true);
}

// Wait for DRY set
bool Atapi::DRY_set_wait_async(){
  readIDE(ComSReg);
  return ! ((dataLval & ~(1<<6)) == true);
}

// ##################################
// Auxiliary functions Packet related
// ##################################

// Send a packet starting at fnc array position idx
void Atapi::enqueue_sendPac(uint8_t index /* index used as pointer within packet array */) {
  ESP_LOGD(TAG,"Enqueue pac, index: %d", index);

  _loopfunctions.emplace([&]() {
     writeIDE (AStCReg, B00001010, 0xFF);     // Set nIEN before you send the PACKET command!
     writeIDE(ComSReg, 0xA0, 0xFF);           // Write Packet Command Opcode
     return true;
  });
  _loopfunctions.emplace([&]() { return async_delay(400); });
  _loopfunctions.emplace(std::bind([&](uint8_t lambda_index) {
     ESP_LOGD(TAG,"Send pac, index: %d", lambda_index);
     for (cnt=0;cnt<paclen;cnt=cnt+2){        // Send packet with length of 'paclen'
        dataLval = fnc[(lambda_index + cnt)];             // to IDE Data Registeraccording to idx value
        dataHval = fnc[(lambda_index + cnt + 1)];
        writeIDE(DataReg, dataLval, dataHval);
        readIDE(AStCReg);                         // Read alternate stat reg.
        readIDE(AStCReg);                         // Read alternate stat reg.
     }
     return true;
  },index));

    /*
  _loopfunctions.emplace([&]() {
     ESP_LOGD(TAG,"Send pac, index: %d - cur_idx: %d", index, cur_idx);
     for (cnt=0;cnt<paclen;cnt=cnt+2){        // Send packet with length of 'paclen'
        dataLval = fnc[(cur_idx + cnt)];             // to IDE Data Registeraccording to idx value
        dataHval = fnc[(cur_idx + cnt + 1)];
        writeIDE(DataReg, dataLval, dataHval);
        readIDE(AStCReg);                         // Read alternate stat reg.
        readIDE(AStCReg);                         // Read alternate stat reg.
     }
     return true;
  });
  */
  _loopfunctions.emplace([&]() { return BSY_clear_wait_async();  });
}

void Atapi::enqueue_get_TOC(){
       // Pointer to Read TOC Packet
       // Send read TOC command packet
       enqueue_sendPac(96);
        _loopfunctions.emplace([&]() { return async_delay(10);  });
        _loopfunctions.emplace([&]() { return DRQ_set_wait_async();  });
        _loopfunctions.emplace([&]() {
          readIDE(DataReg);                      // TOC Data Length not needed, don't care
          readIDE(DataReg);                      // Read first and last session
          s_trck = dataLval;
          e_trck = dataHval;
          return true;
        });
        _loopfunctions.emplace([&]() {
           readIDE(DataReg);                   // Skip Session no. ADR and control fields
           readIDE(DataReg);                   // Read curent track number
           c_trck = dataLval;
           readIDE(DataReg);                   // Read M
           c_trck_m = dataHval;                // Store M of curent track
           readIDE(DataReg);                   // Read S and F
           c_trck_s = dataLval;                // Store S of current track
           c_trck_f = dataHval;                // Store F of current track

           if (c_trck == s_trck){              // Store MSF of first track
               fnc[51] = c_trck_m;             //
               fnc[52] = c_trck_s;
               fnc[53] = c_trck_f;
           }
           if (c_trck == a_trck){              // Store MSF of actual track
               d_trck_m = c_trck_m;            //
               d_trck_s = c_trck_s;
               d_trck_f = c_trck_f;
           }
           if (c_trck == 0xAA){                // Store MSF of end position
               fnc[54] = c_trck_m;
               fnc[55] = c_trck_s;
               fnc[56] = c_trck_f;
           }
           readIDE(ComSReg);
           return ! (dataLval & (1<<3));
        }); // Fetch result


}

void Atapi::enqueue_read_subch_cmd(){
  enqueue_sendPac(144); // Pointer to read Subchannel Packet, Send read Subchannel command packet
  _loopfunctions.emplace([&]() {
    readIDE(DataReg);                    // Get Audio Status
    if(dataHval==0x13){                  // Play operation successfully completed
      dataHval=0x15;                     // means drive is neither paused nor in play
    }                                    // so treat as stopped
    if((dataHval==0x11)|                   // playing
        (dataHval==0x12)|                   // paused
        (dataHval==0x15))                   // stopped
        {aud_stat=dataHval;               //
    }else{
        aud_stat=0;                      // all other values will report "NO DISC"
    }

    readIDE(DataReg);                    // Get (ignore) Subchannel Data Length
    readIDE(DataReg);                    // Get (ignore) Format Code, ADR and Control
    readIDE(DataReg);                    // Get actual track

    a_trck = dataLval;
    readIDE(DataReg);                    // Get M field of actual MFS data and
    MFS_M = dataHval;                    // store M it
    readIDE(DataReg);                    // get S and F fields
    MFS_S = dataLval;                    // Store S value
    return true;
  });
  _loopfunctions.emplace([&]() {
    readIDE(DataReg);
    readIDE(ComSReg);
    return ! (dataLval & (1<<3));          // Read rest of data from Data Reg. until DRQ=0
  });
}
/*
uint8_t Atapi::chck_disk(){
     uint8_t disk_ok = 0xFF;                        // assume no valid disk present.
     idx = 128;                                  // Send mode sense packet
     SendPac();                                  //
     delay(10);
     DRQ_set_wait();                             // Wait for data ready to read.
     readIDE(DataReg);                           // Read and discard Mode Sense data length
     readIDE(DataReg);                           // Get Medium Type byte
                                                 // If valid audio disk present disk_ok=0x00
     if ((dataLval == 0x02) |
         (dataLval == 0x06) |
         (dataLval == 0x12) |
         (dataLval == 0x16) |
         (dataLval == 0x22) |
         (dataLval == 0x26))
         {disk_ok = 0x00;
     }
     if (dataLval == 0x71){                      // Note if door open
        disk_ok = 0x71;
     }
     do{                                         // Skip rest of packet
       readIDE(DataReg);
       readIDE(ComSReg);
     } while(dataLval & (1<<3));
     return(disk_ok);
}
*/

void Atapi::enqueue_unit_ready(){                 // Reuests unit to report status
  enqueue_sendPac(112);                           // used to check_unit_ready
}

void Atapi::enqueue_req_sense(){                  // Request Sense Command is used to check
  enqueue_sendPac(160);                           // used to check_unit_ready
                                                  // the result of the Unit Ready command.
                                                  // The Additional Sense Code is used,
                                                  // see table 71 in sff8020i documentation
  _loopfunctions.emplace([&]() { return async_delay(10);  });
  _loopfunctions.emplace([&]() { return DRQ_set_wait_async();  });
  _loopfunctions.emplace([&]() { cnt=0; return true;  });
  _loopfunctions.emplace([&]() {
    readIDE(DataReg);
    if (cnt == 6){
        asc=dataLval;                          // Store Additional Sense Code
    }
    cnt++;
    readIDE(AStCReg);
    readIDE(ComSReg);
    return ! (dataLval & (1<<3));                  // Skip rest of packet
  });

}


void Atapi::enqueue_init_task_file(){
  _loopfunctions.emplace([&]() {
    writeIDE(ErrFReg, 0x00, 0xFF);            // Set Feature register = 0 (no overlapping and no DMA)
    writeIDE(CylHReg, 0x02, 0xFF);            // Set PIO buffer to max. transfer length (= 200h)
    writeIDE(CylLReg, 0x00, 0xFF);
    writeIDE(AStCReg, 0x02, 0xFF);            // Set nIEN, we don't care about the INTRQ signal
    return true;
  });
  _loopfunctions.emplace([&]() { return BSY_clear_wait_async();});                         // When conditions are met then IDE bus is idle,
  _loopfunctions.emplace([&]() { return DRQ_clear_wait_async(); });                         // this check may not be necessary (???)

}



























}  // namespace atapi
}  // namespace esphome
