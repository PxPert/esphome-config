#include "esphome/core/log.h"
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
  highZ();
  disp_cd_data();

}

void Atapi::loop() {

}

void Atapi::dump_config(){
    ESP_LOGCONFIG(TAG, "Empty I2C component");
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

void Atapi::play(){
    idx = 48;                                     // pointer to play function and Play
    SendPac();                                    // from MSF location stored at idx=(51-56)
}                                                 // See also doc. sff8020i table 76
void Atapi::stop(){
    idx = 32;                                     // pointer to stop unit function
    SendPac();
}
void Atapi::eject(){
    idx = 0;                                      // pointer to eject function
    SendPac();
}
void Atapi::load(){
    idx = 16;                                     // pointer to load
    SendPac();
}
void Atapi::pause(){
     idx = 64;                                    // pointer to hold
     SendPac();
}
void Atapi::resume(){
     idx = 80;                                    // pointer to resume
     SendPac();
}
void Atapi::stop_disk(){
    idx = 176;                                    // pointer to stop disk function
    SendPac();
}

// ###########################
// Auxiliary functions PCF8475
// ###########################

// Set to high impedance all ports of PCF8475 interfacing to IDE.
void Atapi::highZ(){
  #ifdef TODO

  Wire.beginTransmission(RegSel);       // address IDE Register interface
  Wire.write((uint8_t)255);                       // queue FFh into buffer for setting all pins HIGH
  Wire.endTransmission();               // transmit buffered data to IDE Register interface
  Wire.beginTransmission(DataH);        // address IDE DD8-DD15
  Wire.write((uint8_t)255);                       // as above
  Wire.endTransmission();               //
  Wire.beginTransmission(DataL);        // address IDE DD0-DD7
  Wire.write((uint8_t)255);                       // as above
  Wire.endTransmission();               //
#endif
}

// Reset Device
void Atapi::reset_IDE(){
  #ifdef TODO

  Wire.beginTransmission(RegSel);
  Wire.write((uint8_t)B11011111);                // Bit 5 LOW to reset IDE via nRESET
  Wire.endTransmission();
  delay(40);
  Wire.beginTransmission(RegSel);
  Wire.write((uint8_t)B11111111);                // Release reset
  Wire.endTransmission();
  delay(20);
#endif

}

// Read one word from IDE register
void Atapi::readIDE (uint8_t regval){
  #ifdef TODO

  reg = regval & B01111111;             // set nDIOR bit LOW preserving register address
  Wire.beginTransmission(RegSel);
  Wire.write((uint8_t)reg);
  Wire.endTransmission();
  Wire.requestFrom(DataH, 1);
  dataHval = Wire.read();
  Wire.requestFrom(DataL, 1);
  dataLval = Wire.read();
  highZ();                              // set all I/O pins to HIGH -> impl. nDIOR release
#endif
}

// Write one word to IDE register
void Atapi::writeIDE (uint8_t regval, uint8_t dataLval, uint8_t dataHval){
  #ifdef TODO

  reg = regval | B01000000;             // set nDIOW bit HIGH preserving register address
  Wire.beginTransmission(RegSel);
  Wire.write((uint8_t)reg);
  Wire.endTransmission();
  Wire.beginTransmission(DataH);        // send data for IDE D8-D15
  Wire.write((uint8_t)dataHval);
  Wire.endTransmission();
  Wire.beginTransmission(DataL);        // send data for IDE D0-D7
  Wire.write((uint8_t)dataLval);
  Wire.endTransmission();
  reg = regval & B10111111;             // set nDIOW LOW preserving register address
  Wire.beginTransmission(RegSel);
  Wire.write((uint8_t)reg);
  Wire.endTransmission();
  highZ();                              // All I/O pins to high impedance -> impl. nDIOW release
#endif
}

// #################################################
// Auxiliary functions ATAPI Status Register related
// #################################################

// Wait for BSY clear
void Atapi::BSY_clear_wait(){
  #ifdef TODO

  do{
    readIDE(ComSReg);
  } while(dataLval & (1<<7));
#endif
}

// Wait for DRQ clear
void Atapi::DRQ_clear_wait(){
  #ifdef TODO

  do{
    readIDE(ComSReg);
  } while(dataLval & (1<<3));
#endif
}

// Wait for DRQ set
void Atapi::DRQ_set_wait(){
  #ifdef TODO

     do{
        readIDE(ComSReg);
     }while((dataLval & ~(1<<3)) == true);
#endif
}

// Wait for DRY set
void Atapi::DRY_set_wait(){
#ifdef TODO

     do{
        readIDE(ComSReg);
     }while((dataLval & ~(1<<6)) == true);
#endif
}

// ##################################
// Auxiliary functions Packet related
// ##################################

// Send a packet starting at fnc array position idx
void Atapi::SendPac(){
  #ifdef TODO

     writeIDE (AStCReg, B00001010, 0xFF);     // Set nIEN before you send the PACKET command!
     writeIDE(ComSReg, 0xA0, 0xFF);           // Write Packet Command Opcode
     delay(400);
     for (cnt=0;cnt<paclen;cnt=cnt+2){        // Send packet with length of 'paclen'
     dataLval = fnc[(idx + cnt)];             // to IDE Data Registeraccording to idx value
     dataHval = fnc[(idx + cnt + 1)];
     writeIDE(DataReg, dataLval, dataHval);
     readIDE(AStCReg);                         // Read alternate stat reg.
     readIDE(AStCReg);                         // Read alternate stat reg.
     }
     BSY_clear_wait();
#endif
}

void Atapi::get_TOC(){
  #ifdef TODO

       idx =  96;                             // Pointer to Read TOC Packet
       SendPac();                             // Send read TOC command packet
       delay(10);
       DRQ_set_wait();
       read_TOC();                            // Fetch result
#endif
}

void Atapi::read_TOC(){
  #ifdef TODO

        readIDE(DataReg);                      // TOC Data Length not needed, don't care
        readIDE(DataReg);                      // Read first and last session
        s_trck = dataLval;
        e_trck = dataHval;
        do{
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
        } while(dataLval & (1<<3));            // Read data from DataRegister until DRQ=0
#endif
}

void Atapi::read_subch_cmd(){
#ifdef TODO

        idx=144;                             // Pointer to read Subchannel Packet
        SendPac();                           // Send read Subchannel command packet
        readIDE(DataReg);                    // Get Audio Status
        if(dataHval==0x13){                  // Play operation successfully completed
          dataHval=0x15;                     // means drive is neither paused nor in play
        }                                    // so treat as stopped
        if(dataHval==0x11|                   // playing
           dataHval==0x12|                   // paused
           dataHval==0x15)                   // stopped
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
        do{
          readIDE(DataReg);
          readIDE(ComSReg);
        } while(dataLval & (1<<3));          // Read rest of data from Data Reg. until DRQ=0
#endif
}

uint8_t Atapi::chck_disk(){
  #ifdef TODO

     uint8_t disk_ok = 0xFF;                        // assume no valid disk present.
     idx = 128;                                  // Send mode sense packet
     SendPac();                                  //
     delay(10);
     DRQ_set_wait();                             // Wait for data ready to read.
     readIDE(DataReg);                           // Read and discard Mode Sense data length
     readIDE(DataReg);                           // Get Medium Type byte
                                                 // If valid audio disk present disk_ok=0x00
     if (dataLval == 0x02 |
         dataLval == 0x06 |
         dataLval == 0x12 |
         dataLval == 0x16 |
         dataLval == 0x22 |
         dataLval == 0x26)
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
#else
     return 0;
#endif
}

void Atapi::unit_ready(){                                // Reuests unit to report status
    #ifdef TODO
        idx=112;                                  // used to check_unit_ready
        SendPac();
#endif
}

void Atapi::req_sense(){                                 // Request Sense Command is used to check
  #ifdef TODO

  idx=160;                                        // the result of the Unit Ready command.
  SendPac();                                      // The Additional Sense Code is used,
  delay(10);                                      // see table 71 in sff8020i documentation
  DRQ_set_wait();
  cnt=0;
  do{
       readIDE(DataReg);
       if (cnt == 6){
           asc=dataLval;                          // Store Additional Sense Code
       }
       cnt++;
       readIDE(AStCReg);
       readIDE(ComSReg);
     } while(dataLval & (1<<3));                  // Skip rest of packet
#endif
}

void Atapi::init_task_file(){
  #ifdef TODO
  writeIDE(ErrFReg, 0x00, 0xFF);            // Set Feature register = 0 (no overlapping and no DMA)
  writeIDE(CylHReg, 0x02, 0xFF);            // Set PIO buffer to max. transfer length (= 200h)
  writeIDE(CylLReg, 0x00, 0xFF);
  writeIDE(AStCReg, 0x02, 0xFF);            // Set nIEN, we don't care about the INTRQ signal
  BSY_clear_wait();                         // When conditions are met then IDE bus is idle,
  DRQ_clear_wait();                         // this check may not be necessary (???)
#endif
}



























}  // namespace atapi
}  // namespace esphome
