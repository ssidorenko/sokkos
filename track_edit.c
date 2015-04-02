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
#include "track.h"
#include "pattern.h"
#include "switch.h"
#include "led.h"
#include "main.h"
#include "eeprom.h"
#include "synth.h"
#include "delay.h"
#include "midi.h"
#include "dinsync.h"


extern uint8_t function, bank;

uint8_t curr_track_index;/* removed volatile */
uint8_t play_loaded_track;
uint16_t curr_patt;
uint8_t track_location, track_bank;
uint16_t track_buff[TRACK_SIZE];/* removed volatile */

extern uint8_t all_accent, all_slide, all_rest; // all the time/* removed volatile */
extern int8_t curr_pitch_shift;

extern uint8_t play_loaded_pattern, curr_pattern_index;

uint8_t in_stepwrite_mode, in_run_mode;

extern uint8_t sync;

extern uint8_t curr_note;/* removed volatile */
extern volatile uint8_t note_counter;/* removed volatile */
extern volatile uint8_t dinsync_counter;

extern uint8_t PATT_LENGTH;

void do_track_edit(void) {
  uint8_t i;

  turn_on_tempo();

  // initialize
  track_bank = bank % 8;
  track_location = 0;

  in_stepwrite_mode = FALSE;
  in_run_mode = FALSE;
  play_loaded_track = FALSE;
  play_loaded_pattern = FALSE;
  curr_track_index = 0;
  curr_patt = END_OF_TRACK;
  sync = INTERNAL_SYNC;

  clear_bank_leds();
  clear_blinking_leds();

  while (1) {
    read_switches();

      // oops i guess they want something else, return!
    if (function != EDIT_TRACK_FUNC) {
      // stop playing any notes
      turn_off_tempo();
      play_loaded_track = play_loaded_pattern = 0;

      // turn off notes playing
      note_off(0);

      // clear the LEDs
      clear_all_leds();
      clock_leds();

      return;
    }
    
    // this is when they select which bank (1-8)& position(1-8)
    // for the track to edit
    if (!in_stepwrite_mode && !in_run_mode) {
      if (has_bank_knob_changed()) {
	//putstring("changed track bank\n\r");
	track_bank = bank % 8;
	load_track(track_bank, track_location);
	clear_bank_leds();  // track changed, clear the prev indicator
      }
      set_bank_led(track_bank);  // show the track being edited
      
      i = get_lowest_numkey_pressed(); 
      if (i != 0) {
	clear_notekey_leds();
	track_location = i-1;
	load_track(track_bank, track_location);
      }
      set_numkey_led(track_location+1);
    }

    // if they hit run/stop, play the track in 
    if (just_pressed(KEY_RS)) {
      if (in_run_mode) {
	stop_track_run_mode(); 	// stop run mode
      } else {
	// stop stepwrite mode
	if (in_stepwrite_mode) {
	  stop_track_stepwrite_mode();
	}
	start_track_run_mode();
      }
    }

    // do common stuff for both run and stepwritemode
    // ie. blink the bank led for the track index
    // show the current pattern's bank in the bank leds
    // show RAS leds, or DONE if end of track
    if (in_run_mode || in_stepwrite_mode) {
      // show the current position in the edited track
      if (!is_bank_led_blink(curr_track_index))
	clear_blinking_leds();
      set_bank_led_blink(curr_track_index); 

      if (curr_patt == END_OF_TRACK) {
	clear_note_leds();
	// the 'end of track' (0xFFFF) lights up DONE
	set_led(LED_DONE);
      } else {
	clear_led(LED_DONE);                 // make sure DONE isn't on anymore
	set_bank_led((curr_patt >> 3) & 0xF);    // show the bank of the current pattern
	
	// show RAS
	if (curr_patt & TRACK_REST_FLAG)
	  set_led(LED_REST);
	else
	  clear_led(LED_REST);

	if (curr_patt & TRACK_ACCENT_FLAG)
	  set_led(LED_ACCENT);
	else
	  clear_led(LED_ACCENT);

	if (curr_patt & TRACK_SLIDE_FLAG)
	  set_led(LED_SLIDE);
	else
	  clear_led(LED_SLIDE);
      }
    }
	  
    // handle both incrementing/decrementing the track index in stepwrite
    // as well as starting stepwrite
    if ((just_pressed(KEY_NEXT) || just_pressed(KEY_PREV)) && !in_run_mode) {
      note_off(0);  // if something -was- playing, kill it

      if (in_stepwrite_mode) {
	if (just_pressed(KEY_NEXT)) {
	  // step forward in the track
	  if (((curr_track_index+1) >= TRACK_SIZE) ||
	      (track_buff[curr_track_index] == END_OF_TRACK))
	    curr_track_index = 0;   // got to the end of the track, loop back to beginning
	  else
	    curr_track_index++;
	} else {  // just pressed key prev
	  // step backwards in the track
	  if (curr_track_index == 0) {
	    // search thru the buffer -forward- to find the EOT
	    while ((curr_track_index+1 < TRACK_SIZE) && 
		   (track_buff[curr_track_index] != END_OF_TRACK))
	      curr_track_index++;
	  } else {
	    curr_track_index--;
	  }
	}
      } else if (just_pressed(KEY_NEXT)) {
	// not in stepwrite mode, starting stepwrite mode
	start_track_stepwrite_mode();
	curr_track_index = 0;
      }
      
      // grab the current pattern in the track
      curr_patt = track_buff[curr_track_index];
      
      play_loaded_pattern = FALSE; 

      if (curr_patt != END_OF_TRACK) {
	/*
	  putstring("curr patt = loc "); putnum_ud(curr_patt & 0x7);
	  putstring(" in bank "); putnum_ud(curr_patt>>3 & 0xF);
	  putstring("\n\r");
	*/

	// get the pattern's RAS & pitch shift
	curr_pitch_shift = load_curr_patt();

	// start playing from beginning of pattern
	curr_pattern_index = 0;
	// wait for the next 'note on'
	while ((note_counter & 0x1) || dinsync_counter < DINSYNC_PPQ/4);

	// tell the tempo interrupt to start playing the pattern on loop
	play_loaded_pattern = TRUE;
      }
    }
  
    // cope with saving the current buffer and quitting step mode if in it
    if (just_pressed(KEY_DONE)) {
      if (in_stepwrite_mode) {
	if (curr_track_index+1 < TRACK_SIZE) {
	  // set an EOT marker if necc.
	  track_buff[curr_track_index+1] = END_OF_TRACK;
	}
	curr_patt = END_OF_TRACK;              // cleans up LEDs
	// stop the mode, and any playing
	stop_track_stepwrite_mode();           
	clear_led(LED_DONE);
      }
      // and write it to memory
      write_track(track_bank, track_location); 
    }
  
    // deal with specifics for stepwrite mode:
    // changing the track's patterns, RAS and pitchshift changes
    if (in_stepwrite_mode) {
      if (curr_patt != END_OF_TRACK) {
	// handle RAS keypresses -> modifications to current pattern
	
	if (just_pressed(KEY_REST)) {
	  curr_patt = (track_buff[curr_track_index] ^= TRACK_REST_FLAG);
	  all_rest = (curr_patt & TRACK_REST_FLAG) >> 8;
	}
	if (just_pressed(KEY_ACCENT)) {
	  curr_patt = (track_buff[curr_track_index] ^= TRACK_ACCENT_FLAG);
	  all_accent = (curr_patt & TRACK_ACCENT_FLAG) >> 8;
	}
	if (just_pressed(KEY_SLIDE)) {
	  curr_patt = (track_buff[curr_track_index] ^= TRACK_SLIDE_FLAG);
	  all_slide = (curr_patt & TRACK_SLIDE_FLAG) >> 8;
	}
	
	// handle U/D keypresses -> pitch shifting
	if (is_pressed(KEY_UP) || is_pressed(KEY_DOWN)) {
	  uint16_t notekey = get_lowest_notekey_pressed();
	  
	  if (just_pressed(KEY_UP) || just_pressed(KEY_DOWN)) {
	    clear_blinking_leds();
	    clear_notekey_leds();
	    clear_led(LED_UP);
	    clear_led(LED_DOWN);
	  }
	  
	  if (is_pressed(KEY_UP)) {
	    set_led(LED_UP);
	    
	    if (notekey != -1) {
	      clear_blinking_leds();
	      // set the new pitchshift
	      curr_patt = (curr_patt & 0xE0FF) | (notekey << 8);
	      track_buff[curr_track_index] = curr_patt; 
	    }
	    // display the pitchshift
	    curr_pitch_shift = get_pitchshift_from_patt(curr_patt);
	    set_notekey_led_blink(curr_pitch_shift);
	  } else if (is_pressed(KEY_DOWN)) {
	    set_led(LED_DOWN);
	    
	    if (notekey != -1) {
	      clear_blinking_leds();
	      // set the new pitchshift
	      curr_patt = (curr_patt & 0xE0FF) | (((notekey - OCTAVE) & 0x1F) << 8);
	      track_buff[curr_track_index] = curr_patt; 
	    }
	    
	    curr_pitch_shift = get_pitchshift_from_patt(curr_patt);
	    set_notekey_led_blink(OCTAVE + curr_pitch_shift);
	  }
	} else {
	  if (just_released(KEY_UP) || just_released(KEY_DOWN)) {
	    // clear up LEDs if not shifting
	    clear_led(LED_UP);
	    clear_led(LED_DOWN);
	    clear_blinking_leds();
	  }
	}
      }
      
      // see if they changed the bank / pressed numkey
      //    -> change current pattern
      if (! (is_pressed(KEY_UP) || is_pressed(KEY_DOWN))) {
	i = get_lowest_numkey_just_pressed();
	if ((i != 0) || has_bank_knob_changed()) {
	  clear_numkey_leds();
	  clear_bank_leds();
	  if (i == 0) {
	    if (curr_patt == END_OF_TRACK)
	      i = 0;
	    else
	      i = curr_patt & 0x7;
	  } else {
	    i--;
	  }
	  play_loaded_pattern = FALSE;
	  note_off(0);
	  curr_patt = (bank << 3) | i;
	  track_buff[curr_track_index] = curr_patt;
	  load_pattern(bank, i);
	  curr_pattern_index = 0;
	  all_rest = all_accent = all_slide = FALSE;
	  curr_pitch_shift = 0;
	  play_loaded_pattern = TRUE;
	}

	// while we're at it, show the pitch shift
	display_curr_pitch_shift_ud();
	  
	// show the pattern location and pattern bank
	set_single_numkey_led((curr_patt & 0x7) + 1);
      }
    }

    if (in_run_mode) {
	// while we're at it, show the pitch shift
	display_curr_pitch_shift_ud();
	  
	// show the pattern location and pattern bank
	set_single_numkey_led((curr_patt & 0x7) + 1);
    }
  }
}


int8_t get_pitchshift_from_patt(uint16_t patt) { 
 int8_t shift;

  shift = (patt >> 8) & 0x1F;
  if (shift & 0x10)
    shift |= 0xE0;      // extend signed 5-bit int

  return shift;

}
void display_curr_pitch_shift_ud(void) {
  if (curr_pitch_shift == 0) {
    clear_led(LED_UP);
    clear_led(LED_DOWN);
  } else if (curr_pitch_shift < 0) {
    clear_led(LED_UP);
    set_led(LED_DOWN);
  } else {
    set_led(LED_UP);
    clear_led(LED_DOWN);
  }
}

void start_track_stepwrite_mode(void) {
  in_stepwrite_mode = TRUE;
  set_led(LED_NEXT);
  clear_bank_leds();
}


void stop_track_stepwrite_mode(void) {
  in_stepwrite_mode = FALSE;
  clear_led(LED_NEXT);
  clear_all_leds();
  clear_blinking_leds();
  play_loaded_pattern = FALSE; 
  note_off(0);
}
 
void start_track_run_mode(void) {
  in_run_mode = TRUE;
  set_led(LED_RS);
  all_rest = all_slide = all_accent = FALSE;

  play_loaded_pattern = play_loaded_track = FALSE; 

  // theres a little bit of setup to get the track buffer playing
  curr_patt = track_buff[0];
  if (curr_patt != END_OF_TRACK) {     // clearly, dont play the track if there aint no data in it

    // begin at the first pattern, first note
    curr_track_index = 0;
    curr_pattern_index = 0;
    curr_note = REST; // make it a rest just to avoid sliding

    // load the first pattern
    curr_pitch_shift = load_curr_patt();
    
    // wait for the tempo interrupt to be ready for a note-on
    while ((note_counter & 0x1) || dinsync_counter < DINSYNC_PPQ/4);

    // get the tempo interrupt to start playing
    play_loaded_track = TRUE;
    //putstring("\n\raccent = ");putnum_ud(all_accent);
  }
  clear_bank_leds();
}

// a shortcut function...this code is duplicated a bunch of places.
// this just saves codespace
uint8_t load_curr_patt(void) {
  // load the pattern from EEPROM
  load_pattern((curr_patt >> 3) & 0xF, curr_patt & 0x7);
  
  // get the pattern's RAS & pitch shift
  all_rest = (curr_patt & TRACK_REST_FLAG) >> 8;
  all_accent = (curr_patt & TRACK_ACCENT_FLAG) >> 8;
  all_slide = (curr_patt & TRACK_SLIDE_FLAG) >> 8;
  curr_pattern_index = 0;

  return get_pitchshift_from_patt(curr_patt);
}

void stop_track_run_mode(void) {
  in_run_mode = FALSE;   
  play_loaded_pattern = FALSE;
  play_loaded_track = FALSE;
  note_off(0);
  clear_all_leds();
  clear_blinking_leds();
  has_bank_knob_changed();
}

void load_track(uint8_t bank, uint8_t track_loc) {
  uint8_t i;
  uint16_t track_addr;

  track_addr = TRACK_MEM + (bank*BANK_SIZE + track_loc*TRACK_SIZE)*2;

  /*
  putstring("reading track ["); 
  putnum_ud(bank); putstring(", "); putnum_ud(track_loc);
  putstring(" @ 0x");
  putnum_uh(track_addr);
  putstring("\n\r");
  */

  for (i=0; i < TRACK_SIZE; i++) {
    track_buff[i] = spieeprom_read(track_addr + 2*i) << 8;
    track_buff[i] |= spieeprom_read(track_addr + 2*i + 1);
  }
}

void write_track(uint8_t bank, uint8_t track_loc) {
  uint8_t i;
  uint16_t track_addr;

  track_addr = TRACK_MEM + (bank*BANK_SIZE + track_loc*TRACK_SIZE)*2;  

  /*
  putstring("write track ["); 
  putnum_ud(bank); putstring(", "); putnum_ud(track_loc);
  putstring(" @ 0x");
  putnum_uh(track_addr);
  putstring("\n\r");
  */

  for (i=0; i < TRACK_SIZE; i++) {
    spieeprom_write(track_buff[i]>>8, track_addr + 2*i);
    spieeprom_write(track_buff[i] & 0xFF, track_addr + 2*i + 1);
  }
}
