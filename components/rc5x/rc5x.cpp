#include "rc5x.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rc5x {


#define MIN_SHORT  444
#define MAX_SHORT 1333
#define MIN_LONG  1334
#define MAX_LONG  2222

#define MIN_RC5M  4000
#define MAX_RC5M  6000
/*
 * These step by two because it makes it
 * possible to use the values as bit-shift counters
 * when making state-machine transitions.  States
 * are encoded as 2 bits, so we step by 2.
 */
#define EVENT_SHORTSPACE  0
#define EVENT_SHORTPULSE  2
#define EVENT_LONGSPACE   4
#define EVENT_LONGPULSE   6

#define STATE_START1 0
#define STATE_MID1   1
#define STATE_MID0   2
#define STATE_START0 3

/*
 * definitions for parsing the bitstream into
 * discrete parts.  14 bits are parsed as:
 * [S1][S2][T][A A A A A][C C C C C C]
 * Bits are transmitted MSbit first.
 */
#define S2_MASK       0x1000  // 1 bit
#define S2_SHIFT      12
#define TOGGLE_MASK   0x0800  // 1 bit
#define TOGGLE_SHIFT  11
#define ADDRESS_MASK  0x7C0  //  5 bits
#define ADDRESS_SHIFT 6
#define COMMAND_MASK  0x003F //  low 6 bits
#define COMMAND_SHIFT 0

#define MSG_RC5_MARANTZ_FLAG 0x80000000

#define RC5M_VALUE_MASK 0x003F //  low 6 bits
#define RC5M_VALUE_SHIFT 6

#define MESSAGE_LENGTH_RC5 14
#define MESSAGE_LENGTH_MARANTZ 20

/* trans[] is a table of transitions, indexed by
 * the current state.  Each byte in the table
 * represents a set of 4 possible next states,
 * packed as 4 x 2-bit values: 8 bits DDCCBBAA,
 * where AA are the low two bits, and
 *   AA = short space transition
 *   BB = short pulse transition
 *   CC = long space transition
 *   DD = long pulse transition
 *
 * If a transition does not change the state,
 * an error has occured and the state machine should
 * reset.
 *
 * The transition table is:
 * 00 00 00 01  from state 0: short space->1
 * 10 01 00 01  from state 1: short pulse->0, long pulse->2
 * 10 01 10 11  from state 2: short space->3, long space->1
 * 11 11 10 11  from state 3: short pulse->2
 */
const unsigned char trans[] = {0x01,
                               0x91,
                               0x9B,
                               0xFB};


static const char *const TAG = "rc5x";

void RC5x::setup() {


  this->_pin->setup();
  _store.pin = this->_pin->to_isr();
  this->_pin->attach_interrupt(&RC5xComponentStore::rc5x_read, &_store, gpio::INTERRUPT_ANY_EDGE);

  _store.reset(&_store);
  ESP_LOGCONFIG(TAG,"Setup completed");
//  this->publish_initial_state(this->pin_->digital_read());


}
void IRAM_ATTR HOT RC5xComponentStore::reset(RC5xComponentStore* store)
{
    store->state = STATE_MID1;
    store->bits = 1;  // emit a 1 at start - see state machine graph
    store->command = 1;
    store->time0 = micros();
    store->messageLength = MESSAGE_LENGTH_RC5;
}

void RC5x::dump_config() {
//  LOG_BINARY_SENSOR("", "RC5x sensor", this);
//  LOG_PIN("  Pin: ", this->pin_);
}


void IRAM_ATTR HOT RC5xComponentStore::decodeEvent(RC5xComponentStore* store, unsigned char event)
{
    // find next state, 2 bits
// ESP_LOGD(TAG,"Event %d", event);
    unsigned char newState = (trans[store->state]>>event) & 0x3;
    if (newState==store->state) {
        // no state change indicates error, reset
        store->reset(store);
    } else {
        store->state = newState;
        if (newState == STATE_MID0) {
            // always emit 0 when entering mid0 state
            store->command = (store->command<<1)+0;
            store->bits++;
        } else if (newState == STATE_MID1) {
            // always emit 1 when entering mid1 state
            store->command = (store->command<<1)+1;
            store->bits++;
        }
    }
}

void IRAM_ATTR HOT RC5xComponentStore::rc5x_read(RC5xComponentStore* store)
{
    /* Note that the input value read is inverted from the theoretical signal,
       ie we get 1 while no signal present, pulled to 0 when a signal is detected.
       So when the value changes, the inverted value that we get from reading the pin
       is equal to the theoretical (uninverted) signal value of the time period that
       has just ended.
    */
    const bool signal = store->pin.digital_read();

    if (signal != store->lastValue) {
        unsigned long time1 = micros();
        unsigned long period = time1 - store->time0;

        store->time0 = time1;
        store->lastValue = signal;

        if (period >= MIN_SHORT && period <= MAX_SHORT) {
            store->decodeEvent(store, signal ? EVENT_SHORTPULSE : EVENT_SHORTSPACE);
        } else if (period >= MIN_LONG && period <= MAX_LONG) {
            store->decodeEvent(store, signal ? EVENT_LONGPULSE : EVENT_LONGSPACE);
        } else if ((period >= MIN_RC5M && period <= MAX_RC5M) && (store->bits == 8)) {
            // Marantz format
            store->command = (store->command<<1)+1;
            store->messageLength = MESSAGE_LENGTH_MARANTZ;
            store->bits++;
            store->state = STATE_MID1;
        } else {
            // time period out of range, reset
            store->reset(store);
        }

    }

    if (store->bits == store->messageLength) {
        store->message = store->command | (store->messageLength==MESSAGE_LENGTH_MARANTZ?MSG_RC5_MARANTZ_FLAG:0) ;
        store->command = 0;
        store->bits = 0;
    }
}


void RC5x::loop() {
    if (_store.message) {
      uint32_t message;
      unsigned char toggle;
      unsigned char address;
      unsigned char command;
      unsigned char extCode = 0;

      message = _store.message;
      _store.message = 0;
      if (_active_message != message) {
        if (_active_message) {
            ESP_LOGD(TAG,"Force Released RC5x - toggle: 0x%04x address: 0x%04x command: 0x%04x extcode: 0x%04x", _active_toggle, _active_address, _active_command, _active_extCode);
            command_release_callback_.call(_active_toggle,_active_address,_active_command, _active_extCode);
        }

        _active_message = message;

        if (message & MSG_RC5_MARANTZ_FLAG) {
            extCode = message & RC5M_VALUE_MASK;
            message = (message & ~MSG_RC5_MARANTZ_FLAG) >> RC5M_VALUE_SHIFT;
        }
        toggle  = (message & TOGGLE_MASK ) >> TOGGLE_SHIFT;
        address = (message & ADDRESS_MASK) >> ADDRESS_SHIFT;

        // Support for extended RC5:
        // to get extended command, invert S2 and shift into command's 7th bit
        unsigned char extended;
        extended = (~message & S2_MASK) >> (S2_SHIFT - 6);
        command = ((message & COMMAND_MASK) >> COMMAND_SHIFT) | extended;

        _active_toggle = toggle;
        _active_address = address;
        _active_command = command;
        _active_extCode = extCode;

        ESP_LOGD(TAG,"Received RC5x - toggle: 0x%04x address: 0x%04x command: 0x%04x extcode: 0x%04x", toggle, address, command, extCode);
        command_press_callback_.call(toggle,address,command, extCode);
      }

      _active_command_press_time = millis();

    } else {
        if ( (_active_message) && (_active_command_press_time < millis() - 200) ) {
            ESP_LOGD(TAG,"Released RC5x - toggle: 0x%04x address: 0x%04x command: 0x%04x extcode: 0x%04x", _active_toggle, _active_address, _active_command, _active_extCode);
            command_release_callback_.call(_active_toggle,_active_address,_active_command, _active_extCode);
            _active_message = 0;
        }
    }

//	this->publish_state(this->pin_->digital_read());
}

float RC5x::get_setup_priority() const { return setup_priority::HARDWARE; }

}  // namespace rc5x
}  // namespace esphome
