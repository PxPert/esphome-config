/*
 * the following adapted from
 * https://community.home-assistant.io/t/share-your-esphome-light-effects/250294/14
 */
#include <math.h>

#include "esphome.h"

#define PMC_LETTER(arr) { arr, sizeof(arr) }

bool letters_ready = false;

struct PMCLetter {
    const byte *pixels;
    byte pixels_count;
};

static const byte pixels_0_0_P[] = { 0,1,2,3,4,5,6,7,8,9 };
static const byte pixels_0_1_U[] = { 20,19,18,17,16,15,14,13,12,11,10 };
static const byte pixels_0_2_B[] = { 21,22,23,24,25,26,27,28,29,30,31,32,33,34 };
static const byte pixels_0_3_L[] = { 41,40,39,38,37,36,35 };
static const byte pixels_0_4_I[] = { 42,43,44,45,46 };
static const byte pixels_0_5_C[] = { 47,48,49,50,51,52,53,54 };

static const byte pixels_1_0_M[] = { 55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72 };
static const byte pixels_1_1_A[] = { 73,74,75,76,77,78,79,80,81,82,83,84,85 };
static const byte pixels_1_2_R[] = { 86,87,88,89,90,91,92,93,94,95,96,97,98,99 };
static const byte pixels_1_3_K[] = { 100,101,102,103,104,105,106,107,108,109,110,111 };
static const byte pixels_1_4_E[] = { 112,113,114,115,116,117,118,119,120,121,122 };
static const byte pixels_1_5_T[] = { 123,124,125,126,127,128,129,130 };

static const byte pixels_2_0_C[] = { 131,132,133,134,135,136,137,138 };
static const byte pixels_2_1_E[] = { 148,147,146,145,144,143,142,141,140,139 };
static const byte pixels_2_2_N[] = { 149,150,151,152,153,154,155,156,157,158,159,160,161,162 };
static const byte pixels_2_3_T[] = { 163,164,165,166,167,168,169,170 };
static const byte pixels_2_4_E[] = { 171,172,173,174,175,176,177,178,179,180 };
static const byte pixels_2_5_R[] = { 193,192,191,190,189,188,187,186,185,184,183,182,181 };


static struct PMCLetter letters[] = {
    PMC_LETTER(pixels_0_0_P),
    PMC_LETTER(pixels_0_1_U),
    PMC_LETTER(pixels_0_2_B),
    PMC_LETTER(pixels_0_3_L),
    PMC_LETTER(pixels_0_4_I),
    PMC_LETTER(pixels_0_5_C),

    PMC_LETTER(pixels_1_0_M),
    PMC_LETTER(pixels_1_1_A),
    PMC_LETTER(pixels_1_2_R),
    PMC_LETTER(pixels_1_3_K),
    PMC_LETTER(pixels_1_4_E),
    PMC_LETTER(pixels_1_5_T),

    PMC_LETTER(pixels_2_0_C),
    PMC_LETTER(pixels_2_1_E),
    PMC_LETTER(pixels_2_2_N),
    PMC_LETTER(pixels_2_3_T),
    PMC_LETTER(pixels_2_4_E),
    PMC_LETTER(pixels_2_5_R),
};

static const byte letters_count = 18;
static const byte words_count = 3;

static void light_letter(AddressableLight &it,
                         const Color &current_color,
                         byte selected_letter) {
  for (byte i = 0; i < letters[selected_letter].pixels_count; i++) {
    it[letters[selected_letter].pixels[i]] = current_color;
  }
}

static void single_letter(AddressableLight &it,
                              const Color &current_color,
                              bool clear, byte index = min((byte)(id(selected_led_idx).state),(byte)(letters_count - 1))) {
/*
  ESP_LOGD("single_letter","Light effect refresh");
  static byte current_letter = 0;
  if ((! initial_run) && (selected_letter == current_letter) ) {
    return;
  }
  current_letter = selected_letter;
*/
//  ESP_LOGD("single_letter","Light effect applying");
  if (clear) {
    it.range(0, it.size()) = Color::BLACK;
  }
  light_letter(it,current_color, index);



}

static void single_word(AddressableLight &it,
                              const Color &current_color,
                              bool clear, byte index = min((byte)(id(selected_led_idx).state),(byte)(words_count - 1))) {

/*
  ESP_LOGD("single_letter","Light effect refresh");
  static byte current_word = 0;
  if ((! initial_run) && (selected_word == current_word) ) {
    return;
  }
  current_word = selected_word;
*/
//  ESP_LOGD("single_letter","Light effect applying");
  if (clear) {
    it.range(0, it.size()) = Color::BLACK;
  }

  for (byte i = 0; i < 6; i++) {
    light_letter(it,current_color, (index * 6) + i);
  }

}

static void words_effect_1(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {

  static byte step = 0;
  if (initial_run) {
    step = 0;
  }

  switch (step) {
    case 0:
      it.range(0, it.size()) = Color::BLACK;
      break;
    case 3:
    case 4:
    case 5:
      single_word(it,current_color,false, step -3);
      break;
    default:
      break;
  }
  step++;

  if (step == 9) {
    step = 0;
  }

}

static void letter_effect_1(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {

  static unsigned int step = 0;
  static const unsigned int letter_start_step = 3;
  static const unsigned int letter_start_blink = letter_start_step + letters_count + 5;
  static const unsigned int letter_stop_blink = letter_start_blink + 10;
  static const unsigned int letter_start_black = letter_stop_blink + 16;
  static const unsigned int letter_stop_step = letter_start_black + letters_count + 8;
  static const byte blink_multiplier = 1;

  if (initial_run) {
    step = 0;
  }

  if (step == 0) {
    it.range(0, it.size()) = Color::BLACK;
  } else if ((step > letter_start_step) && (step < letters_count + letter_start_step + 1)) {
    single_letter(it,current_color,false, step - letter_start_step - 1);
  } else if ((step > letter_start_blink) && (step < letter_stop_blink)) {
    if (step % blink_multiplier == 0) {
      it.range(0, it.size()) = ((step - letter_start_blink) / blink_multiplier)  % 2 == 0?Color::BLACK:current_color;

    }
  } else if (step == letter_stop_blink) {
    it.range(0, it.size()) = current_color;
  } else if ((step > letter_start_black) && (step < letters_count + letter_start_black + 1 )) {
    single_letter(it,Color::BLACK,false, step - letter_start_black - 1);
  }


  step++;
  if (step == letter_stop_step) {
    step = 0;
  }

}

static void letter_effect_2(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {

  static unsigned int step = 0;
  static const unsigned int letter_start_black = letters_count + 16;
  static const unsigned int letter_stop_step = letter_start_black + letters_count + 4;

  static uint32_t letters_on = 0;
  if (initial_run) {
    step = 0;
  }

  if (step == 0) {
    it.range(0, it.size()) = Color::BLACK;
    letters_on = 0;

  } else if (step < letters_count + 1) {
      byte letter = random(1, (letters_count + 1) - step);
      byte letter_idx = 0;
      while (letter > 0) {
        letter_idx++;
        if ((letters_on & (1 << letter_idx)) == 0) {
          letter--;
        }
      }
      letters_on = letters_on | (1 << letter_idx);
      single_letter(it,current_color,false, letter_idx - 1);

  } else if ((step > letter_start_black) && (step < letter_start_black + letters_count + 1)) {
      byte letter = random(1, (letters_count + 1) - (step - letter_start_black) );
      byte letter_idx = 0;
      while (letter > 0) {
        letter_idx++;
        if (letters_on & (1 << letter_idx)) {
          letter--;
        }
      }
      letters_on = letters_on ^ (1 << letter_idx);
      single_letter(it,Color::BLACK,false, letter_idx - 1);
  }


  step++;
  if (step == letter_stop_step) {
    step = 0;
  }

}


static void pixel_effect_1(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {

  static byte current_letter_idx = 0;
  static byte current_pixel_idx = 0;
  static byte step = 0;

  if (initial_run) {
    step = 0;
  }

  switch (step) {
    case 0:
      it.range(0, it.size()) = Color::BLACK;
      current_letter_idx = 0;
      current_pixel_idx = 0;
      step++;
      break;
    case 1:
      it[letters[current_letter_idx].pixels[current_pixel_idx]] = current_color;
      current_pixel_idx++;
      if (current_pixel_idx == letters[current_letter_idx].pixels_count) {
        current_pixel_idx = 0;
        current_letter_idx++;
        if (current_letter_idx == letters_count) {
          current_letter_idx = 0;
          step++;
        }
      }
      break;
    case 100:
      it[letters[current_letter_idx].pixels[current_pixel_idx]] = Color::BLACK;
      current_pixel_idx++;
      if (current_pixel_idx == letters[current_letter_idx].pixels_count) {
        current_pixel_idx = 0;
        current_letter_idx++;
        if (current_letter_idx == letters_count) {
          current_letter_idx = 0;
          step++;
        }
      }
      break;
    default:
      step++;
      break;
  }

  if (step == 200) {
    step = 0;
  }



}


static void single_led(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {

  byte selected_led = min((byte)(id(selected_led_idx).state),(byte)(it.size() - 1));
/*
  ESP_LOGD("light","Light effect refresh");
  static byte current_led = 0;
  if ((! initial_run) && (selected_led == current_led) ) {
    return;
  }
  current_led = selected_led;
*/
//  ESP_LOGD("light","Light effect applying");

  it.range(0, it.size()) = Color::BLACK;

  it[selected_led] = current_color;

}

static byte broken_effect_helper(byte* neon_state, byte ko_multiplier = 1) {
  // ogni tick decide se sfarfallare
  if (random(0, 100) < 3) {
    *neon_state = random(1, 6 * ko_multiplier);
  }
  byte b = 255;

  if (*neon_state == 1) b = 76;      // lampo debole
  else if (*neon_state == 2) b = 155; // media intensità
  else if (*neon_state == 3) {
    b = 0;                               // breve spegnimento
    *neon_state = 0;                  // reset dopo il flash
  }

  return b;

}
static void broken_neon(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {
  static byte neon_state = 0;
  if (initial_run) {
    neon_state = 0;
  }

  it.range(0, it.size()) = current_color * broken_effect_helper(&neon_state);

}

static void broken_letters(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {
  static byte neon_state[letters_count] = {0};
  if (initial_run) {
    memset(neon_state, 0, sizeof(neon_state));
  }

  for (byte i = 0 ; i < letters_count; i++) {
    single_letter(it,current_color * broken_effect_helper(&neon_state[i], 5), false, i);
  }

}

static void broken_words(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {
  static byte neon_state[words_count] = {0};
  if (initial_run) {
    memset(neon_state, 0, sizeof(neon_state));
  }

  for (byte i = 0 ; i < words_count; i++) {
    single_word(it,current_color * broken_effect_helper(&neon_state[i],3), false, i);
  }

}

static void neon_startup(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {
  static int step = 0;
  static bool active = true;

  static const int FIRST_RUN_STEPS = 72;
  static const float ALL_OFF_STEPS = 60.0;
  static const float FADE_STEPS = 30.0;


  if (initial_run) {
    active = true;
    letters_ready = false;
  }

  if (!active) return; // effetto non attivo

  // fase 1: sfarfallio iniziale (simula la scintilla)
  if (step < FIRST_RUN_STEPS) {
    if (step % 3 == 0) {
      byte b = random(1,5);
      // byte b = (step % 2 == 0) ? 0 : 76; // alterna acceso/spento
      it.all() = current_color * (30 * b);
    }
    step++;
    return;
  }

  if (step < FIRST_RUN_STEPS + ALL_OFF_STEPS) {
    if (step == FIRST_RUN_STEPS)
      it.all() = Color::BLACK;
    step++;
    return;
  }

  // fase 2: fade in verso luminosità massima con piccoli jitter
  float progress = (step - (FIRST_RUN_STEPS + ALL_OFF_STEPS)) / FADE_STEPS;
  letters_ready = true;
  if (progress > 1.0) progress = 1.0;

  // aggiunge piccole variazioni casuali
  float jitter = (random(-5, 5) / 100.0);
  float brightness = progress + jitter;
  if (brightness > 1.0) brightness = 1.0;
  if (brightness < 0.0) brightness = 0.0;

  it.all() = current_color * (255 * brightness);

  step++;

  // fine effetto, reset per poterlo riavviare
  if (step > (FADE_STEPS + FIRST_RUN_STEPS + ALL_OFF_STEPS)) {
    active = false;
    step = 0;
    it.all() = current_color;
  }
}

static void current_time_numbers(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {
  auto time = id(homeassistant_time).now();
  byte cur_minute_index = time.minute / 5;
  byte next_minute_index = cur_minute_index == 11?0:cur_minute_index+1;
  const byte brightness_off = 64;


  float cur_minute_brightness = 1.0 - ( ((time.minute + (time.second / 60.0)) / 5.0) - cur_minute_index);

  byte cur_hour_index = time.hour > 12 ? time.hour - 12:time.hour;

  ESP_LOGD("current_time_numbers","Current hour:minute: %d:%d hour index: %d - current minute index: %d / %.2f - next minute index: %d / %.2f",
           time.hour,time.minute, cur_hour_index,cur_minute_index, cur_minute_brightness, next_minute_index, 1.0 - cur_minute_brightness );

  for (byte i = 0; i < it.size(); i++) {
    if (i == cur_minute_index) {
      it[i] = current_color * max((byte)(255 * cur_minute_brightness), brightness_off);
    } else if (i == next_minute_index) {
      it[i] = current_color * max((byte)(255 * (1.0 - cur_minute_brightness)),brightness_off);
    } else if (i == cur_hour_index) {
      it[i] = current_color;
    } else {
      it[i] = current_color * brightness_off;
    }
  }

}

static byte ring_led_from_minute(byte minute) {
  static const byte TOTAL_LEDS = 38;
  static const byte LED_12 = 36;

  return (byte)((LED_12 + TOTAL_LEDS - (byte)round((float)minute * TOTAL_LEDS / 60)) % TOTAL_LEDS);

}

static byte ring_led_hour(byte hour, byte minute) {
  static const byte TOTAL_LEDS = 38;
  static const byte LED_12 = 36;
  // Calcolo posizione in "frazione di ore" (es. 3:30 = 3.5)
  float hour_fraction = (hour % 12) + (float)minute / 60.0;

  // LED corrispondente all’ora con frazione
  return (byte)((LED_12 + TOTAL_LEDS - (byte)round(hour_fraction * TOTAL_LEDS / 12)) % TOTAL_LEDS);
}



static void current_time_external_ring_old(AddressableLight &it,
                              const Color &current_color,
                              bool initial_run) {
  static const byte ring_leds_count = 38;
  static const float divider_minutes = 60.0 / ring_leds_count;

  auto time = id(homeassistant_time).now();

  byte cur_minute_index = ring_led_from_minute(time.minute);
  byte cur_hour_index = ring_led_hour(time.hour > 12 ? time.hour - 12:time.hour,time.minute);
  ESP_LOGD("current_time_ring","Current hour:minute: %d:%d hour index: %d - current minute index: %d",
           time.hour,time.minute, cur_hour_index,cur_minute_index);
  for (byte i = 0; i < it.size(); i++) {
    if (i == cur_minute_index) {
      it[i] = current_color;
    } else if (i == cur_hour_index) {
      it[i] = current_color;
    } else {
      it[i] = Color::BLACK;
    }

  }


}


static void current_time_external_ring(AddressableLight &it,
                                       const Color &current_color,
                                       bool initial_run) {
    static const byte ring_leds_count = 38;
    static const byte LED_12 = 36; // LED corrispondente alle 12

    auto time = id(homeassistant_time).now();

    // Posizioni fluide con secondi inclusi
    float minute_pos = LED_12 + ring_leds_count - ((time.minute + (float)time.second / 60.0f) * ring_leds_count / 60.0f);
    float hour_pos   = LED_12 + ring_leds_count - (((time.hour % 12) + (float)time.minute / 60.0f + (float)time.second / 3600.0f) * ring_leds_count / 12.0f);

    minute_pos = fmodf(minute_pos, ring_leds_count);
    hour_pos   = fmodf(hour_pos, ring_leds_count);
    ESP_LOGD("current_time_ring","Current hour:minute: %d:%d",time.hour,time.minute);

    for (byte i = 0; i < it.size(); i++) {
        // distanza minima circolare
        float dist_minute = fminf(fabsf(i - minute_pos), ring_leds_count - fabsf(i - minute_pos));
        float dist_hour   = fminf(fabsf(i - hour_pos),   ring_leds_count - fabsf(i - hour_pos));

        float fade_range = 2.0f; // regola l'alone
        unsigned char b_minute = (dist_minute <= fade_range) ? (unsigned char)((1.0f - dist_minute / fade_range) * 255.0f) : 0;
        unsigned char b_hour   = (dist_hour   <= fade_range) ? (unsigned char)((1.0f - dist_hour   / fade_range) * 255.0f) : 0;

        unsigned char brightness = b_minute > b_hour ? b_minute : b_hour;

        it[i] = current_color * brightness;
    }
}
