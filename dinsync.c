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
#include "dinsync.h"
#include "main.h"
#include "delay.h"

// counter: counts up DINSYNC_PPM per beat, for dinsync out
volatile uint8_t dinsync_counter = 0;   
// clocked: keeps track of input dinsync pulses
volatile int16_t dinsync_clocked = 0;

// when doing midi sync to dinsync conversion, this is the timeout
// to dropping the clock after a MIDICLOCK message
volatile int16_t dinsync_clock_timeout = 0;

// these variables keep track of the dinsync pin states for dinsync in
uint8_t last_dinsync_start = 0;
uint8_t last_dinsync_stop = 0;
uint8_t last_dinsync_c = 0;

extern uint8_t sync;  // what sync mode are we in?

extern volatile uint8_t note_counter;
extern uint16_t timer3_init;

/* output functions (dinsync_start/stop) start and stop dinsync
   that is clocked from the internal tempo function */
void dinsync_start(void) {
  uint8_t flag = is_tempo_running();
  
  // make sure we're not in a "dinsync in" mode
  if (sync != DIN_SYNC) {
    //putstring("Starting DIN Sync\n\r");
    if (flag) 
      turn_off_tempo(); // if tempo was on, turn if off

    // set the clock low (rising edge is a clock)
    cbi(DINSYNC_PORT, DINSYNC_CLK);
    // send start signal
    DINSYNC_PORT |= _BV(DINSYNC_START);

    // wait for start signal to be noticed, then start the tempo up again.
    delay_ms(5);
    TCNT3 = timer3_init - 10;       // make it start soon
    dinsync_counter = 0;
    note_counter = 0;
    if (flag)
      turn_on_tempo();
  }
}

void dinsync_stop(void) {
  if (sync != DIN_SYNC) {  // make sure we're not input mode
    //putstring("Stopping DinSync\n\r");
    cbi(DINSYNC_PORT, DINSYNC_START);   // easy, just set Start low.
  }
}


/* input functions are for keeping track of whether an event occured
   on the dinsync port */

/* dinsync_started returns TRUE if the start pin is high and the previous
   call to this function was FALSE (ie. since the last function call, dinsync
   has started */
uint8_t dinsync_started(void) {
  uint8_t curr_dinsync_s;
  curr_dinsync_s = (DINSYNC_PIN >> DINSYNC_START) & 0x1;

  if (!last_dinsync_start && curr_dinsync_s) {
    last_dinsync_start = curr_dinsync_s;
    return TRUE;
  }

  last_dinsync_start = curr_dinsync_s;
  return FALSE;
}

/* dinsync_stopped returns TRUE if the start pin is low and the previous
   call to this function was FALSE (ie. since the last function call, dinsync
   has stopped */
uint8_t dinsync_stopped(void) {
  uint8_t curr_dinsync_s;
  curr_dinsync_s = (DINSYNC_PIN >> DINSYNC_START) & 0x1;

  if (last_dinsync_stop && !curr_dinsync_s) {
    last_dinsync_stop = curr_dinsync_s;
    return TRUE;
  }

  last_dinsync_stop = curr_dinsync_s;
  return FALSE;
}


/* these functions set the input/output descriptors */
void dinsync_set_out() {

  DINSYNC_DDR |= _BV(DINSYNC_START) | _BV(DINSYNC_CLK) |
    _BV(DINSYNC_4) | _BV(DINSYNC_5);

  DINSYNC_PORT &= ~( _BV(DINSYNC_START) | _BV(DINSYNC_CLK) |
		    _BV(DINSYNC_4) | _BV(DINSYNC_5) );

}

void dinsync_set_in() {

  DINSYNC_DDR &= ~( _BV(DINSYNC_START) | _BV(DINSYNC_CLK) |
		    _BV(DINSYNC_4) | _BV(DINSYNC_5) );

  DINSYNC_PORT &= ~( _BV(DINSYNC_START) | _BV(DINSYNC_CLK) |
		    _BV(DINSYNC_4) | _BV(DINSYNC_5) );

}
