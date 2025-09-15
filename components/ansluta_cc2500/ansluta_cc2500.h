#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace ansluta_cc2500 {

class EH_RL_SPI : public esphome::spi::SPIDevice<esphome::spi::BIT_ORDER_MSB_FIRST,esphome::spi::CLOCK_POLARITY_LOW,
                            esphome::spi::CLOCK_PHASE_LEADING,esphome::spi::DATA_RATE_2MHZ> {};

class AnslutaCC2500Component : public Component, public EH_RL_SPI {
  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    void radioInit();

    void setLight(uint8_t state);

    void add_on_remote_click_callback(std::function<void(uint8_t)> &&callback){
      this->on_remote_click_callback_.add(std::move(callback));
    }
  private:
    bool _setupComplete = false;

    void enableRead();
    void writeReg(uint8_t addr, uint8_t value);
    void flushRx();
    uint8_t getMARCSTATE();
    uint8_t readReg(const uint8_t regAddr, const uint8_t regType);
    bool readRXFIFO(uint8_t len);
    uint8_t readRemoteCommand();
    void sendCommand(byte AddressByteA, byte AddressByteB, byte Command);
    uint8_t getRXBYTES();
    uint8_t sendStrobe(uint8_t strobe);

    CallbackManager<void(uint8_t)> on_remote_click_callback_{};

};


}  // namespace empty_spi_component
}  // namespace esphome
