#pragma once

#define CC_MAX_BUF 64                  // for cc1101 FIFO, variable is better to revised

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"


#define RADIOLIB_LOW_LEVEL 1



#include "EHRLHal.h"

namespace esphome {
namespace radiolib_cc1101 {

struct BresserReading {
  uint16_t sensor_id;
  float temperature;
  int humidity;
  float wind_direction_deg;
  float wind_gust;
  float wind_avg;
  float rain;
  int battery_ok;
};

enum CC1101_state {CC1101_NOINIT,CC1101_STANDBY,CC1101_RECV,CC1101_XMIT};
enum CC1101Modulation {OOK_MODULATION=0, FSK_MODULATION};

class RadiolibCC1101Component : public Component, public EH_RL_SPI {
  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    void set_rx_pin(InternalGPIOPin *rx_pin) { _gd0_rx = rx_pin; }



    int standby();
    int xmit();
    int recv();
    void set_tx_pin(InternalGPIOPin *tx_pin) { _gd0_tx = tx_pin; }
    void set_frequency(float freq) { _freq=freq/1e6; }
    void set_modulation(CC1101Modulation modulation) { _modulation=modulation; }
    void set_filter(float filter) { _bandwidth=filter/1e3; }
    void set_bitrate(float bitrate) { _bitrate=bitrate; }
    void set_reg_agcctrl0(uint8_t reg_agcctrl0) { _REG_AGCCTRL0 = reg_agcctrl0; }
    void set_reg_agcctrl1(uint8_t reg_agcctrl1) { _REG_AGCCTRL1 = reg_agcctrl1; }
    void set_reg_agcctrl2(uint8_t reg_agcctrl2) { _REG_AGCCTRL2 = reg_agcctrl2; }
    void set_registers();
    void setup_direct_mode();
    float getRSSI();

    EH_RL_Hal* hal;
    CC1101 radio=NULL;
    int init_state=0;
    CC1101_state state=CC1101_NOINIT;
    float _freq=433.92;
    CC1101Modulation _modulation=OOK_MODULATION;
    float _bandwidth=464;
    
    // these are bandwidth specific
    u_int8_t _REG_FREND1=0xb6;
    u_int8_t _REG_TEST2=0x88;
    u_int8_t _REG_TEST1=0x31;
    u_int8_t _REG_FIFOTHR=0x07;

    // AGC settings from: LSatan/SmartRC-CC1101-Driver-Lib
    // They _generally_ seem to work well
    // AGCCTRL2[7:6] reduce maximum available DVGA gain: disable top three gain setting
    // AGCCTRL2[2:0] average amplitude target for filter: 42 dB
    // AGCCTRL1[6:6] LNA priority setting: LNA2 first
    u_int8_t _REG_AGCCTRL2=0xc7;
    u_int8_t _REG_AGCCTRL1=0x00;
    u_int8_t _REG_AGCCTRL0=0xb2;

  // datarate seems to be very crtitical to be around 5.0k sometimes, but 40k or 100k seems to make more sense
  // -- intertwined with AGC settings from smartRC below
  // see also DN022: https://www.ti.com/lit/an/swra215e/swra215e.pdf
    float _bitrate=5;

    InternalGPIOPin* _gd0_tx=nullptr;

    // For RSSI rx average
    float last_rx_rssi=0;

  protected:
    uint8_t _ccBuf[CC_MAX_BUF];             // for cc1101 FIFO, if Circuit board for more cc110x -> ccBuf expand ( ccBuf[radionr][CC_MAX_BUF] )
    volatile bool _gpioChanged = false;
    bool _setupComplete = false;

  private:
    InternalGPIOPin* _gd0_rx=nullptr;
    ISRInternalGPIOPin _gd0_rx_isr=nullptr; // Used by ISR
    uint8_t bresser_5in1_decode();

    bool readRXFIFO(uint8_t start,uint8_t len);                             // xFSK
    uint8_t checkParity(const byte*);
    uint8_t getRXBYTES();
    uint8_t getRSSIdev();


    void radioInit();
    void setReceiveMode();
    bool flushrx();
    void writeCfg();
    void writeCfg(const char* IB_1);
    void commandStrobes(const char* IB_1);
    void writeCCreg(uint8_t reg, uint8_t var);
    void writePatable();
    uint8_t readReg(const uint8_t regAddr, const uint8_t regType);  // read CC1101 register via SPI
    void writeReg(const uint8_t regAddr, const uint8_t val);        // write single register into the CC1101 IC via SPI
    uint8_t waitTo_Miso();
    uint8_t cmdStrobe(const uint8_t cmd);
    uint8_t cmdStrobeTo(const uint8_t cmd);
    uint8_t getMARCSTATE();                                         // xFSK
    uint8_t populateBuffer();

    static void handleInterrupt(RadiolibCC1101Component* component);



    void adjustBW(float bandwidth); // rx filter bw snapper



};


}  // namespace empty_spi_component
}  // namespace esphome
