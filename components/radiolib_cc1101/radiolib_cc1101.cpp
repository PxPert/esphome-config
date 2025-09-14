#include "esphome/core/log.h"
#include "radiolib_cc1101.h"

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
namespace radiolib_cc1101 {

static const char *TAG = "radiolib_cc1101.component";

void RadiolibCC1101Component::setup() {
  ESP_LOGI(TAG, "SPI Setup");
  this->spi_setup();

  this->_gd0_rx->setup();
  this->_gd0_rx_isr = this->_gd0_rx->to_isr();
  this->_gd0_rx->attach_interrupt(&RadiolibCC1101Component::handleInterrupt, this, gpio::INTERRUPT_ANY_EDGE);

  this->radioInit();

}

void RadiolibCC1101Component::radioInit() {
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
      ESP_LOGD(TAG, "Init Write response: %d", res);
	}

	this->disable();
	delayMicroseconds(10);                                          // ### todo: welcher Wert ist als delay sinnvoll? ###
	this->writePatable();                                                 // write PatableArray to patable reg
	delay(1);
//	this->setReceiveMode();



  for (size_t i = 0; i < sizeof(setupCommands) / sizeof(setupCommands[0]); i++) {
    this->writeCfg(setupCommands[i]);
  }

  this->flushrx();                    // Flush the RX FIFO buffer
  delay(1);
  this->setReceiveMode();
  this->_setupComplete = true;
}

void IRAM_ATTR HOT RadiolibCC1101Component::handleInterrupt(RadiolibCC1101Component* component) {

  if (! component->_setupComplete) {
    return;
  }

  ESP_LOGD(TAG, "handleInterrupt");
	uint8_t fifoBytes;
	bool dup;                                      // true bei identischen Wiederholungen bei readRXFIFO


 if (component->_ccBufReady) {
   ESP_LOGD(TAG, "Buffer ready. Return");
   return;
 }

  if (component->_gd0_rx_isr.digital_read()) {                     // wait for CC1100_FIFOTHR given bytes to arrive in FIFO
    ESP_LOGD(TAG, "GPIO UP. Reading");

/*
 *
 * Ralf ( mode numbering == own selection )
 * cc1101 Mode: 0 - normal ASK/OOK, 1 - FIFO, 2 - FIFO ohne dup, 3 - FIFO LaCrosse, 4 - experimentell, 9 - FIFO mit Debug Ausgaben
 *
 * Sidey ( mode numbering == cc1101 data sheet setting numbering 0x12: MDMCFG2–Modem Configuration )
 * cc1101 Mode: 0 - FIFO LaCrosse, 3 - normal ASK/OOK
 *

    if (ccmode == 4) {
      cc1101::ccStrobe_SIDLE(); // start over syncing
    }
*/

		fifoBytes = component->getRXBYTES();          // & 0x7f; // read len, transfer RX fifo
		if (fifoBytes > 0) {
			uint8_t RSSI = component->getRSSIdev();

/*
 * !!! for DEVELOPMENT and DEBUG only !!!
 *
      #ifdef DEBUG
        if (cc1101::ccmode == 0) {
          MSG_PRINT(F("RX fifoBytes ("));
          MSG_PRINT(fifoBytes);
          MSG_PRINTLN((") "));
        }
      #endif
 *
 */

			/* if (fifoBytes < 0x80) */ {                // RXoverflow?
			/*	if (fifoBytes > CC_MAX_BUF) {
					fifoBytes = CC_MAX_BUF;
				}
       */
				dup = component->readRXFIFO(fifoBytes);
				if (dup == false) { // Ralf9: 2 - FIFO ohne dup
          component->_ccBufReady = true;
          ESP_LOGD(TAG, "Buffer ready, Read %d bytes - RSSI: %d",fifoBytes, RSSI);

//				  for (uint8_t i = 0; i < fifoBytes; i++) {
//						DBG_PRINTtoHEX(ccBuf[i]);
//					}
				} else {
          ESP_LOGD(TAG, "Buffer read error");
        }
			}
		} else {
      ESP_LOGD(TAG, "Error: 0 fifo bytes");
    }
	} else {
    ESP_LOGD(TAG, "GPIO Down");
  }

}

uint8_t RadiolibCC1101Component::getRXBYTES() {                             // xFSK
	return readReg(CC1101_SFTX,CC1101_STATUS);
}

uint8_t RadiolibCC1101Component::getRSSIdev() {
  uint8_t revision = 0;
	return readReg((revision == 0x01 ? CC1101_RSSI_REV01 : CC1101_RSSI_REV00), CC1101_STATUS);
}

bool RadiolibCC1101Component::readRXFIFO(uint8_t len) {                             // xFSK
  bool dup = true;
  uint8_t rx;

  this->enable(); // select CC1101
  this->transfer_byte(CC1101_RXFIFO | CC1101_READ_BURST);    // send register address
  for (uint8_t i = 0; i < len; i++) {
    rx = this->transfer_byte(0x00);                        // read result
    if (i + 1 < CC_MAX_BUF) {
      if (rx != _ccBuf[i]) {                              // if Circuit board for more cc110x -> ccBuf expand ( if (rx != ccBuf[radionr][i] ) )
        dup = false;
        _ccBuf[i] = rx;                                 // if Circuit board for more cc110x -> CC_MAX_BUF expand ( if (rx != _ccBuf[radionr][i] = rx ) )
      }
    }
  }
  this->disable();
  return dup;
}


void RadiolibCC1101Component::writePatable() {
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


void RadiolibCC1101Component::writeCfg(const char* IB_1) {
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

void RadiolibCC1101Component::setReceiveMode() {
  uint8_t res = this->cmdStrobe(CC1101_SIDLE);
  ESP_LOGD(TAG, "Set IDLE response: %d", res);
	delay(1);

  uint8_t maxloop = 0xff;

	while (maxloop-- && (this->cmdStrobe(CC1101_SRX) & CC1101_STATUS_STATE_BM) != CC1101_STATE_RX) // RX enable
		delay(1);

  if (maxloop == 0) { ESP_LOGD(TAG, "Setting RX failed"); }

}

void RadiolibCC1101Component::writeCCreg(uint8_t reg, uint8_t var) { // write CC1101 register
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
    ESP_LOGD(TAG, "W%02X%02X",reg,var);

  }
}
void RadiolibCC1101Component::commandStrobes(const char* IB_1) {
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

void RadiolibCC1101Component::writeReg(const uint8_t regAddr, const uint8_t val) {       // write single register into the CC1101 IC via SPI
	this->transfer_byte(regAddr);                               // send register address
	this->transfer_byte(val);                                   // send value
}

uint8_t RadiolibCC1101Component::readReg(const uint8_t regAddr, const uint8_t regType) {       // read CC1101 register via SPI
  this->enable();
	this->transfer_byte(regAddr | regType);         // send register address
	uint8_t val = this->transfer_byte(0x00);                    // read result
  this->disable();
	return val;
}

uint8_t RadiolibCC1101Component::waitTo_Miso() {
	uint8_t i = 255;
	while(isHigh(misoPin)) {
		delayMicroseconds(10);
		i--;
	}
	return i;
}

uint8_t RadiolibCC1101Component::cmdStrobe(const uint8_t cmd) {
	this->enable();                                // select CC1101
	wait_Miso_rf();                                 // wait until MISO goes low
	uint8_t ret = this->transfer_byte(cmd);                     // send strobe command
	wait_Miso_rf();                                 // wait until MISO goes low
	this->disable();                              // deselect CC1101
	return ret;                                     // Chip Status Byte
}

uint8_t RadiolibCC1101Component::cmdStrobeTo(const uint8_t cmd) {
	this->enable();                                // select CC1101
	if (waitTo_Miso() == 0) {                       // wait with timeout until MISO goes low
    this->disable();
		return false;                               // timeout
	}
	this->transfer_byte(cmd);                                   // send strobe command
  this->disable();
	return true;

}


uint8_t RadiolibCC1101Component::bresser_5in1_decode()
{
    byte msg[CC_MAX_BUF];
    memcpy(msg,(const byte*) this->_ccBuf,CC_MAX_BUF);

    // First 13 bytes need to match inverse of last 13 bytes
    for (unsigned col = 0; col < sizeof (this->_ccBuf) / 2; ++col) {
        if ((msg[col] ^ msg[col + 13]) != 0xff) {
            ESP_LOGD(TAG,"Parity wrong");
            return 11; // message isn't correct
        }
    }

    BresserReading reading;
    reading.sensor_id = msg[14];


    int temp_raw = (msg[20] & 0x0f) + ((msg[20] & 0xf0) >> 4) * 10 + (msg[21] &0x0f) * 100;
    if (msg[25] & 0x0f)
        temp_raw = -temp_raw;
    reading.temperature = (float)temp_raw * 0.1f;

    reading.humidity = (msg[22] & 0x0f) + ((msg[22] & 0xf0) >> 4) * 10;

    reading.wind_direction_deg = (float)((msg[17] & 0xf0) >> 4) * 22.5f;

    int gust_raw = ((msg[17] & 0x0f) << 8) + msg[16]; //fix merbanan/rtl_433#1315
    reading.wind_gust = (float)gust_raw * 0.1f;

    int wind_raw = (msg[18] & 0x0f) + ((msg[18] & 0xf0) >> 4) * 10 + (msg[19] & 0x0f) * 100; //fix merbanan/rtl_433#1315
    reading.wind_avg = (float)wind_raw * 0.1f;

    int rain_raw = (msg[23] & 0x0f) + ((msg[23] & 0xf0) >> 4) * 10 + (msg[24] & 0x0f) * 100;
    reading.rain = (float)rain_raw * 0.1f;

    reading.battery_ok = ((msg[25] & 0x80) == 0);

    ESP_LOGD(TAG,"Reading complete. sensor id: %d - Tempera: %.2f - Humidity: %d - Wind direction: %.2f - Wind gust: %.2f - Wind avg: %.2f - Rain: %.2f - Battery ok: %d",
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

uint8_t RadiolibCC1101Component::getMARCSTATE() {
	return readReg(CC1101_MARCSTATE_REV00, CC1101_STATUS);  // xFSK, Pruefen ob Umwandung von uint to int den richtigen Wert zurueck gibt
}

bool RadiolibCC1101Component::flushrx() {
  this->transfer_byte(CC1101_SIDLE);

  this->transfer_byte(CC1101_SNOP);
	this->transfer_byte(CC1101_SFRX);
	return true;

}

void RadiolibCC1101Component::loop() {
//  this->enable();
  static unsigned long lastmillis;

  if ((false) && (lastmillis < millis() - 1000)) {
    lastmillis = millis();
    ESP_LOGD(TAG,"CHECK");

    uint8_t marcstate = this->getMARCSTATE();

    if (marcstate != 13) {
      ESP_LOGD(TAG,"CC1101 MARC STATE != 13: %d", marcstate);
      delay(1);
    }

    if (marcstate == 17) {   // RXoverflow oder nicht ASK/OOK
      ESP_LOGD(TAG,"CC1101 MARC STATE = 17, flushing and resetting receive mode");
      if (this->flushrx()) {                    // Flush the RX FIFO buffer
        delay(1);
        this->setReceiveMode();
      }
    }
  }

  if (this->_gd0_rx->digital_read()) {
    this->handleInterrupt(this);
  }

  /*
  if (mills > (_millis_ultima_lettura + CC1101_TIMEOUT_MSECS) ) {
    mqtt_accoda(MessaggioMQTT(CHIAVE_SM_STATO, "Errore - Resetto CC1101"));
    setup();
  }
  */


  if (this->_ccBufReady) {
    ESP_LOGD(TAG,"Buffer Ready. Parsing");
    this->bresser_5in1_decode();
//    bresser_5in1_decode(cc1101::ccBuf);
    this->_ccBufReady = false;
    // cc1101::getRxFifo();
    this->flushrx();
    delay(1);
    this->setReceiveMode();
    /*
    if (cc1101::flushrx()) {                    // Flush the RX FIFO buffer
      cc1101::setReceiveMode();
    }
    */

  }


//  this->disable();

  /*
  static int last_millis = 0;
  if (last_millis == 0 || last_millis < millis() - 1000) {
    last_millis = millis();
    ESP_LOGD(TAG, "CC1101 Status: state recv=%d - gd0:%d, last_rssi:%d", state, _gd0_rx->digital_read(), getRSSI());
  }
  */
}

void RadiolibCC1101Component::dump_config(){
    ESP_LOGCONFIG(TAG, "RadioLib-cc1101 component");
}

void RadiolibCC1101Component::set_registers() {
  /*
  init_state|=radio.setFrequency(_freq);
  init_state|=radio.setBitRate(_bitrate);
  // set rx bw after datarate - and only specific ones make sense...
  adjustBW(_bandwidth);
  init_state|=radio.setRxBandwidth(_bandwidth);
  */
  /* NEW */

  init_state|=radio.setCrcFiltering(false);
  ESP_LOGD(TAG, "CC1101 01 init_state =%d", init_state);
  init_state|=radio.fixedPacketLengthMode(27);
  ESP_LOGD(TAG, "CC1101 02 init_state =%d", init_state);

  init_state|=radio.setSyncWord(0xAA, 0x2D, 0, false);
  ESP_LOGD(TAG, "CC1101 03 init_state =%d", init_state);
  /* END NEW */

/*
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_FREND1,_REG_FREND1);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_TEST2,_REG_TEST2);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_TEST1,_REG_TEST1);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_FIFOTHR, _REG_FIFOTHR);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL2,_REG_AGCCTRL2);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL1,_REG_AGCCTRL1);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL0,_REG_AGCCTRL0);
*/
  ESP_LOGD(TAG, "CC1101 set_registers() complete - freq=%.2f, bitrate=%.2f, bandwidth=%.2f, OOK Modulation=%d registers set, init_state =%d", _freq,_bitrate,_bandwidth,_modulation==OOK_MODULATION, init_state);
}

void RadiolibCC1101Component::setup_direct_mode() {
  // init_state|=standby();

  // per DN022 adjust LNA as needed
  _REG_FREND1=(_bandwidth>101) ? 0xb6 : 0x56;
  // also per DN022
  _REG_TEST2= (_bandwidth>325) ? 0x88 : 0x81;
  _REG_TEST1= (_bandwidth>325) ? 0x31 : 0x35;
  _REG_FIFOTHR= (_bandwidth>325) ? 0x07 : 0x47;

  set_registers();

  init_state|=radio.setOOK(_modulation==OOK_MODULATION); 

  // start receiving onto GDO
  // init_state|= recv();

}

int RadiolibCC1101Component::standby() {
  // standby state: radio in standby
  init_state|=radio.standby();
  state=init_state==0 ? CC1101_STANDBY : CC1101_NOINIT;
  return init_state;
}

int RadiolibCC1101Component::recv() {
  // receive state: radio doing receiveDirectAsync
  if (state==CC1101_XMIT) standby();

  init_state|=radio.receiveDirectAsync();
  state=init_state==0 ? CC1101_RECV : CC1101_NOINIT;
  return init_state;
}

int RadiolibCC1101Component::xmit() {
  // xmit state: gd0 is output
  standby(); 

  init_state|=radio.transmitDirectAsync();
  state=init_state==0 ? CC1101_XMIT : CC1101_NOINIT;

  return init_state;
}

void RadiolibCC1101Component::adjustBW(float bandwidth) {
  // set to a valid value
  float possibles[16] = {58, 68, 81, 102, 116, 135, 162, 203, 232, 270, 325, 406, 464, 541, 650, 812};
  for(int i=0;i<15;i++) {
    if ((bandwidth>=possibles[i])&&(bandwidth<=possibles[i+1])) {
      _bandwidth=bandwidth-possibles[i]<possibles[i+1]-bandwidth ? possibles[i] : possibles[i+1];
      break;
    }
  }
}

float RadiolibCC1101Component::getRSSI() {
  return state==CC1101_RECV ? radio.getRSSI() : -1;
}

}  // namespace radiolib_cc1101
}  // namespace esphome
