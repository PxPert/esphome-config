 
#include "esphome.h"
#include "BluetoothA2DPSink.h"
/*
class Advanced_bt_Component : public Component
{
public:
    BluetoothA2DPSink a2dp_sink;

    Advanced_bt_Component(esphome::template_::TemplateSwitch *&_enable){
        _enable->add_on_state_callback([this](bool newState)
        {
            if (newState){
                i2s_pin_config_t my_pin_config = {
                    .bck_io_num = 14,
                    .ws_io_num = 15,
                    .data_out_num = 22,
                    .data_in_num = I2S_PIN_NO_CHANGE};
                a2dp_sink.set_pin_config(my_pin_config);
                a2dp_sink.start("Kitchen Radio");
            }
            else
                a2dp_sink.end(true);
        });
    }

    void setup() override {
    }

    void loop() override {
    }
};
*/
