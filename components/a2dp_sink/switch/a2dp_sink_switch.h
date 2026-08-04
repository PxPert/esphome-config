#pragma once

#include "../a2dp_sink_hub.h"
#include "esphome/components/switch/switch.h"

namespace esphome::a2dp_sink {

/**
 * @brief Switch che riflette lo stato di connessione A2DP dei client.
 *
 * Lo stato è solo di lettura: quando un client si connette o disconnette,
 * l'hub invoca il callback e lo switch aggiorna il proprio stato.
 */
class A2DPSwitchConnection : public A2DPSinkChild, public switch_::Switch {
 public:
  void dump_config() override;
  void setup() override;
  void write_state(bool state) override;

 protected:
  void publish_state_if_changed_(bool state);
};

/**
 * @brief Switch che accende/spegne l'interfaccia Bluetooth.
 *
 * Lo stato è completamente controllato dall'utente: ON chiama
 * a2dp_sink_->start(), OFF chiama a2dp_sink_->stop().
 */
class A2DPSwitchBluetooth : public A2DPSinkChild, public switch_::Switch {
 public:
  void dump_config() override;
  void setup() override;
  void write_state(bool state) override;
  float get_setup_priority() const override { return a2dp_sink_priority::CHILD - 1.0f; }

 protected:
  void publish_state_if_changed_(bool state);
};

}  // namespace esphome::a2dp_sink