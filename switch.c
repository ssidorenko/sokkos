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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "switch.h"
#include "led.h"
#include "main.h"
#include "delay.h"
 
uint8_t last_tempo;
uint8_t switches[3];
uint8_t last_switches[3];
uint8_t pressed_switches[3];
uint8_t released_switches[3];

uint8_t bank_switched = 0;   // has the bank knob been moved?
uint8_t function, bank;
uint8_t last_func, last_bank;

extern uint16_t tempo;/* removed volatile */
extern uint16_t newtempo; // changed by the interrupt then updated to the current tempo?/* removed volatile */

volatile uint8_t debounce_timer = 0;     // modified by timer0 in main.c


const static uint8_t numkey_tab[8] = {
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_A,
  KEY_B,
  KEY_C2
};

const uint8_t loopkey_tab[16] = {
  KEY_C,
  KEY_CS,
  KEY_D,
  KEY_DS,
  KEY_E,
  KEY_F,
  KEY_FS,
  KEY_G,
  KEY_GS,
  KEY_A,
  KEY_AS,
  KEY_B,
  KEY_C2,
  KEY_REST,
  KEY_ACCENT,
  KEY_SLIDE
};

void read_switches(void) {
  uint8_t i, s, l, t;
  uint8_t temp_switches[3];
  uint8_t keypress_switches[3];

  // check if tempo has been changed by interrupt
  if (newtempo != tempo)
    change_tempo(newtempo);

  for (i = 0; i < 3; i++) 
    {
      pressed_switches[i] = 0;
      released_switches[i] = 0;
    }
  // to debounce switches, check if it's been more than 20 ms since
  // the last check, and that the switch is the same as it was 20ms ago.
  if (debounce_timer < 20)  // timer is in 1ms incr
    return;                // we only want to be called every 20 ms or more

  debounce_timer = 0;        // reset the timer

  select_bank_read(); // wait a bit then call read_bank

  read_keypad(temp_switches);
  keypress_switches[0] = switches[0];
  keypress_switches[1] = switches[1];
  keypress_switches[2] = switches[2];
  for (i=0; i<24; i++) {
    s = (switches[i/8] & (1 <<(i % 8)));
    l = (last_switches[i/8] & (1 <<(i % 8)));
    t = (temp_switches[i/8] & (1 <<(i % 8)));
    //printf("%d: s %x, l %x, t %x\n\r", i, s, l, t);
    if (s != 0) {
      switches[i/8] = (switches[i/8] & ~(1 << (i % 8))) | (l | t);
    } else {
      switches[i/8] = (switches[i/8] & ~(1 << (i % 8))) | (l & t);
    }
  }

  i = read_bank();
  if ((i != bank) && (i == last_bank)) {
    bank = i;
    bank_switched = 1;
    //putstring("Bank #"); putnum_ud(bank); putstring("\n\r");
  }
  last_bank = i;

  select_func_read();

  for (i=0; i<3; i++) {
    last_switches[i] = temp_switches[i];

    pressed_switches[i] = (keypress_switches[i] ^ switches[i]) & switches[i];
    released_switches[i] = (keypress_switches[i] ^ switches[i]) & keypress_switches[i];
  }
  /*printf("pressed %x %x %x, released %x %x %x\n\r",
	 pressed_switches[0], pressed_switches[1], pressed_switches[2],
	 released_switches[0], released_switches[1], released_switches[2]);
  */


  i = read_function();
  if ((i != function) && (i == last_func)) {
    function = i;
    //putstring("Function #"); putnum_ud(function); putstring("\n\r");
  }
  last_func = i;

  clock_leds();
}

void read_keypad(uint8_t *switchinput) {
  uint8_t i;

  cli();

  cbi(SWITCH_LATCH_PORT, SWITCH_LATCH_PIN);
  NOP; NOP; NOP; NOP;
  sbi(SWITCH_LATCH_PORT, SWITCH_LATCH_PIN);
  for (i=0; i<3; i++) {
    SPDR = 0;
    while (!(SPSR & (1<<SPIF)));
    switchinput[i] = SPDR;
  }
  //printf("got %2x %2x %2x\n\r", switches[2], switches[1], switches[0]);

  sei();
}

// we need to call this, then wait a bit, then read the value off the pins
void select_bank_read(void) {
  BANK_COMMON_PORT &= ~_BV(BANK_COMMON_PIN);
}

// we need to call this, then wait a bit, then read the value off the pins
void select_func_read(void) {
  FUNC_COMMON_PORT |= _BV(FUNC_COMMON_PIN);
}

uint8_t read_bank() {
  uint8_t val;

  val = BANK_PIN;
  val = (val >> 2) & 0xF;
  val = (val >> 3) | ((val >> 1)&0x2) | ((val << 1)&0x4) | ((val << 3)&0x8);
  return 15-val;
}

uint8_t read_function() {
  uint8_t val;

  val = FUNC_PIN;
  val = (val >> 2) & 0xF;

  return val;
}

// prototype for key pressing/releasing detection
uint8_t key_action(uint8_t key, uint8_t* keyvec) { 
  if (key >= 24)
    return 0;

  if ((keyvec[key/8] & (1 << key%8)) != 0)
    return 1;
  else
    return 0;
}

// returns 1 if that key is pressed
uint8_t is_pressed(uint8_t key) { 
  return key_action(key, switches);
}

uint8_t just_pressed(uint8_t key) { 
  return key_action(key, pressed_switches);
}

uint8_t just_released(uint8_t key) { 
  return key_action(key, released_switches);
}

uint8_t no_keys_pressed(void) {
  if ((switches[0] == 0) && (switches[1] == 0) && (switches[2] == 0))
    return 1;
  return 0;
}

static uint8_t get_lowest_key_pressed(const uint8_t* key_tab, uint8_t size) {
  uint8_t i;
  for (i = 0; i < size; i++) {
    if (is_pressed(key_tab[i]))
      return i+1;
  }
  return 0;
}

int8_t get_lowest_notekey_pressed(void) {
  return get_lowest_key_pressed(loopkey_tab, 13)-1;
}

uint8_t get_lowest_numkey_pressed(void) {
  return get_lowest_key_pressed(numkey_tab, 8);
}


static uint8_t get_lowest_key_just_pressed(const uint8_t* key_tab, uint8_t size) {
  uint8_t i;
  for (i = 0; i < size; i++) {
    if (just_pressed(key_tab[i]))
      return i+1;
  }
  return 0;
}

uint8_t get_lowest_numkey_just_pressed(void) {
  return get_lowest_key_just_pressed(numkey_tab, 8);
}

uint8_t get_lowest_loopkey_just_pressed(void) {
  return get_lowest_key_just_pressed(loopkey_tab, 16);
}

uint8_t get_lowest_notekey_just_pressed(void) {
  return get_lowest_key_just_pressed(loopkey_tab, 13);
}

uint8_t has_bank_knob_changed(void) {
  uint8_t temp = bank_switched;
  bank_switched = 0;
  return temp;
}
