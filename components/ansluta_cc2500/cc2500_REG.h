/*

    CC2500 Register values
 
*/

#define VAL_IOCFG2           0x29
#define VAL_IOCFG0           0x06
#define VAL_PKTLEN           0xFF
#define VAL_PKTCTRL1         0x04
#define VAL_PKTCTRL0         0x05
#define VAL_ADDR             0x01
#define VAL_CHANNR           0x10
#define VAL_FSCTRL1          0x09
#define VAL_FSCTRL0          0x00
#define VAL_FREQ2            0x5D
#define VAL_FREQ1            0x93
#define VAL_FREQ0            0xB1
#define VAL_MDMCFG4          0x2D
#define VAL_MDMCFG3          0x3B
#define VAL_MDMCFG2          0x73  
#define VAL_MDMCFG1          0xA2
#define VAL_MDMCFG0          0xF8
#define VAL_DEVIATN          0x01
#define VAL_MCSM2            0x07
#define VAL_MCSM1            0x30
// #define VAL_MCSM1            0x3F // Ad ogni invio e ricezione torno in RX
#define VAL_MCSM0            0x18
#define VAL_FOCCFG           0x1D
#define VAL_BSCFG            0x1C
#define VAL_AGCCTRL2         0xC7
#define VAL_AGCCTRL1         0x00
#define VAL_AGCCTRL0         0xB2
#define VAL_WOREVT1          0x87
#define VAL_WOREVT0          0x6B
#define VAL_WORCTRL          0xF8
#define VAL_FREND1           0xB6
#define VAL_FREND0           0x10
#define VAL_FSCAL3           0xEA
#define VAL_FSCAL2           0x0A
#define VAL_FSCAL1           0x00
#define VAL_FSCAL0           0x11
#define VAL_RCCTRL1          0x41
#define VAL_RCCTRL0          0x00
#define VAL_FSTEST           0x59
#define VAL_TEST2            0x88
#define VAL_TEST1            0x31
#define VAL_TEST0            0x0B
#define VAL_DAFUQ            0xFF


/*

    CC2500 Command Strobes
 
*/

#define CC2500_SRES         0x30
#define CC2500_SFSTON       0x31
#define CC2500_SXOFF        0x32
#define CC2500_SCAL         0x33
#define CC2500_SRX          0x34
#define CC2500_STX          0x35
#define CC2500_SIDLE        0x36
#define CC2500_SWOR         0x38
#define CC2500_SPWD         0x39
#define CC2500_SFRX         0x3A
#define CC2500_SFTX         0x3B
#define CC2500_SWORRST      0x3C
#define CC2500_SNOP         0x3D
#define CC2500_FIFO         0x3F 

#define CC2500_READ_SINGLE  0x80
#define CC2500_READ_BURST   0xC0
#define CC2500_STATUS       CC2500_READ_BURST

#define CC2500_STATUS_STATE_BM                 0x70
#define CC2500_STATE_TX                        0x20

/*

    CC2500 Register settings
 
*/

#define REG_IOCFG2           0x00
#define REG_IOCFG1           0x01
#define REG_IOCFG0           0x02
#define REG_FIFOTHR          0x03
#define REG_SYNC1            0x04
#define REG_SYNC0            0x05
#define REG_PKTLEN           0x06
#define REG_PKTCTRL1         0x07
#define REG_PKTCTRL0         0x08
#define REG_ADDR             0x09
#define REG_CHANNR           0x0A
#define REG_FSCTRL1          0x0B
#define REG_FSCTRL0          0x0C
#define REG_FREQ2            0x0D
#define REG_FREQ1            0x0E
#define REG_FREQ0            0x0F
#define REG_MDMCFG4          0x10
#define REG_MDMCFG3          0x11
#define REG_MDMCFG2          0x12
#define REG_MDMCFG1          0x13
#define REG_MDMCFG0          0x14
#define REG_DEVIATN          0x15
#define REG_MCSM2            0x16
#define REG_MCSM1            0x17
#define REG_MCSM0            0x18
#define REG_FOCCFG           0x19
#define REG_BSCFG            0x1A
#define REG_AGCCTRL2         0x1B
#define REG_AGCCTRL1         0x1C
#define REG_AGCCTRL0         0x1D
#define REG_WOREVT1          0x1E
#define REG_WOREVT0          0x1F
#define REG_WORCTRL          0x20
#define REG_FREND1           0x21
#define REG_FREND0           0x22
#define REG_FSCAL3           0x23
#define REG_FSCAL2           0x24
#define REG_FSCAL1           0x25
#define REG_FSCAL0           0x26
#define REG_RCCTRL1          0x27
#define REG_RCCTRL0          0x28
#define REG_FSTEST           0x29
#define REG_PTEST            0x2A
#define REG_AGCTEST          0x2B
#define REG_TEST2            0x2C
#define REG_TEST1            0x2D
#define REG_TEST0            0x2E
#define REG_PARTNUM          0x30
#define REG_VERSION          0x31
#define REG_FREQEST          0x32
#define REG_LQI              0x33
#define REG_RSSI             0x34
#define REG_MARCSTATE        0x35
#define REG_WORTIME1         0x36
#define REG_WORTIME0         0x37
#define REG_PKTSTATUS        0x38
#define REG_VCO_VC_DAC       0x39
#define REG_TXBYTES          0x3A
#define REG_RXBYTES          0x3B
#define REG_RCCTRL1_STATUS   0x3C
#define REG_RCCTRL0_STATUS   0x3D
#define REG_DAFUQ            0x7E
