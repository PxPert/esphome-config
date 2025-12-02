#include "esphome/core/log.h"
#include "bresser5in1_cc1101.h"

//based on:
// https://github.com/dbuezas/esphome-cc1101
// and https://github.com/NorthernMan54/rtl_433_ESP/blob/main/src/rtl_433_ESP.cpp
// https://github.com/jgromes/RadioLib/blob/master/examples/CC1101/CC1101_Settings/CC1101_Settings.ino
// https://github.com/mag1024/esphome-rtl433/


#ifndef isHigh
  #define isHigh(pin) (digitalRead(pin) == HIGH)
#endif
#ifndef isLow
  #define isLow(pin) (digitalRead(pin) == LOW)
#endif

#define misoPin MISO   // MISO in


#ifdef misoPin
  #define wait_Miso_n()       { uint8_t miso_count = 255; while(isHigh(misoPin)) { delay(1); if(miso_count == 0) return      ; miso_count--; } }    // wait until SPI MISO line goes low
  #define wait_Miso_rf_n()    { uint8_t miso_count = 255; while(isHigh(misoPin)) { delay(1); if(miso_count == 0) return false; miso_count--; } }    // wait until SPI MISO line goes low
#endif

#ifdef misoPin
  #define wait_Miso()       {  }    // wait until SPI MISO line goes low
  #define wait_Miso_rf()    {  }    // wait until SPI MISO line goes low
#endif


#define EE_CC1101_CFG        2
#define EE_CC1101_CFG_SIZE   0x29
#define EE_CC1101_PA         0x30   //  (EE_CC1101_CFG+EE_CC1101_CFG_SIZE)  // 2C
#define EE_CC1101_PA_SIZE    8

#define PATABLE_DEFAULT      0x84   // 5 dB default value for factory reset


//---------------------------------------------------
// Chip Status Byte
//---------------------------------------------------

// Bit fields in the chip status byte
#define CC1101_STATUS_CHIP_RDYn_BM             0x80
#define CC1101_STATUS_STATE_BM                 0x70
#define CC1101_STATUS_FIFO_BYTES_AVAILABLE_BM  0x0F

// Chip states
#define CC1101_STATE_IDLE                      0x00
#define CC1101_STATE_RX                        0x10
#define CC1101_STATE_TX                        0x20
#define CC1101_STATE_FSTXON                    0x30
#define CC1101_STATE_CALIBRATE                 0x40
#define CC1101_STATE_SETTLING                  0x50
#define CC1101_STATE_RX_OVERFLOW               0x60
#define CC1101_STATE_TX_UNDERFLOW              0x70



#define CC1101_WRITE_BURST    0x40
#define CC1101_WRITE_SINGLE   0x00
#define CC1101_READ_BURST     0xC0
#define CC1101_READ_SINGLE    0x80
#define CC1101_CONFIG         CC1101_READ_SINGLE
#define CC1101_STATUS         CC1101_READ_BURST

#define CC1101_FREQ2       0x0D  // Frequency control word, high byte
#define CC1101_FREQ1       0x0E  // Frequency control word, middle byte
#define CC1101_FREQ0       0x0F  // Frequency control word, low byte
#define CC1101_PATABLE     0x3E  // 8 byte memory
#define CC1101_IOCFG2      0x00  // GDO2 output configuration
#define CC1101_PKTCTRL0    0x08  // Packet config register

// Status registers - newer version base on 0xF0
#define CC1101_PARTNUM_REV01      0xF0 // Chip ID
#define CC1101_VERSION_REV01      0xF1 // Chip ID
#define CC1101_RSSI_REV01         0xF4 // Received signal strength indication
#define CC1101_MARCSTATE_REV01    0xF5 // Control state machine state

// Status registers - older version base on 0x30
#define CC1101_PARTNUM_REV00      0x30 // Chip ID
#define CC1101_VERSION_REV00      0x31 // Chip ID
#define CC1101_RSSI_REV00         0x34 // Received signal strength indication
#define CC1101_MARCSTATE_REV00    0x35 // Control state machine state

// Strobe commands
#define CC1101_SRES     0x30  // reset
#define CC1101_SFSTXON  0x31  // Enable and calibrate frequency synthesizer (if MCSM0.FS_AUTOCAL=1).
#define CC1101_SCAL     0x33  // Calibrate frequency synthesizer and turn it off
#define CC1101_SRX      0x34  // Enable RX. Perform calibration first if coming from IDLE and MCSM0.FS_AUTOCAL=1
#define CC1101_STX      0x35  // In IDLE state: Enable TX. Perform calibration first if MCSM0.FS_AUTOCAL=1
#define CC1101_SIDLE    0x36  // Exit RX / TX, turn off frequency synthesizer
#define CC1101_SAFC     0x37  // Perform AFC adjustment of the frequency synthesizer

#define CC1101_SFRX     0x3A  // Flush the RX FIFO buffer | Underflow and # of bytes in TXFIFO / CC1101_TXBYTES
#define CC1101_SFTX     0x3B  // Flush the TX FIFO buffer | Overflow and # of bytes in RXFIFO / CC1101_RXBYTES
#define CC1101_SNOP     0x3D  // No operation. May be used to get access to the chip status byte.
#define CC1101_TXFIFO   0x3F
#define CC1101_RXFIFO   0x3F

namespace esphome {
namespace bresser5in1_cc1101 {

static const char *TAG = "bresser5in1_cc1101.component";

void Bresser5in1CC1101Component::setup() {
  ESP_LOGI(TAG, "SPI Setup");
  this->spi_setup();

  this->_gd0_rx->setup();
  this->_gd0_rx->attach_interrupt(&Bresser5in1CC1101Component::handleInterrupt, this, gpio::INTERRUPT_ANY_EDGE);
  this->_gd0_rx_isr = this->_gd0_rx->to_isr();

  this->radioInit();

}

void Bresser5in1CC1101Component::stopInterrupts() {
  this->_gd0_rx->detach_interrupt();
}

void Bresser5in1CC1101Component::radioInit() {
  this->_setupComplete = false;
  ESP_LOGI(TAG, "CC1101 Setup");

  static const char setupCommands[][14] = {
      "W0201",
      "W0446",
      "W0546",
      "W062D",
      "W07D4",
      "W08FF",
      "W09C0",
      "W0A02",
      "W0F21",
      "W1065",
      "W11E8",
      "W1288",
      "W134C",
      "W1402",
      "W1522",
      "W16F8",
      "W1751",
      "W1B16",
      "W1D43",
      "W1E68",
      "WS36",
      "WS34"
    };

    static const uint8_t initVal[] =
    {
            // IDX NAME     RESET  COMMENT
      0x01, // 00 IOCFG2    29     Modificato per Stazione meteo
      0x2E, // 01 IOCFG1           Tri-State
      0x2D, // 02 IOCFG0    3F     GDO0 for input
      //0x07, // 03 FIFOTHR          RX filter bandwidth > 325 kHz, FIFOTHR = 0x07
      0x47, // 03 FIFOTHR          RX filter bandwidth = 325 kHz, FIFOTHR = 0x47
      0xD3, // 04 SYNC1
      0x91, // 05 SYNC0
      0x3D, // 06 PKTLEN    0F
      0x04, // 07 PKTCTRL1
      0x02, // 08 PKTCTRL0  02     Modificato per Stazione meteo
      0x00, // 09 ADDR
      0x00, // 0A CHANNR
      0x06, // 0B FSCTRL1   0F     152kHz IF Frquency
      0x00, // 0C FSCTRL0
      0x21, // 0D FREQ2     1E     Modificato per Stazione meteo
      0x65, // 0E FREQ1     C4     Modificato per Stazione meteo
      0xE8, // 0F FREQ0     EC     Modificato per Stazione meteo
      0x57, // 10 MDMCFG4   8C     bWidth 325kHz
      0xC4, // 11 MDMCFG3   22     DataRate
      0x30, // 12 MDMCFG2   02     Modulation: ASK
      0x23, // 13 MDMCFG1   22
      0xb9, // 14 MDMCFG0   F8     ChannelSpace: 350kHz
      0x00, // 15 DEVIATN   47
      0x07, // 16 MCSM2     07
      0x00, // 17 MCSM1     30     Bit 3:2  RXOFF_MODE:  Select what should happen when a packet has been received: 0 = IDLE  3 =  Stay in RX ####
      0x18, // 18 MCSM0     04     Calibration: RX/TX->IDLE
      0x14, // 19 FOCCFG    36
      0x6C, // 1A BSCFG
      0x07, // 1B AGCCTRL2  03     42 dB instead of 33dB
      0x00, // 1C AGCCTRL1  40
      //0x90, // 1D AGCCTRL0  90     4dB decision boundery
      0x91, // 1D AGCCTRL0  91     8dB decision boundery
      0x87, // 1E WOREVT1
      0x6B, // 1F WOREVT0
      0xF8, // 20 WORCTRL
      //0x56, // 21 FREND1    56     RX filter bandwidth = 101 kHz, FREND1 = 0x56
      0xB6, // 21 FREND1    B6     RX filter bandwidth > 101 kHz, FREND1 = 0xB6
      0x11, // 22 FREND0    16     0x11 for no PA ramping
      0xE9, // 23 FSCAL3    A9     E9 ??
      0x2A, // 24 FSCAL2    0A
      0x00, // 25 FSCAL1    20     19 ??
      0x1F, // 26 FSCAL0    0D
      0x41, // 27 RCCTRL1
      0x00, // 28 RCCTRL0
  };

  ESP_LOGD(TAG, "CC1101 Setup Completed");

  this->enable();
  uint8_t res = 0;

	this->disable();                                     // some deselect and selects to init the cc1101
	delayMicroseconds(30);

	// Begin of power on reset
	this->enable();
	delayMicroseconds(30);

	this->disable();
	delayMicroseconds(45);

	res = this->cmdStrobe(CC1101_SRES);                                // send reset
	delay(10);
  ESP_LOGD(TAG, "Reset: %d", res);


  this->enable();
	wait_Miso();                                           // Wait until MISO goes low

	res = this->transfer_byte(0x00 | CC1101_WRITE_BURST);
  ESP_LOGD(TAG, "Write Burst response: %d", res);



	for (uint8_t i = 0; i < sizeof(initVal); i++) {         // write value to cc1101
      res = this->transfer_byte(initVal[i]);
      // ESP_LOGD(TAG, "Init Write response: %d", res);
	}

	this->disable();
	delayMicroseconds(10);                                          // ### todo: welcher Wert ist als delay sinnvoll? ###
	this->writePatable();                                                 // write PatableArray to patable reg
	delay(1);
//	this->setReceiveMode();



  for (size_t i = 0; i < sizeof(setupCommands) / sizeof(setupCommands[0]); i++) {
    this->writeCfg(setupCommands[i]);
  }

	this->enable();
  this->flushrx();                    // Flush the RX FIFO buffer
  delay(1);
  this->setReceiveMode();
	this->disable();

  _last_station_read = millis();
  this->_setupComplete = true;
}


void IRAM_ATTR HOT Bresser5in1CC1101Component::handleInterrupt(Bresser5in1CC1101Component* component) {
  if (component->_setupComplete) {
    int8_t bufIndex = 1 - component->_activeBuf;


    // ESP_LOGD(TAG, "handleInterrupt");
    uint8_t fifoBytes;
    bool dup;                                      // true bei identischen Wiederholungen bei readRXFIFO

    // unsigned long readStartMillis = millis();
    // while ((this->_gd0_rx->digital_read()) && (readStart < CC_MAX_BUF) && (readStartMillis > (millis() - 500) )) {                     // wait for CC1100_FIFOTHR given bytes to arrive in FIFO
    if (component->_gd0_rx_isr.digital_read()) {
      // ESP_LOGD(TAG, "GPIO UP. Reading");

      memset((void*)component->_ccBuf[bufIndex],0,CC_MAX_BUF); // Zero out memory

      fifoBytes = component->getRXBYTES();          // & 0x7f; // read len, transfer RX fifo
      component->_RSSI = component->getRSSIdev();
      if (fifoBytes > 0) {
        dup = component->readRXFIFO(0, bufIndex, fifoBytes);
        component->_readBytes[bufIndex] = fifoBytes;
        component->_gpioChanged = true;
        component->_activeBuf = bufIndex;
      }
      /*
      component->enable();
      component->flushrx();
      component->setReceiveMode();
      component->disable();
      */
    }
  }
}

uint8_t Bresser5in1CC1101Component::getRXBYTES() {                             // xFSK
	return readReg(CC1101_SFTX,CC1101_STATUS);
}

uint8_t Bresser5in1CC1101Component::getRSSIdev() {
  uint8_t revision = 0;
	return readReg((revision == 0x01 ? CC1101_RSSI_REV01 : CC1101_RSSI_REV00), CC1101_STATUS);
}

bool Bresser5in1CC1101Component::readRXFIFO(uint8_t start, uint8_t index, uint8_t len) {                             // xFSK
  bool dup = true;
  uint8_t rx;

  this->enable(); // select CC1101
  this->transfer_byte(CC1101_RXFIFO | CC1101_READ_BURST);    // send register address
  for (uint8_t i = start; i < start + len; i++) {
    rx = this->transfer_byte(0x00);                        // read result
    if (i + 1 < CC_MAX_BUF) {
      if (rx != _ccBuf[index][i]) {                              // if Circuit board for more cc110x -> ccBuf expand ( if (rx != ccBuf[radionr][i] ) )
        dup = false;
        _ccBuf[index][i] = rx;                                 // if Circuit board for more cc110x -> CC_MAX_BUF expand ( if (rx != _ccBuf[radionr][i] = rx ) )
      }
    }
  }
  this->disable();
  return dup;
}


void Bresser5in1CC1101Component::writePatable() {
  this->enable();
	wait_Miso();                                    // wait until MISO goes low

	this->transfer_byte(CC1101_PATABLE | CC1101_WRITE_BURST);   // send register address
  for (uint8_t i = 0; i < 8; i++) {
    if (i == 1) {
      this->transfer_byte(PATABLE_DEFAULT);     // send value
    }
    else {
      this->transfer_byte(0);     // send value
    }
  }

  this->disable();
}


void Bresser5in1CC1101Component::writeCfg(const char* IB_1) {
  if (IB_1[1] == 'S' && IB_1[2] == '3')
  {
    this->commandStrobes(IB_1);
  } else if (isHexadecimalDigit(IB_1[1]) && isHexadecimalDigit(IB_1[2]) && isHexadecimalDigit(IB_1[3]) && isHexadecimalDigit(IB_1[4])) {
    char b[3];
    b[2] = '\0';

    memcpy(b, &IB_1[1], 2);
    uint8_t reg = strtol(b, nullptr, 16);
    memcpy(b, &IB_1[3], 2);
    uint8_t val = strtol(b, nullptr, 16);

    ESP_LOGD(TAG, "Setting reg %d=%d",reg,val);

    this->writeCCreg(reg, val);
  }
}

void Bresser5in1CC1101Component::setReceiveMode() {
  uint8_t res = this->cmdStrobe(CC1101_SIDLE);
  // ESP_LOGD(TAG, "Set IDLE response: %d", res);
	delay(1);

  uint8_t maxloop = 0xff;

	while (maxloop-- && (this->cmdStrobe(CC1101_SRX) & CC1101_STATUS_STATE_BM) != CC1101_STATE_RX) // RX enable
		delay(1);

  if (maxloop == 0) { ESP_LOGD(TAG, "Setting RX failed"); }

}

void Bresser5in1CC1101Component::writeCCreg(uint8_t reg, uint8_t var) { // write CC1101 register
	if (reg > 1 && reg < 0x40) {
    this->enable();                                // select CC1101
    wait_Miso();                                    // wait until MISO goes low
    this->transfer_byte(reg - EE_CC1101_CFG);
    this->transfer_byte(var);
    this->disable();

  /*
	if (reg - EE_CC1101_CFG == 18) {
		ccmode = ( var & 0x70 ) >> 4;                // read modulation direct from 0x12 MDMCFG2
	}
 */

    /*
     * sprintf(b,"W%02X%02X",reg,var); // is 48 bytes bigger
     */
    // ESP_LOGD(TAG, "W%02X%02X",reg,var);

  }
}
void Bresser5in1CC1101Component::commandStrobes(const char* IB_1) {
	uint8_t reg;
	uint8_t val;
	uint8_t val1;

	if (isHexadecimalDigit(IB_1[3])) {
		reg = (uint8_t)strtol(&IB_1[2], nullptr, 16);  // address strobe command | CC1101 - Table 42: Command Strobes
		if (reg < 0x3E) {
			val = this->cmdStrobe(reg);
			delay(1);
			val1 = this->cmdStrobe(0x3D);                 //  No operation. May be used to get access to the chip status byte.

      /*
       *  sprintf_P(b, PSTR("cmdStrobeReg %02X chipStatus %02X delay1 %02X"), reg, val >> 4, val1 >> 4); // is 110 bytes bigger
       */
			//
      ESP_LOGD(TAG, "cmdStrobeReg - 0x%02x chipStatus 0x%02x - delay1 0x%02x", reg,val >> 4, val1 >> 4);

		}
	}
}

void Bresser5in1CC1101Component::writeReg(const uint8_t regAddr, const uint8_t val) {       // write single register into the CC1101 IC via SPI
	this->transfer_byte(regAddr);                               // send register address
	this->transfer_byte(val);                                   // send value
}

uint8_t Bresser5in1CC1101Component::readReg(const uint8_t regAddr, const uint8_t regType) {       // read CC1101 register via SPI
  this->enable();
//  delay(10);
	this->transfer_byte(regAddr | regType);         // send register address
	uint8_t val = this->transfer_byte(0x00);                    // read result
  this->disable();
	return val;
}

uint8_t Bresser5in1CC1101Component::waitTo_Miso() {
	uint8_t i = 255;
	while(isHigh(misoPin)) {
		delayMicroseconds(10);
		i--;
	}
	return i;
}

uint8_t Bresser5in1CC1101Component::cmdStrobe(const uint8_t cmd) {
	this->enable();                                // select CC1101
	wait_Miso_rf();                                 // wait until MISO goes low
	uint8_t ret = this->transfer_byte(cmd);                     // send strobe command
	wait_Miso_rf();                                 // wait until MISO goes low
	this->disable();                              // deselect CC1101
	return ret;                                     // Chip Status Byte
}

uint8_t Bresser5in1CC1101Component::cmdStrobeTo(const uint8_t cmd) {
	this->enable();                                // select CC1101
	if (waitTo_Miso() == 0) {                       // wait with timeout until MISO goes low
    this->disable();
		return false;                               // timeout
	}
	this->transfer_byte(cmd);                                   // send strobe command
  this->disable();
	return true;

}

uint8_t Bresser5in1CC1101Component::checkParity(const byte* msg, uint8_t readBytes) {
  // First 13 bytes need to match inverse of last 13 bytes
  if (readBytes < 26) {
    ESP_LOGD(TAG,"Read %d bytes, not enough.", readBytes);
    return CC_MAX_BUF;
  }

  for (uint8_t startcol = 0; startcol < (readBytes - 25); startcol++ )
  {
    uint8_t col = 0;
    for (col = 0; col < 13; ++col) {
      if ((msg[col + startcol] ^ msg[col + startcol + 13]) != 0xff) {
        // ESP_LOGD(TAG,"Parity wrong");
        break;
      }
    }
    if (col == 13) {
      uint8_t pops_check = msg[startcol + 13];
      uint8_t pops_count = 0;
      for (size_t i = startcol + 14; i < startcol + 25; i++) {
        pops_count += __builtin_popcount(msg[i]);
      }
      if (pops_check == pops_count) {
        ESP_LOGD(TAG,"Parity found at %d", startcol);
        return startcol;
      } else {
        ESP_LOGD(TAG,"Parity found but pops check fail. Pops TO CHECK: %d - Counted: %d", pops_check, pops_count);
      }
    }
  }
  return CC_MAX_BUF;
}

uint8_t Bresser5in1CC1101Component::bresser_5in1_decode()
{
    byte msg[CC_MAX_BUF];
    memcpy(msg,(const byte*) this->_ccBuf[_activeBuf],CC_MAX_BUF);

    uint8_t startIndex = checkParity(msg, _readBytes[_activeBuf]);

    if (startIndex ==  CC_MAX_BUF) {
      // ESP_LOGD(TAG,"Parity wrong");
      return 11; // message isn't correct
    }
/*
    // First 13 bytes need to match inverse of last 13 bytes
    for (unsigned col = 0; col < 13; ++col) {
        if ((msg[col] ^ msg[col + 13]) != 0xff) {
            ESP_LOGD(TAG,"Parity wrong");
            return 11; // message isn't correct
        }
    }
*/
    ESP_LOGD(TAG,
      "Parsing the followind hex values:"
      "%02X %02X %02X %02X %02X %02X "
      "%02X %02X %02X %02X %02X %02X "
      "%02X %02X %02X %02X %02X %02X "
      "%02X %02X %02X %02X %02X %02X "
      "%02X %02X",
      msg[startIndex + 0], msg[startIndex + 1], msg[startIndex + 2], msg[startIndex + 3], msg[startIndex + 4], msg[startIndex + 5],
      msg[startIndex + 6], msg[startIndex + 7], msg[startIndex + 8], msg[startIndex + 9], msg[startIndex + 10], msg[startIndex + 11],
      msg[startIndex + 12], msg[startIndex + 13], msg[startIndex + 14], msg[startIndex + 15], msg[startIndex + 16], msg[startIndex + 17],
      msg[startIndex + 18], msg[startIndex + 19], msg[startIndex + 20], msg[startIndex + 21], msg[startIndex + 22], msg[startIndex + 23],
      msg[startIndex + 24], msg[startIndex + 25]
    );

    BresserReading reading;
    reading.sensor_id = msg[startIndex + 14];


    if ( (this->_filter_station_id) && (reading.sensor_id != this->_filter_station_id) )
    {
      ESP_LOGD(TAG,"Station ID not selected. Read value from: %d - selected: %d", reading.sensor_id, this->_filter_station_id);
      return 10;
    }

    int temp_raw = (msg[startIndex + 20] & 0x0f) + ((msg[startIndex + 20] & 0xf0) >> 4) * 10 + (msg[startIndex + 21] &0x0f) * 100;
    if (msg[startIndex + 25] & 0x0f)
        temp_raw = -temp_raw;

    reading.temperature = (float)temp_raw * 0.1f;
    ESP_LOGD(TAG,"Temp raw: %d - Calculated: %.6f", temp_raw, reading.temperature);
    reading.humidity = (msg[startIndex + 22] & 0x0f) + ((msg[startIndex + 22] & 0xf0) >> 4) * 10;

    reading.wind_direction_deg = (float)((msg[startIndex + 17] & 0xf0) >> 4) * 22.5f;

    int gust_raw = ((msg[startIndex + 17] & 0x0f) << 8) + msg[startIndex + 16]; //fix merbanan/rtl_433#1315
    reading.wind_gust = (float)gust_raw * 0.1f;

    int wind_raw = (msg[startIndex + 18] & 0x0f) + ((msg[startIndex + 18] & 0xf0) >> 4) * 10 + (msg[startIndex + 19] & 0x0f) * 100; //fix merbanan/rtl_433#1315
    reading.wind_avg = (float)wind_raw * 0.1f;

    int rain_raw = (msg[startIndex + 23] & 0x0f) + ((msg[startIndex + 23] & 0xf0) >> 4) * 10 + (msg[startIndex + 24] & 0x0f) * 100;
    reading.rain = (float)rain_raw * 0.1f;

    reading.battery_ok = ((msg[startIndex + 25] & 0x80) == 0);

    state_callback_.call(&reading);

    if (this->_temperature != nullptr) {
      this->_temperature->publish_state(reading.temperature);
    }

    if (this->_battery_sensor != nullptr) {
      this->_battery_sensor->publish_state(reading.battery_ok == 0);
    }

    if (this->_humidity != nullptr) {
      this->_humidity->publish_state(reading.humidity);
    }

    if (this->_wind_direction_degrees != nullptr) {
      this->_wind_direction_degrees->publish_state(reading.wind_direction_deg);
    }

    if (this->_wind_gusts_speed != nullptr) {
      this->_wind_gusts_speed->publish_state(reading.wind_gust);
    }

    if (this->_wind_speed != nullptr) {
      this->_wind_speed->publish_state(reading.wind_avg);
    }

    if (this->_rain_level != nullptr) {
      this->_rain_level->publish_state(reading.rain);
    }

    if (this->_station_id != nullptr) {
      this->_station_id->publish_state(reading.sensor_id);
    }

    if (this->_RSSI_level != nullptr) {
      this->_RSSI_level->publish_state(_RSSI);
    }

    ESP_LOGD(TAG,"Reading complete. RSSI: %d, sensor id: %d - Temp: %.2f - Humidity: %d - Wind direction: %.2f - Wind gust: %.2f - Wind avg: %.2f - Rain: %.2f - Battery ok: %d",
             _RSSI,
             reading.sensor_id,
             reading.temperature,
             reading.humidity,
             reading.wind_direction_deg,
             reading.wind_gust,
             reading.wind_avg,
             reading.rain,
             reading.battery_ok
    );

    return 1;
}

uint8_t Bresser5in1CC1101Component::getMARCSTATE() {
	return readReg(CC1101_MARCSTATE_REV00, CC1101_STATUS);  // xFSK, Pruefen ob Umwandung von uint to int den richtigen Wert zurueck gibt
}

bool Bresser5in1CC1101Component::flushrx() {
  this->transfer_byte(CC1101_SIDLE);

  this->transfer_byte(CC1101_SNOP);
	this->transfer_byte(CC1101_SFRX);
	return true;

}

void Bresser5in1CC1101Component::loop() {
  if (this->_gpioChanged) {
    this->enable();
    this->flushrx();
    delay(1);
    this->setReceiveMode();
    this->disable();


    this->bresser_5in1_decode();
    this->_gpioChanged = false;
    _last_station_read = millis();
  } else {
    if (millis() - _last_station_read > 900000) {
      ESP_LOGW(TAG,"No Data from CC1101 since 15 minutes. Reinit");
      this->radioInit();
      return;
    }
  }
}

void Bresser5in1CC1101Component::dump_config(){
    ESP_LOGCONFIG(TAG, "bresser-cc1101-reader component");
    LOG_SENSOR("  ", "Station ID", this->_station_id);
    LOG_SENSOR("  ", "Temperature", this->_temperature);
    LOG_SENSOR("  ", "Humidity", this->_humidity);
    LOG_SENSOR("  ", "Wind direction", this->_wind_direction_degrees);
    LOG_SENSOR("  ", "Wind gusts speed", this->_wind_gusts_speed);
    LOG_SENSOR("  ", "Wind avg speed", this->_wind_speed);
    LOG_SENSOR("  ", "Rain level", this->_rain_level);
    LOG_BINARY_SENSOR("  ", "Battery low", this->_battery_sensor);

}

}  // namespace bresser5in1_cc1101
}  // namespace esphome
