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
#include <stdio.h>
#include <avr/interrupt.h>
#include "pattern.h"
#include "track.h"
#include "switch.h"
#include "led.h"
#include "main.h"
#include "synth.h"
#include "delay.h"
#include "dinsync.h"
#include "midi.h"

extern uint8_t function, bank;

extern uint8_t sync;

extern volatile uint8_t note_counter;
extern volatile uint8_t dinsync_counter;
extern volatile int16_t dinsync_clocked, midisync_clocked;

// pattern running info
extern uint8_t curr_pattern_index;/* removed volatile */
extern uint8_t pattern_buff[PATT_SIZE];    // the 'loaded' pattern buffer/* removed volatile */
extern uint16_t curr_patt;
extern uint8_t patt_location;

// track runnning info
extern uint8_t curr_track_index;/* removed volatile */
extern uint16_t track_buff[TRACK_SIZE]; // the 'loaded' track buffer/* removed volatile */

// a chain can either hold patterns or tracks (depending on the mode
uint8_t curr_chain[MAX_CHAIN];/* removed volatile */
uint8_t curr_chain_index;/* removed volatile */
uint8_t next_chain[MAX_CHAIN];/* removed volatile */
uint8_t buff_chain[MAX_CHAIN];
uint8_t buff_chain_len = 0;

// the currently-playing pitch shift and the upcoming pitch shift
extern int8_t curr_pitch_shift;
extern int8_t next_pitch_shift;

extern uint8_t curr_note;
extern uint8_t swing_it;
extern uint8_t running_stepwrite;

uint8_t PATT_LENGTH = PATT_SIZE;

uint8_t all_accent = 0;
uint8_t all_slide = 0;
uint8_t all_rest = 0; // all the time

uint8_t curr_bank = 0;
uint8_t next_bank = 0;

uint8_t playing;
uint8_t swing_time = 0;
uint8_t new_swing_time = 0;
uint8_t live_edit = FALSE;
uint8_t loop = FALSE;
uint8_t loop_countdown = FALSE;
uint8_t loop_start = 1;
uint8_t loop_end = PATT_SIZE;
int8_t countdown = 0;
uint8_t prev_pattern_index;
/* 8th note stuff */
uint8_t eighths = FALSE;
/* */

extern uint8_t play_loaded_pattern; // are we playing?

volatile uint16_t tap_tempo_timer = 0;

extern uint8_t midi_cmd;
extern uint8_t midi_in_addr;

extern uint8_t settings;

// could be MIDISYNC, DINSYNC or SYNCOUT
#define function_changed (function != curr_function)

// both pattern and track play are similar enough in function
// (and codespace is small enough) that they're in the same
// function. eek
void do_patterntrack_play(void) {
  uint8_t i = 0, curr_function;
  uint8_t start_point = 0;
  uint8_t end_point = 0;
  uint8_t midi_data;
  uint8_t no_loop = FALSE;

	  //080602
  uint8_t studge = FALSE;//stutter or nudge

  curr_function = function;

  if (sync == INTERNAL_SYNC) {
    turn_on_tempo();
  } else {
    turn_off_tempo();
  } 
  if (sync == DIN_SYNC) {
    dinsync_set_in();
  } else {
    dinsync_set_out();
  }

  clear_all_leds();
  clear_blinking_leds();
  next_chain[0] = curr_chain[0] = 0;
  next_chain[1] = curr_chain[1] = 0xFF;
  set_numkey_led(1);
 
  playing = FALSE;

  curr_track_index = 0;
  curr_pattern_index = 0;

  curr_patt = 0;

  curr_chain_index = 0;

  curr_pitch_shift = next_pitch_shift = 0;
  
  swing_time = 0;
  
  live_edit = FALSE;
  loop = FALSE;
  loop_countdown = FALSE;
  loop_start = 1;
  loop_end = PATT_LENGTH;
  countdown = 0;

  clear_bank_leds();
  if (ANYPATTERNPLAYFUNC)
    next_bank = curr_bank = bank;
  else  // TRACKPLAY
    next_bank = curr_bank = bank % 8;

  set_bank_led(bank);

  while (1) {
    read_switches();

//    if (function_changed && is_pressed(KEY_RS)) {
    if (function_changed && function == EDIT_PATTERN_FUNC) {
	  live_edit = TRUE;
	  set_led_blink(LED_RS);
	}
	
    if (function_changed && !playing) {
      playing = FALSE;
	  live_edit = FALSE;
      dinsync_stop();
      midi_stop();
      curr_pitch_shift = next_pitch_shift = 0;
      all_accent = all_rest = all_slide = swing_it = 0;

      clear_all_leds();
      clear_blinking_leds();
      clock_leds();
      return;
    }

	if (live_edit && function == EDIT_PATTERN_FUNC) {
	  patt_location = buff_chain[curr_patt];
	  edit_live();
        if ((settings & 1<<2)) buff_chain[curr_patt] = next_chain[curr_patt] = patt_location;
	}
	else {

    // detect 'tap tempo' requests by timing between KEY_TEMPO strikes
    if (just_pressed(KEY_TEMPO) && !is_pressed(KEY_DONE)) {
      if ((tap_tempo_timer < 3334) //  more than 3s between taps = 20BPM
	  && (tap_tempo_timer > 333)) // less than .3ms between taps = 200BPM
	{
	  tap_tempo_timer = 60000UL/tap_tempo_timer; // convert to BPM
	  change_tempo(tap_tempo_timer);
	}
      tap_tempo_timer = 0;
    }

    // start a new chain if just pressed
    if (just_pressed(KEY_CHAIN)) {
      clear_notekey_leds();
      clear_blinking_leds();
      set_led(LED_CHAIN);
      buff_chain_len = 0;  // 'start' to write a new chain
    }

    // releasing the chain key 'finalizes' the chain buffer
    if (just_released(KEY_CHAIN)) {
      /*
	putstring("buff'd chain = ");
	for (i=0; i<MAX_CHAIN; i++) {
	  if (buff_chain[i] >= 8)
	    break;
	  putnum_ud(buff_chain[i]);
	  uart_putchar(' ');
	}
	putstring("\n\r");
      */
      for (i=0; i<MAX_CHAIN; i++) {
	next_chain[i] = buff_chain[i];
      }
      // if we're not playing something right now, curr = next
      if (!playing) {
	for (i=0; i<MAX_CHAIN; i++)
	  curr_chain[i] = next_chain[i];
	curr_pitch_shift = next_pitch_shift;
	clear_led(LED_UP);
	clear_led(LED_DOWN);
      }
      clear_led(LED_CHAIN);
    }

    if (is_pressed(KEY_CHAIN)) {
      // display the current chain
      for (i=0; i<buff_chain_len; i++) {
	if (buff_chain[i] >= 8)
	  break;
	set_numkey_led(buff_chain[i]+1);
      }

      // ok lets add patterns/tracks to the buffer chain!
      i = get_lowest_numkey_just_pressed();
      if ((i != 0) && (buff_chain_len < MAX_CHAIN)) {
	buff_chain[buff_chain_len++] = i - 1;
	buff_chain[buff_chain_len] = 0xFF;
		
	/*
	  putstring("adding: ");
	  putnum_uh(buff_chain[buff_chain_len-1]);
	  putstring("\n\r");
	*/
	/*
	  putstring("buff'd chain = ");
	  for (i=0; i<MAX_CHAIN; i++) {
	  if (buff_chain[i] >= 8)
	  break;
	  putnum_ud(buff_chain[i]);
	  uart_putchar(' ');
	  }
	  putstring("\n\r");
	*/

      }
    }
    // if they press U or D, show the current pitch shift and allow pitch shift adjust
    else if (is_pressed(KEY_UP) || is_pressed(KEY_DOWN)) {
      int8_t notekey = get_lowest_notekey_pressed();

      // clear any pattern indicator leds
      if (just_pressed(KEY_UP) || just_pressed(KEY_DOWN)) {
	clear_notekey_leds();
	clear_blinking_leds();
	clear_led(LED_CHAIN);
      }

      // check if they are changing the shift
      if (is_pressed(KEY_UP)) {
	clear_led(LED_DOWN);
	set_led(LED_UP);

	if (notekey != -1) 
	  next_pitch_shift = notekey; 
	if (curr_pitch_shift >= 0) {
	  if (! is_notekey_led_blink(curr_pitch_shift)) {
	    clear_blinking_leds();
	    set_notekey_led_blink(curr_pitch_shift);
	  }
	}
	if (next_pitch_shift != curr_pitch_shift)
	  set_notekey_led(next_pitch_shift);
      } else if (is_pressed(KEY_DOWN)) {
	clear_led(LED_UP);
	set_led(LED_DOWN);

	if (notekey != -1)
	  next_pitch_shift = notekey - OCTAVE;  // invert direction 

	if (curr_pitch_shift <= 0) {
	  if (!is_notekey_led_blink(OCTAVE + curr_pitch_shift)) {
	    clear_blinking_leds();
	    set_notekey_led_blink(OCTAVE + curr_pitch_shift);
	  }
	}
	if (next_pitch_shift != curr_pitch_shift)
	  set_notekey_led(OCTAVE + next_pitch_shift);
      }

      // if not playing something right now,
      // make the pitch shift effective immediately
      if (!playing)
	curr_pitch_shift = next_pitch_shift;      

    } else {
      if (just_released(KEY_UP) || just_released(KEY_DOWN)) {
	// clear any pitch shift indicators
	clear_notekey_leds();
	clear_blinking_leds();
      }

      // if they just pressed a numkey, make a chain thats
      // one pattern long
      i = get_lowest_numkey_pressed();
      if (!is_pressed(KEY_DONE) && ((i != 0) || has_bank_knob_changed())) {
	if (i != 0) {
	  clear_numkey_leds();
	  buff_chain[0] = next_chain[0] = i - 1;
	  buff_chain[1] = next_chain[1] = 0xFF;
	  
	  if (!playing)
	    for (i=0; i<MAX_CHAIN; i++) 
	      curr_chain[i] = next_chain[i];
	} else {
	  if (ANYPATTERNPLAYFUNC)
	    next_bank = bank;
	  else
	    next_bank = bank%8;

	  if (!playing)
	    curr_bank = next_bank;
	}
	if (!playing) {
	  clear_bank_leds();
	  set_bank_led(next_bank);
	  curr_pitch_shift = next_pitch_shift;
	}
      }
	  
	if (sync != MIDI_SYNC) 	midi_cmd = midi_recv_cmd();
	
	if (midi_cmd >> 4 == 0xC) {
	  midi_data = midi_getchar();
      if (!(midi_data & 0x80)) {
	    next_bank = midi_data/8;
		next_chain[0] = midi_data%8;
		next_chain[1] = 0xFF;
		if (!playing) {
		  curr_bank=next_bank;
		  curr_chain[0]=midi_data%8;
		  curr_chain[1]=0xFF;
		}
		clear_numkey_leds();
		set_numkey_led(next_chain[0]+1);
      }
	} else // 110109
    if ((midi_cmd >> 4 == 0x9) && ((midi_cmd & 0xF) == midi_in_addr)) { //MIDI_NOTE_ON
        int8_t midi_ps = midi_getchar() - 0x3C;
        if (midi_ps < 13 && midi_ps > -13) curr_pitch_shift = next_pitch_shift = midi_ps;
    }
        
      // indicate current pattern & next pattern & shift 
      if (!chains_equiv(next_chain, curr_chain)) {
	if (next_chain[1] == END_OF_CHAIN && curr_chain[1] == END_OF_CHAIN) {
	  // basically single patterns. current blinks
	  set_numkey_led_blink(curr_chain[0]+1);
	}

	// otherwise, always just show the next chain in all solid lights
	for (i=0; i<MAX_CHAIN; i++) {
	  if (next_chain[i] > 8)
	    break;
	  set_numkey_led(next_chain[i] + 1);
	}
      } else {
	for (i=0; i<MAX_CHAIN; i++) {
	  if (curr_chain[i] > 8)
	    break;
	  if (playing && (curr_chain[i] == curr_chain[curr_chain_index])) {
	    if (! is_numkey_led_blink(curr_chain[i]+1) ) 
	      {
		// if playing, current pattern/track blinks
		clear_numkey_led(curr_chain[i]+1);
		set_numkey_led_blink(curr_chain[i]+1); 
	      }
	  } else {
	    // clear old blinking tracks/patterns
	    if (is_numkey_led_blink(curr_chain[i]+1))
	      clear_blinking_leds();
	    // all other patterns in chain solid
	    set_numkey_led(curr_chain[i] + 1); 
	  }
	}
      }
      display_curr_pitch_shift_ud();
    }
 
	clock_ticks();
 
    if ( ((sync == INTERNAL_SYNC) && just_released(KEY_RS) && 
	       playing) ||
//	       playing && !live_edit) ||
	 ((sync == MIDI_SYNC) && (midi_cmd == MIDI_STOP)) ||
	 ((sync == DIN_SYNC) && dinsync_stopped()) ) 
      {
	//putstring("stop\n\r");
	playing = FALSE;
	play_loaded_pattern = FALSE;
	loop = FALSE;
    loop_countdown = FALSE;
    loop_start = 1;
    loop_end = PATT_LENGTH;
    countdown = 0;
	note_off(0);
	midi_stop();
	if (sync != DIN_SYNC) 
	  dinsync_stop();

	clear_led(LED_RS);
	clear_blinking_leds();
	clear_bank_leds();
	if (ANYPATTERNPLAYFUNC)	
	  set_bank_led(bank);
	else
	  set_bank_led(bank % 8);
	if (function_changed) {
	  live_edit = FALSE;
      curr_pitch_shift = next_pitch_shift = 0;
      all_accent = all_rest = all_slide = swing_it = 0;
      clear_all_leds();
      clock_leds();
	  return;
	}
      }
    else if ( ((sync == INTERNAL_SYNC) && just_released(KEY_RS) && !playing) ||
	      ((sync == MIDI_SYNC) && 
	       ((midi_cmd == MIDI_START) || (midi_cmd == MIDI_CONTINUE))) ||
	      ((sync == DIN_SYNC) && dinsync_started()) )
      {
	set_led(LED_RS);
	//putstring("start\n\r");

	if (ANYPATTERNPLAYFUNC) {
	  if (has_bank_knob_changed()) {
    	  load_pattern(bank, curr_chain[0]);
		  }
	  else {
	      load_pattern(curr_bank, curr_chain[0]);
	      }
	} else {
	  load_track(bank%8, curr_chain[0]);
	  curr_patt = track_buff[0];
	  load_curr_patt(); // ignore pitch shift returned
	}
	curr_note = REST;
	/*
	  putstring("next pattern (bank ");
	  putnum_ud(bank);
	  putstring(", loc ");
	  putnum_ud(curr_pattern_location);
	  putstring("\n\r");
	*/

	// on midisync continue message, continue!
	if (! ((sync == MIDI_SYNC) && (midi_cmd == MIDI_CONTINUE))) {
	  curr_chain_index = 0;  // index into current chain
	  curr_pattern_index = 0;        // index into current pattern in chain
	  curr_track_index = 0;        // index into current pattern in chain
	}
	
	note_counter = 0;
	midisync_clocked = 0;
	dinsync_counter = 0;
	dinsync_clocked = 0;
	swing_it = 0;
	playing = TRUE;
	play_loaded_pattern = TRUE;
	midi_putchar(MIDI_START);
	if (sync != DIN_SYNC)
	  dinsync_start();
      } 

///	if (just_released(KEY_RS) && live_edit && 
	if (live_edit && 
	    function != EDIT_PATTERN_FUNC) {
		  live_edit = FALSE;
		  running_stepwrite = FALSE;
		  clear_all_leds();
		  set_led(LED_RS);
	}
	
	if (is_pressed(KEY_CHAIN)) {
	  if (just_pressed(KEY_PREV)) {
		curr_pattern_index = loop_start-1;
	  }	
	}
	
	if (is_pressed(KEY_DONE)) {
	
	/* 8th note stuff */
	if (just_pressed(KEY_TEMPO)) {
	  eighths = !eighths;
	  no_loop = TRUE;
	  }
	/* */
	
	  i = get_lowest_loopkey_just_pressed();
	  if (start_point == 0) {
	    start_point = i;
	  }
	  else if (end_point == 0){
	    end_point = i;
	  }
	}
	else {
	  if (just_pressed(KEY_SLIDE) || just_released(KEY_SLIDE)) {
	    all_slide = !all_slide;
	    if (is_pressed(KEY_SLIDE)) set_led(LED_SLIDE);
	    else clear_led(LED_SLIDE);
	  }
	  if (just_pressed(KEY_ACCENT) || just_released(KEY_ACCENT)) {
	    all_accent = !all_accent;
	    if (is_pressed(KEY_ACCENT)) set_led(LED_ACCENT);
	    else clear_led(LED_ACCENT);
	  }
	  if (just_pressed(KEY_REST) || just_released(KEY_REST)) {
	    all_rest = !all_rest;
	    if (is_pressed(KEY_REST)) set_led(LED_REST);
	    else clear_led(LED_REST);
	  }
	}
	
	if (just_pressed(KEY_NEXT) && is_pressed(KEY_PREV)) {
	  //080602
	  studge = TRUE;

	  curr_pattern_index = get_next_patt_idx();
	}
	else if (just_pressed(KEY_PREV) && is_pressed(KEY_NEXT)) {
	  //080602
	  studge = TRUE;

      curr_pattern_index = prev_pattern_index;
	  if (loop_countdown) 
	    if (countdown < 0) 
		  countdown--;
		else 
		  countdown++;
	}
	else if (just_pressed(KEY_NEXT)) {
//080602
	  if (studge) studge = FALSE;
	  else
	  if (++new_swing_time > 5) new_swing_time = 5;
	}
	else if (just_pressed(KEY_PREV)) {
//080602
	  if (studge) studge = FALSE;
	  else
	  if (new_swing_time-- == 0) new_swing_time = 0;
	}

	if (just_released(KEY_DONE)) {
	  if (!no_loop)
	  {
	  loop_countdown = TRUE;
	  countdown = loop_end - curr_pattern_index - 1;
	  if (start_point == 0 && end_point == 0) {
		loop_start = 1;
		loop_end = PATT_LENGTH;
	    loop = FALSE;		
	  }
	  else {
		if (end_point != 0 && pattern_buff[start_point-1] != 0xFF &&
		    pattern_buff[end_point-1] != 0xFF) {
     	  loop = TRUE;
	      loop_start = start_point;
	      loop_end = end_point;
        } 	
	    start_point = end_point = 0;
	  }
	  } else no_loop = FALSE;
	}

    }

  }
}

uint8_t chains_equiv(volatile uint8_t *chain1, volatile uint8_t *chain2) {
  uint8_t i;
  
  for (i=0; i < MAX_CHAIN; i++) {
    if (chain1[i] != chain2[i])
      return FALSE;
    if (chain1[i] == 0xFF) 
      return TRUE;
  }
  return TRUE;
}

uint8_t get_next_patt_idx() {
  prev_pattern_index = curr_pattern_index;
  if (loop && !loop_countdown) {
	if (curr_pattern_index == loop_end-1) return loop_start-1;
	if (loop_start > loop_end) return curr_pattern_index-1;
  }
  else if (loop_countdown){
    if (countdown < 0) {
	  countdown++;
	  return curr_pattern_index-1;
	}
	else if (countdown > 0) countdown--;
	else {
	  loop_countdown = FALSE;
	  if (loop) return loop_start-1;
	  else return loop_end;
	}
  }
  return curr_pattern_index+1;
}
