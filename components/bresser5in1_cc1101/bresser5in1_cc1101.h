#pragma once

#define CC_MAX_BUF 64                  // for cc1101 FIFO, variable is better to revised

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace bresser5in1_cc1101 {

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

class EH_RL_SPI : public esphome::spi::SPIDevice<esphome::spi::BIT_ORDER_MSB_FIRST,esphome::spi::CLOCK_POLARITY_LOW,
                            esphome::spi::CLOCK_PHASE_LEADING,esphome::spi::DATA_RATE_2MHZ> {};

class Bresser5in1CC1101Component : public Component, public EH_RL_SPI {
  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    void set_rx_pin(InternalGPIOPin *rx_pin) { _gd0_rx = rx_pin; }

    void add_on_state_callback(std::function<void(const BresserReading*)> &&callback){
      this->state_callback_.add(std::move(callback));
    }

    void set_temperature(sensor::Sensor *temperature) { temperature_ = temperature; }
    void set_battery_sensor(binary_sensor::BinarySensor *sensor) { this->battery_sensor_ = sensor; }

  private:
    sensor::Sensor *temperature_{nullptr};
    binary_sensor::BinarySensor *battery_sensor_{nullptr};


    uint8_t _ccBuf[CC_MAX_BUF];             // for cc1101 FIFO, if Circuit board for more cc110x -> ccBuf expand ( ccBuf[radionr][CC_MAX_BUF] )
    volatile bool _gpioChanged = false;
    bool _setupComplete = false;
    InternalGPIOPin* _gd0_rx=nullptr;


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

    static void handleInterrupt(Bresser5in1CC1101Component* component);

    CallbackManager<void(const BresserReading*)> state_callback_{};


};


}  // namespace empty_spi_component
}  // namespace esphome
