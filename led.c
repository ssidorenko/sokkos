/* 
 * The software for the x0xb0x is available for use in accordance with the 
 * following open source license (MIT License). For more information about
 * OS licensing, please visit -> http://www.opensource.org/
 *
 * For more information about the x0xb0x project, please visit
 * -> http://www.ladyada.net/make/x0xb0x
 *
 *                                     *****
 * Copyright (c) 2005 Limor Fried
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *                                     *****
 *
 */

#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include "main.h"
#include "switch.h"
#include "led.h"
#include "synth.h"

uint8_t leds[5] = {0,0,0,0,0};
uint8_t blinkleds[5] = {0,0,0,0,0};

const static uint8_t bank_led_tab[16] = {
  LED_BANK1,
  LED_BANK2,
  LED_BANK3,
  LED_BANK4,
  LED_BANK5,
  LED_BANK6,
  LED_BANK7,
  LED_BANK8,
  LED_BANK9,
  LED_BANK10,
  LED_BANK11,
  LED_BANK12,
  LED_BANK13,
  LED_BANK14,
  LED_BANK15,
  LED_BANK16
};

const static uint8_t key_led_tab[24] = {
  LED_CHAIN,
  LED_RS,
  LED_TEMPO,
  LED_PREV,
  LED_C,
  LED_D ,
  LED_E,
  LED_F,

  LED_NEXT,
  LED_CS, 
  LED_DS, 
  LED_FS, 
  LED_GS, 
  LED_AS, 
  LED_DOWN, 
  LED_UP, 

  LED_B, 
  LED_G, 
  LED_A, 
  LED_C2,
  LED_REST,
  LED_ACCENT,
  LED_SLIDE,
  LED_DONE
};

// table for converting notes (C = 0, C2 = 12) into leds
const static uint8_t notekey_led_tab[13] = {
  LED_C,
  LED_CS,
  LED_D,
  LED_DS,
  LED_E,
  LED_F,
  LED_FS,
  LED_G,
  LED_GS,
  LED_A,
  LED_AS,
  LED_B,
  LED_C2
};

// table for converting numbered keys into leds
const static uint8_t numkey_led_tab[8] = {
  LED_C,
  LED_D,
  LED_E,
  LED_F,
  LED_G,
  LED_A,
  LED_B,
  LED_C2
};

static void set_led_proto(uint8_t ledno, uint8_t* led_array) {
  if (ledno >= MAX_LED)
    return;
  led_array[ledno / 8] |= 1 << (ledno % 8);
}

void set_led(uint8_t ledno) {
  set_led_proto(ledno, leds);
}

void set_led_blink(uint8_t ledno) {
  set_led_proto(ledno, blinkleds);
}


void clear_led_blink(uint8_t ledno) {
  if (ledno >= MAX_LED)
    return;
  blinkleds[ledno / 8] &= ~_BV(ledno % 8);
}

uint8_t is_led_blink(uint8_t ledno) {
  if (ledno >= MAX_LED)
    return 0;
  return blinkleds[ledno / 8] & _BV(ledno % 8);
}

void clear_led(uint8_t ledno) {
  if (ledno >= MAX_LED)
    return;
  leds[ledno / 8] &= ~(1 << (ledno % 8));
}

int is_led_set(uint8_t ledno) {
  return (leds[ledno/8] >> (ledno % 8)) & 0x1;
}

void clear_all_leds(void) {
  leds[0] = leds[1] = leds[2] = leds[3] = leds[4] = 0;
  blinkleds[0] = blinkleds[1] = blinkleds[2] = blinkleds[3] = blinkleds[4] = 0;
}

// bank leds (strip of 16 above keys)

void clear_bank_leds(void) {
  uint8_t  i;
  for (i=0; i<16; i++) {
    clear_led(bank_led_tab[i]);
  }
}

void set_bank_led(uint8_t num) {
  if (num >= 16)
    return;
  set_led(bank_led_tab[num]);
}

void set_bank_led_blink(uint8_t num) {
  if (num >= 16)
    return;
  set_led_blink(bank_led_tab[num]);
}


uint8_t is_bank_led_set(uint8_t num) {
 if (num >= 16)
    return 0;
  return is_led_set(bank_led_tab[num]);
}

uint8_t is_bank_led_blink(uint8_t num) { 
  if (num >= 16)
    return 0;
  return is_led_blink(bank_led_tab[num]);
}

// key leds (all but tempo/bank)
void set_key_led(uint8_t num) {
  if (num >= 24)
    return;
  set_led(key_led_tab[num]);
}

void set_key_led_blink(uint8_t num) {
  if (num >= 24)
    return;
  set_led_blink(key_led_tab[num]);
}

void clear_key_led(uint8_t num) {
  if (num >= 24)
    return;
  clear_led(key_led_tab[num]);
}

void clear_key_leds(void) {
  uint8_t i;
  for (i=0; i<24; i++) {
    clear_led(key_led_tab[i]);
  }
}

// numbered keys (bottom row 1 thru 8)

void set_numkey_led(uint8_t num) {
  if ((num >= 1) && (num <= 8))
    set_led(numkey_led_tab[num-1]);   // num is 1 thru 8
}

void set_single_numkey_led(uint8_t num) {
  uint8_t i;
  for (i=1; i <= 8; i++)
    if (i == num) 
      set_led(numkey_led_tab[i-1]);
    else
      clear_led(numkey_led_tab[i-1]);
}

void clear_numkey_led(uint8_t num) {
  if ((num >= 1) && (num <= 8))
    clear_led(numkey_led_tab[num-1]);   // num is 1 thru 8
}

void set_numkey_led_blink(uint8_t num) {
  if ((num >= 1) && (num <= 8))
    set_led_blink(numkey_led_tab[num-1]);
}

uint8_t is_numkey_led_blink(uint8_t num) {
  if ((num >= 1) && (num <= 8))
    return is_led_blink(numkey_led_tab[num-1]);
  return 0;
}


uint8_t is_numkey_led_set(uint8_t num) {
  if ((num >= 1) && (num <= 8))
    return is_led_set(numkey_led_tab[num-1]);
  return 0;
}

void clear_numkey_leds(void) {
  uint8_t i;
  for (i = 0; i < 8; i++) {
    clear_led(numkey_led_tab[i]);
  }
}

// note keys (C thru C')

void set_notekey_led(uint8_t num) {
  if (num <= 12)
    set_led(notekey_led_tab[num]);
}

void clear_notekey_led(uint8_t num) {
  if (num <= 12)
    clear_led(notekey_led_tab[num]);
}

void set_notekey_led_blink(uint8_t num) {
  if (num <= 12)
    set_led_blink(notekey_led_tab[num]);
}

uint8_t is_notekey_led_blink(uint8_t num) {
  return is_led_blink(notekey_led_tab[num]);
}

void clear_notekey_leds(void) {
  uint8_t i;
  for (i = 0; i < 13; i++) {
    clear_led(notekey_led_tab[i]);
  }
}

// note leds (notes, U, D, RAS)
void clear_note_leds(void) {
  clear_notekey_leds();
  clear_led(LED_DOWN);
  clear_led(LED_UP);
  clear_led(LED_REST);
  clear_led(LED_ACCENT);
  clear_led(LED_SLIDE);
}

void set_note_led(uint8_t note) {
  int8_t shift;          // our octave shift

  // if slide, turn on slide LED
  if (note>>7)
    set_key_led(KEY_SLIDE);
  else
    clear_key_led(KEY_SLIDE);

  // if accent, turn on accent LED
  if ((note>>6) & 0x1) 
    set_key_led(KEY_ACCENT);
  else
    clear_key_led(KEY_ACCENT);

  note &= 0x3F;
  if (note == 0) {
    shift = 0;
  } else if (note < C2) {
    shift = -1;
  } else if (note <= C3) {
    note -= OCTAVE;
    shift = 0;
  } else if (note <= C4) {
    note -= 2*OCTAVE;
    shift = 1;
  } else if (note <= C5) {
    note -= 3*OCTAVE;
    shift = 2;
  } else {
    shift = 3;
    note -= 4*OCTAVE;
  }

  display_octave_shift(shift);

  // figure out what led to light

  if (note == REST) {
    clear_notekey_leds();
    set_key_led(KEY_REST);
  } else {
    clear_key_led(KEY_REST);
    for (shift = C1; shift <= C2; shift++) {
      if (shift != note)
	clear_led(notekey_led_tab[shift - C1]);
    }
    set_led(notekey_led_tab[note - C1]);
  }
}


void clock_leds(void) {
  int i;

  cli();
  cbi(LED_LATCH_PORT, LED_LATCH_PIN);
  for (i=0; i<5; i++) {
    SPDR = leds[i];
    while (!(SPSR & (1<<SPIF)));
  }
  sbi(LED_LATCH_PORT, LED_LATCH_PIN);
  sei();
}

void blink_leds_on(void) {
  uint8_t i;

  for (i=0; i<5; i++)
    leds[i] |= blinkleds[i];
}

void blink_leds_off(void) {
  uint8_t i;

  for (i=0; i<5; i++)
    leds[i] &= ~blinkleds[i];
}

void clear_blinking_leds(void) {
  uint8_t i;

  for (i=0; i<5; i++) {
    if (leds[i] & blinkleds[i])
      leds[i] &= ~blinkleds[i];
    blinkleds[i] = 0;
  }
}

void display_octave_shift(int8_t shift) {
  clear_led(LED_DOWN);
  if (shift == 2) {
    set_led_blink(LED_UP);
  } else {
    clear_led(LED_UP);
    clear_led_blink(LED_UP);
    
    if (shift == 1) 
      set_led(LED_UP);
    else if (shift == -1)
      set_led(LED_DOWN);
  }
}
