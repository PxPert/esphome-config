#include "esphome/core/log.h"
#include "ansluta_cc2500.h"
#include "cc2500_REG.h"

#define Light_OFF       0x01      // Command to turn the light off
#define Light_ON_50     0x02      // Command to turn the light on 50%
#define Light_ON_100    0x03      // Command to turn the light on 100%
// #define Light_PAIR      0xFF      // Command to pair a remote to the light


namespace esphome {
namespace ansluta_cc2500 {

static const char *TAG = "ansluta_cc2500.component";
static const uint8_t AddressByteA = 0x8B;
static const uint8_t AddressByteB = 0xB1;

void AnslutaCC2500Component::setLight(uint8_t state) {
  if(state < 3) {
    this->sendCommand(AddressByteA,AddressByteB, state + 1);
  }
}

void AnslutaCC2500Component::setup() {
  ESP_LOGI(TAG, "SPI Setup");
  this->spi_setup();

  this->radioInit();

}

void AnslutaCC2500Component::writeReg(byte addr, byte value){

  this->enable();
  delayMicroseconds(1);         // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode
  this->transfer_byte(addr);
  delayMicroseconds(1);
  this->transfer_byte(value);
  delayMicroseconds(1);
  this->disable();
  delayMicroseconds(1);         // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode

}

uint8_t AnslutaCC2500Component::sendStrobe(uint8_t strobe) {
  this->enable();
  delayMicroseconds(1);         // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode
  uint8_t ret = this->transfer_byte(strobe);
  this->disable();
  delayMicroseconds(1);         // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode

  return ret;
}

void AnslutaCC2500Component::flushRx() {
  this->sendStrobe(CC2500_SIDLE);      // Exit RX / TX
  this->sendStrobe(CC2500_SFRX);       // Flush the RX FIFO buffer
}

uint8_t AnslutaCC2500Component::getRXBYTES() {                             // xFSK
  return this->readReg(CC2500_SFTX,CC2500_STATUS);
}

uint8_t AnslutaCC2500Component::readReg(const uint8_t regAddr, const uint8_t regType) {       // read CC1101 register via SPI
  this->enable();
  delayMicroseconds(1);         // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode
  this->transfer_byte(regAddr | regType);                     // send register address
  uint8_t val = this->transfer_byte(0x00);                    // read result
  this->disable();
  delayMicroseconds(1);         // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode
  return val;
}

uint8_t AnslutaCC2500Component::readRemoteCommand(){     //Read Address Bytes From a remote by sniffing its packets wireless

  uint8_t packetLength = this->getRXBYTES();
  uint8_t recvPacket[packetLength];

  if ( packetLength >= 1) {
    ESP_LOGD(TAG,"CC2500 Packet received: %d bytes", DEC);

    this->enable();
    this->transfer_byte(CC2500_FIFO | CC2500_READ_BURST);    // send register address
    for(uint8_t i = 1; i <= packetLength; i++){    //Read the received data from CC2500
      recvPacket[i] = this->transfer_byte(0x00);
     }
    this->disable();

    if (packetLength < 5) {
      return 255;
    }

    uint8_t start=packetLength;
    for ( uint8_t i = 0; i < packetLength - 5; i++) {
      if(recvPacket[i+1]==0x01 && recvPacket[i+5]==0xAA){
        start = i;
      }
    }

    if(start < packetLength){   //If the bytes match an Ikea remote sequence
      ESP_LOGD(TAG,"CC2500 Payload received");
      /*
        Serial.print ("Payload ricevuto: ");
        for (uint8_t i = start; i < packetLength; i++) {
          Serial.print(recvPacket[i],HEX);
          Serial.print( " " );
        }
        Serial.println();
      */

      if ((AddressByteA == recvPacket[start+2]) && (AddressByteB == recvPacket[start+3])) {
        uint8_t command = recvPacket[start+4];
        ESP_LOGD(TAG,"Command received: %d", command);
        return command;
      }
    } else {
      return 255;
    }
  }

   return 0;
}


void AnslutaCC2500Component::sendCommand(byte AddressByteA, byte AddressByteB, byte Command){

    for(byte i=0;i<50;i++){         //Send 50 times


      this->sendStrobe(CC2500_SIDLE);     // 0x36
      this->sendStrobe(CC2500_SFTX);      // 0x3B

      this->enable();

      delayMicroseconds(1);         // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode

      this->transfer_byte(0x7F);           // activate burst data
      delayMicroseconds(2);

      this->transfer_byte(0x06);           // send 6 data bytes
      delayMicroseconds(2);

      this->transfer_byte(0x55);           // ansluta data byte 1
      delayMicroseconds(2);

      this->transfer_byte(0x01);           // ansluta data byte 2
      delayMicroseconds(2);

      this->transfer_byte(AddressByteA);   // ansluta data address byte A
      delayMicroseconds(2);

      this->transfer_byte(AddressByteB);   // ansluta data address byte B
      delayMicroseconds(2);

      this->transfer_byte(Command);        // ansluta data command 0x01=Light OFF 0x02=50% 0x03=100% 0xFF=Pairing
      delayMicroseconds(2);

      this->transfer_byte(0xAA);           // ansluta data byte 6
      // delayMicroseconds(2);
      this->transfer_byte(0xFF);           // ansluta data byte 6


      this->disable();

      this->sendStrobe(CC2500_STX);      // 0x35  transmit data in TX

      delay(1);
      while(this->getMARCSTATE() != 1) {
        delay(1);
      }
    }
}

bool AnslutaCC2500Component::readRXFIFO(uint8_t len) {                             // xFSK
  bool dup = true;
  uint8_t rx;

  uint8_t cc2500Buf[256];

  this->enable();
  this->transfer_byte(CC2500_FIFO | CC2500_READ_BURST);    // send register address
  for (uint8_t i = 0; i < len; i++) {
    rx = this->transfer_byte(0x00);                        // read result
    if (rx != cc2500Buf[i]) {                              // if Circuit board for more cc110x -> ccBuf expand ( if (rx != ccBuf[radionr][i] ) )
      dup = false;
      cc2500Buf[i] = rx;                                 // if Circuit board for more cc110x -> ccBuf expand ( if (rx != ccBuf[radionr][i] = rx ) )
    }
  }
  this->disable();
  return dup;
}



uint8_t AnslutaCC2500Component::getMARCSTATE() {                            // xFSK, Control state machine state
  return this->readReg(REG_MARCSTATE, CC2500_STATUS);  // xFSK, Pruefen ob Umwandung von uint to int den richtigen Wert zurueck gibt
}


void AnslutaCC2500Component::enableRead() {
  this->sendStrobe(CC2500_SRX);       // 0x34 Enable RX.
  // CC2500_WriteReg(REG_IOCFG1,0x01);    // REG_IOCFG1 = 0x01 Switch MISO to output if a packet has been received or not
}

void AnslutaCC2500Component::radioInit() {
  this->_setupComplete = false;


  this->sendStrobe(CC2500_SRES); //0x30 SRES Reset chip.

  this->writeReg(REG_IOCFG2,VAL_IOCFG2);
  this->writeReg(REG_IOCFG0,VAL_IOCFG0);
  this->writeReg(REG_PKTLEN,VAL_PKTLEN);
  this->writeReg(REG_PKTCTRL1,VAL_PKTCTRL1);
  this->writeReg(REG_PKTCTRL0,VAL_PKTCTRL0);
  this->writeReg(REG_ADDR,VAL_ADDR);
  this->writeReg(REG_CHANNR,VAL_CHANNR);
  this->writeReg(REG_FSCTRL1,VAL_FSCTRL1);
  this->writeReg(REG_FSCTRL0,VAL_FSCTRL0);
  this->writeReg(REG_FREQ2,VAL_FREQ2);
  this->writeReg(REG_FREQ1,VAL_FREQ1);
  this->writeReg(REG_FREQ0,VAL_FREQ0);
  this->writeReg(REG_MDMCFG4,VAL_MDMCFG4);
  this->writeReg(REG_MDMCFG3,VAL_MDMCFG3);
  this->writeReg(REG_MDMCFG2,VAL_MDMCFG2);
  this->writeReg(REG_MDMCFG1,VAL_MDMCFG1);
  this->writeReg(REG_MDMCFG0,VAL_MDMCFG0);
  this->writeReg(REG_DEVIATN,VAL_DEVIATN);
  this->writeReg(REG_MCSM2,VAL_MCSM2);
  this->writeReg(REG_MCSM1,VAL_MCSM1);
  this->writeReg(REG_MCSM0,VAL_MCSM0);
  this->writeReg(REG_FOCCFG,VAL_FOCCFG);
  this->writeReg(REG_BSCFG,VAL_BSCFG);
  this->writeReg(REG_AGCCTRL2,VAL_AGCCTRL2);
  this->writeReg(REG_AGCCTRL1,VAL_AGCCTRL1);
  this->writeReg(REG_AGCCTRL0,VAL_AGCCTRL0);
  this->writeReg(REG_WOREVT1,VAL_WOREVT1);
  this->writeReg(REG_WOREVT0,VAL_WOREVT0);
  this->writeReg(REG_WORCTRL,VAL_WORCTRL);
  this->writeReg(REG_FREND1,VAL_FREND1);
  this->writeReg(REG_FREND0,VAL_FREND0);
  this->writeReg(REG_FSCAL3,VAL_FSCAL3);
  this->writeReg(REG_FSCAL2,VAL_FSCAL2);
  this->writeReg(REG_FSCAL1,VAL_FSCAL1);
  this->writeReg(REG_FSCAL0,VAL_FSCAL0);
  this->writeReg(REG_RCCTRL1,VAL_RCCTRL1);
  this->writeReg(REG_RCCTRL0,VAL_RCCTRL0);
  this->writeReg(REG_FSTEST,VAL_FSTEST);
  this->writeReg(REG_TEST2,VAL_TEST2);
  this->writeReg(REG_TEST1,VAL_TEST1);
  this->writeReg(REG_TEST0,VAL_TEST0);
  this->writeReg(REG_DAFUQ,VAL_DAFUQ);

  //  this->sendStrobe(CC2500_SPWD); //Enter power down mode    -   Not used in the prototype
  this->writeReg(0x3E,0xFF);  //Maximum transmit power - write 0xFF to 0x3E (PATABLE)

  this->enableRead();

  this->_setupComplete = true;
}

void AnslutaCC2500Component::loop() {
  uint8_t marcstate = this->getMARCSTATE();

  /*
   * marcstate interessanti:
   *
   * 1  = IDLE
   * 17 = Overflow
   * 13 = RX
   * 19 = TX
   *
   */

   switch (marcstate) {
    case 17:
      this->flushRx();
    case 1:
      this->enableRead();
      break;
    case 13:
      uint8_t command= this->readRemoteCommand();
      if (command!= 0) {
        ESP_LOGD(TAG,"Data received from CC2500");
        this->enableRead();
      }
    break;
  }
}

void AnslutaCC2500Component::dump_config(){
    ESP_LOGCONFIG(TAG, "ansluta-cc2500 component");

}

}  // namespace ansluta_cc2500
}  // namespace esphome
