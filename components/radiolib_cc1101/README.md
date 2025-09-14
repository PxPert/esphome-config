```yaml
# example configuration:

radiolib_cc1101:
  id: mycc1101
  cs_pin: D8 # # CC1101 pin 4
  filter: 150khz # 58-812khz, default=464kHz
  freq: 433.92MHz # 300-348, 387-464, 779-928MHz, default=433.92MHz
  rx_pin:
    number: D2 # # This is CC1101 GDO0 pin 3
    allow_other_uses: true
  tx_pin:
    number: D2 # This is also GDO0
    allow_other_uses: true

spi:
  # these are the default SPI pins to use on ESP8266
  clk_pin: D5   # CC1001 pin 5
  mosi_pin: D7  # CC1001 pin 6
  miso_pin: D6  # CC1001 pin 7

```
