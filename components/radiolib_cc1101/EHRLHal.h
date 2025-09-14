#ifndef EHRLHHAL_H
#define EHRLHHAL_H

#define RADIOLIB_LOW_LEVEL 1

#include "esphome/components/spi/spi.h"

#include <RadioLib.h>

// pretty much from EspHal.h / ArduinoHal.h
// define Arduino-style macros
#ifndef USE_ARDUINO
#define LOW                         (0x0)
#define HIGH                        (0x1)
#define INPUT                       (0x01)
#define OUTPUT                      (0x03)
#define RISING                      (0x01)
#define FALLING                     (0x02)
#endif

class EH_RL_SPI : public esphome::spi::SPIDevice<esphome::spi::BIT_ORDER_MSB_FIRST,esphome::spi::CLOCK_POLARITY_LOW, 
                            esphome::spi::CLOCK_PHASE_LEADING,esphome::spi::DATA_RATE_2MHZ> {};

class EH_RL_Hal : public RadioLibHal {
    public:
    // default constructor - initializes the base HAL and any needed private members
    EH_RL_Hal(EH_RL_SPI* spi)
      : RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING), spi(spi) {}

    void init() override {
    }

    void term() override {
    }

    void inline delay(RadioLibTime_t ms) override {
        esphome::delay(ms);
    }

    void inline delayMicroseconds(RadioLibTime_t us) override {
        esphome::delayMicroseconds(us);
    }

    unsigned long inline millis() override {
      return((unsigned long)esphome::millis());
    }

    unsigned long inline micros() override {
        return((unsigned long)esphome::micros());
    }

    void spiBegin() override {
      // ESPHome will do it with SPIDevice
    }
    void spiEnd() override {
      // ESPHome will do it with SPIDevice
    }
    void inline spiBeginTransaction() override {
      spi->enable();
    }
    void inline spiEndTransaction() override {
      spi->disable();
    }

    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override {
      for(size_t i = 0; i < len; i++) {
        in[i] = spi->transfer_byte(out[i]);
      }
    }

    esphome::spi::SPIDevice<esphome::spi::BIT_ORDER_MSB_FIRST,esphome::spi::CLOCK_POLARITY_LOW, 
                            esphome::spi::CLOCK_PHASE_LEADING,esphome::spi::DATA_RATE_2MHZ>* spi;

    void inline yield() override {
      ::esphome::yield();
    }

    // this is *hopefully* temporary --- I should be able to provide this hal constructor a sparse pin mapping
    // to the esphome GPIOPin (assuming there is a way to get it via id()).  Another slight challenge may
    // be with interrupts... we'll see.  As it is --- for just using this with esphome remote_receiver and 
    // remote_transmitter - only SPI stuff above is necessary - but there is alot more to radiolib that would
    // be nice to use with esphome and homeassistant :-)

    // GPIO-related methods (pinMode, digitalWrite etc.) should check
    // RADIOLIB_NC as an alias for non-connected pins
    void inline pinMode(uint32_t pin, uint32_t mode ) override {
        return;
    }

    void inline digitalWrite(uint32_t pin, uint32_t value) override {
        return;
    }

    uint32_t inline digitalRead(uint32_t pin) override {
        return 0;
    }

    void inline attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
        return;
    }

    void inline detachInterrupt(uint32_t interruptNum) override {
        return;
    }

    long inline pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) override {
        return 0;
    }

};


#endif
