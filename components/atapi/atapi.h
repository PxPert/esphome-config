#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace atapi {

class Atapi : public i2c::I2CDevice, public PollingComponent {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  void reset_all();
  inline bool is_device_ready() { return device_ready; }
  // ##################################
  // Auxiliary functions User Interface
  // ##################################

  void play();
  void stop();
  void eject();
  void load();
  void pause();
  void resume();
  void stop_disc();
  inline void next() { goto_track(a_trck + 1); }
  inline void previous(){goto_track(a_trck - 1);}
  inline void restart_track(){goto_track(a_trck);}
  void goto_track(uint8_t trck);


 private:
    // Program Variables
    uint8_t dataLval;                     // dataLval and dataHval hold data from/to
    uint8_t dataHval;                     // D0-D15 of IDE
    uint8_t regval;                       // regval holds addr. of reg. to be addressed on IDE
    uint8_t reg;                          // Holds the addr. of the IDE register with adapted
                                      // nDIOR/nDIOW/nRST values to suit purpose.
    uint8_t cnt;                          // packet uint8_t counter
    uint8_t idx;                          // index used as pointer within packet array
    uint8_t paclen = 12;                  // Default packet length
    uint8_t s_trck;                       // Holds start track
    uint8_t e_trck;                       // Holds end track
    uint8_t c_trck;                       // Follows current track while reading TOC
    uint8_t c_trck_m;                     // MSF values for current track
    uint8_t c_trck_s;
    uint8_t c_trck_f;
    uint8_t a_trck = 1;                   // Holds actual track from reading subchannel data
    uint8_t MFS_M;                        // Holds actual M value from reading subchannel data
    uint8_t MFS_S;                        // Holds actual S value from reading subchannel data
    uint8_t d_trck;                       // Destination track
    uint8_t d_trck_m;                     // MSF values for destination track
    uint8_t d_trck_s;
    uint8_t d_trck_f;
    uint8_t aud_stat = 0xFF;              // subchannel data: 0x11=play, 0x12=pause, 0x15=stop
    uint8_t asc;
    long prev_millis=0;
    long interval=100;
    bool toc;
    bool device_ready;

    // Array containing sets of 16 byte packets corresponding to part of the CD-ROM
    // ATAPI function set. If the IDE device only supports packets with 12 byte length
    // the last 4 bytes are not sent. The great majority of tested devices use 12 byte.
    uint8_t fnc[192]= {
      0x1B,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=0 Open tray
      0x1B,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=16 Close tray
      0x1B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=32 Stop unit
      0x47,0x00,0x00,0x10,0x28,0x05,0x4C,0x1A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=48 Start PLAY
      0x4B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=64 PAUSE play
      0x4B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=80 RESUME play
      0x43,0x02,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=96 Read TOC
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=112 unit ready
      0x5A,0x00,0x01,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=128 mode sense
      0x42,0x02,0x40,0x01,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=144 rd subch.
      0x03,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=160 req. sense
      0x4E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  // idx=176 Stop disk
    };



    void disp_cd_data();

    void curr_MSF();

    // ###########################
    // Auxiliary functions PCF8475
    // ###########################

    // Set to high impedance all ports of PCF8475 interfacing to IDE.
    void highZ();

    // Reset Device
    void reset_IDE();

    // Read one word from IDE register
    void readIDE (uint8_t regval);

    // Write one word to IDE register
    void writeIDE (uint8_t regval, uint8_t dataLval, uint8_t dataHval);

    // #################################################
    // Auxiliary functions ATAPI Status Register related
    // #################################################

    // Wait for BSY clear
    void BSY_clear_wait();

    // Wait for DRQ clear
    void DRQ_clear_wait();

    // Wait for DRQ set
    void DRQ_set_wait();

    // Wait for DRY set
    void DRY_set_wait();

    // ##################################
    // Auxiliary functions Packet related
    // ##################################

    // Send a packet starting at fnc array position idx
    void SendPac();

    void get_TOC();
    void read_TOC();
    void read_subch_cmd();
    uint8_t chck_disk();
    void unit_ready();
    void req_sense();
    void init_task_file();

};


}  // namespace atapi
}  // namespace esphome
