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
#include "switch.h"
#include "led.h"
#include "main.h"
#include "eeprom.h"
#include "synth.h"
#include "delay.h"
#include "dinsync.h"
#include "midi.h"

// these are set by read_switches()
extern uint8_t function, bank;

// the midi clock counter
extern volatile int16_t midisync_clocked, dinsync_clocked;
extern uint8_t playing;/* removed volatile */

// The tempo note counter. used to start runwrite on a note-on interrupt call
volatile uint8_t note_counter;
extern volatile uint8_t dinsync_counter;

uint8_t curr_pattern_index;/* removed volatile */
extern uint8_t running_stepwrite;
uint8_t play_loaded_pattern;                 // are we playing?
uint8_t patt_location;
uint8_t patt_bank;
uint8_t pattern_buff[PATT_SIZE];    // the 'loaded' pattern buffer/* removed volatile */

uint8_t in_runwrite_mode, in_stepwrite_mode;
uint8_t runstepwrite_pattidx;
uint8_t dirtyflag = 0; // you filthy pattern, have you been modified?
//080602
uint8_t midi_cmd = 0;

extern uint8_t PATT_LENGTH;

extern uint8_t curr_note, sync, live_edit, settings;

/***********************/
void do_pattern_edit(void) {
  uint8_t i, curr_function;

  curr_function = function;

  // initialize
  patt_location = 0;
  in_stepwrite_mode = 0;
  in_runwrite_mode = 0;
  running_stepwrite = FALSE;
  play_loaded_pattern = 0;
  curr_pattern_index = 0;
  curr_note = 0;
  
  if (sync == INTERNAL_SYNC)
    turn_on_tempo();
  else {
    turn_off_tempo();
    dinsync_set_out();
  }

  read_switches();
  patt_bank = bank;

  load_pattern(patt_bank, patt_location);
  clear_all_leds();

  set_bank_led(bank);
      
  while (1) {
    read_switches();

    if (function != curr_function) {
      // oops i guess they want something else, return!
      turn_off_tempo();
      play_loaded_pattern = FALSE;
      
      // turn off all sound & output signals
      note_off(0);
      dinsync_stop();
      midi_stop();

      // clear the LEDs
      clear_all_leds();
      clock_leds();
      return;
    }

    if (patt_bank != bank) {
      //putstring("changed bank!");
//      if (in_runwrite_mode)
//	stop_runwrite_mode();
      if (in_stepwrite_mode)
	stop_stepwrite_mode();
      patt_bank = bank;
      load_pattern(patt_bank, patt_location);
      clear_bank_leds();
      set_bank_led(bank);
    }

    // if they pressed one of the 8 bottom buttons (location select)
    if (! (in_runwrite_mode || in_stepwrite_mode)) {
      // display whatever location is selected on number keys
      set_numkey_led(patt_location+1);

      i = get_lowest_numkey_pressed();
      if (i != 0) {
	clear_notekey_leds();
	set_numkey_led(i);
	patt_location = i - 1;
	load_pattern(patt_bank, patt_location);
	//printf("location #%d\n\r", patt_location);
      }
    }

    edit_pattern();


  } // while loop
}

void load_pattern(uint8_t bank, uint8_t patt_location) {
  uint16_t pattern_addr;
  uint8_t i;

  clock_leds();

  pattern_addr = PATTERN_MEM + bank*BANK_SIZE + patt_location*PATT_SIZE;

  /*
  putstring("load patt ["); 
  putnum_ud(bank); putstring(", "); putnum_ud(patt_location);
  putstring(" @ 0x");
  putnum_uh(pattern_addr);
  putstring("\n\r");
  */

  for(i=0; i<PATT_SIZE; i++) {
    pattern_buff[i] = spieeprom_read(pattern_addr + i);
    //putstring(" 0x"); putnum_uh(pattern_buff[i]);
  }
  //putstring("\n\r");

    if (pattern_buff[PATT_SIZE-1] == 0x01) {
        PATT_LENGTH = 15;
    } else {
        PATT_LENGTH = 16;
    }

    
  dirtyflag = 0;
  clear_led_blink(LED_DONE);
  clock_leds();

}

void write_pattern(uint8_t bank, uint8_t patt_location) {
  uint16_t pattern_addr;
  uint8_t i;

  clock_leds();

  pattern_addr = PATTERN_MEM + bank*BANK_SIZE + patt_location*PATT_SIZE;

  /*
  putstring("writing pattern. bank ");
  putnum_ud(bank);
  putstring(", loc ");
  putnum_ud(patt_location);
  putstring(" from 0x");
  putnum_uh(pattern_addr);
  putstring("\n\rold memory: \n\r");
  for (i=0; i<PATT_SIZE; i++) {
    putstring(" 0x"); putnum_uh(spieeprom_read(pattern_addr+i));
  }
  putstring("\n\r");
  */

  // modify the buffer with new data
  for (i = 0; i<PATT_SIZE; i++) {
   spieeprom_write(pattern_buff[i], pattern_addr+i);
   clock_ticks();
  }
  /*
  putstring("\n\rnew memory: \n\r");
  for (i=0; i<PATT_SIZE; i++) {
    putstring(" 0x"); putnum_uh(spieeprom_read(pattern_addr+i));
  }
  putstring("\n\r");
  */

  clock_leds();

}

void clock_ticks()
{
	if (playing) {
		if ((sync == MIDI_SYNC) && (midisync_clocked > 0)) {
		    midisync_clocked = 0;
			do_tempo();
			do_tempo();
		}
		else if ((sync == DIN_SYNC) && (dinsync_clocked > 0)) {
		    dinsync_clocked = 0;
		    do_tempo();
			do_tempo();
		}
	}
	if (sync == MIDI_SYNC) {
      midi_cmd = midi_recv_cmd(); // returns 0 if no midi commands waiting
      if (midi_cmd != 0 && function == EDIT_PATTERN_FUNC)
	    putnum_uh(midi_cmd);
    }
}

void edit_pattern() {
    uint8_t i;
	
	uint8_t note_entered = FALSE;

	if (just_pressed(KEY_TEMPO) && !is_pressed(KEY_CHAIN)) {
        if (PATT_LENGTH == 15) {
            PATT_LENGTH = 16;
            pattern_buff[PATT_LENGTH-1] = 0xFF;
        } else {
            PATT_LENGTH = 15;
            pattern_buff[PATT_LENGTH] = 0x01;            
        }
        dirtyflag = 1;
    } else

    // erase pattern on KEY_CHAIN + KEY_TEMPO
    if (is_pressed(KEY_CHAIN)) {
//      set_led(LED_CHAIN);
	  
	  if (just_pressed(KEY_TEMPO)) {
      for (i=0; i< PATT_SIZE; i++) {
	
	    pattern_buff[i] = 0xFF;
      }
	    dirtyflag = 1; // clearly, changed	  
	  }
	  
    } else if (just_released(KEY_CHAIN) && in_runwrite_mode && PATT_LENGTH==16) {
	  /* Re-shuffle it! */
      uint8_t pattern_copy[5];
	  
	  memcpy(pattern_copy, pattern_buff+11, 5);
	  memcpy(pattern_buff+11, pattern_buff+4, 2);
	  pattern_buff[13] = pattern_buff[0];
	  memcpy(pattern_buff+14, pattern_buff+9, 2);
	  pattern_buff[0] = pattern_buff[6];
	  memcpy(pattern_buff+9, pattern_buff+2, 2);
	  pattern_buff[2] = pattern_buff[8];
	  pattern_buff[8] = pattern_buff[1];
	  pattern_buff[1] = pattern_buff[7];
	  memcpy(pattern_buff+3, pattern_copy, 5);

      dirtyflag = 1; // clearly, changed
	
//      clear_led(LED_CHAIN);
    }

    // midi sync clock ticks	
    clock_ticks();

    // if they hit run/stop, or on midi stop/start,
    // play the pattern in scratch! (until they hit rs again)
    if (in_runwrite_mode && 
	(((sync == INTERNAL_SYNC) && just_pressed(KEY_RS)) ||
	 ((sync == MIDI_SYNC) && (midi_cmd == MIDI_STOP)) ||
	 ((sync == DIN_SYNC) && dinsync_stopped()))) {
      stop_runwrite_mode();
	  running_stepwrite = FALSE;
    } else if (((sync == INTERNAL_SYNC) && just_pressed(KEY_RS)) ||
	       ((sync == MIDI_SYNC) && 
		((midi_cmd == MIDI_START) || (midi_cmd == MIDI_CONTINUE)) ) ) {
      if (in_stepwrite_mode)
	stop_stepwrite_mode();
      start_runwrite_mode();
    }

    if (in_runwrite_mode || in_stepwrite_mode) {
      uint8_t curr_note, index;
      int8_t shift;

	  if (running_stepwrite) index = runstepwrite_pattidx;
      else index = curr_pattern_index;
      curr_note = pattern_buff[index] & 0x3F;

	  if ((curr_note != 0x3F)) {
      // dont accent or slide 'done' notes, duh!
      if (just_pressed(KEY_ACCENT)) {
	//putstring("accent ");
	pattern_buff[index] ^= 1 << 6;
	dirtyflag = 1; // clearly, changed
      }
      if (just_pressed(KEY_SLIDE)) {
	//putstring("slide ");
      	pattern_buff[index] ^= 1 << 7;
	dirtyflag = 1; // clearly, changed
      }
	  }

      // rests/dones are middle octave (if you hit a note key next)
      if ((curr_note == 0x3F) || (curr_note == 0)) 
	shift = 0;
      else if (curr_note < C2)
	shift = -1;
      else if (curr_note <= C3)
	shift = 0;
      else if (curr_note <= C4)
	shift = 1;
      else if (curr_note <= C5)
	shift = 2;
      else 
	shift = 3;

      if (just_pressed(KEY_REST)) {
	curr_note = 0;
      }

	  if ((curr_note != 0x3F) && (curr_note != 0)) {
	    if (!is_pressed(KEY_PREV) && just_pressed(KEY_UP) && shift < (2 - (settings & 1))) curr_note += OCTAVE;
		if (!is_pressed(KEY_PREV) && just_pressed(KEY_DOWN) && shift > -1) curr_note -= OCTAVE;
	  }


    if (get_lowest_notekey_just_pressed() != 0) curr_note = shift*OCTAVE;
	
      if (just_pressed(KEY_C))
	curr_note += C2;
      if (just_pressed(KEY_CS))
	curr_note += C2_SHARP; 
      if (just_pressed(KEY_D))
	curr_note += D2;
      if (just_pressed(KEY_DS))
	curr_note += D2_SHARP;

      // note shouldnt go 'above' 0x3E
      if (shift < 3) {
	if (just_pressed(KEY_E))
	  curr_note += E2;
	if (just_pressed(KEY_F))
	  curr_note += F2;
	if (just_pressed(KEY_FS))
	  curr_note += F2_SHARP;
	if (just_pressed(KEY_G))
	  curr_note += G2;
	if (just_pressed(KEY_GS))
	  curr_note += G2_SHARP;
	if (just_pressed(KEY_A))
	  curr_note += A3;
	if (just_pressed(KEY_AS))
	  curr_note += A3_SHARP;
	if (just_pressed(KEY_B))
	  curr_note += B3;
	if (just_pressed(KEY_C2)) {
	  curr_note += C3;
	}
      }
	
      // when changing to a note from end of pattern (0xff), toss top 2 bits
      if ((pattern_buff[index] != 0xFF) || (curr_note == 0x3F))
	curr_note |= (pattern_buff[index] & 0xC0);   

      if (curr_note != pattern_buff[index]) {
	    	   
	if (in_stepwrite_mode) {
	  note_off(0);
	  delay_ms(1);
	}
	// if the note changed!
	pattern_buff[index] = curr_note;
	dirtyflag = 1; // clearly, changed

	if (in_stepwrite_mode) 	 // restrike note
	  note_on(curr_note & 0x3F,
		  0,              // no slide
		  (curr_note>>6) & 0x1);       // accent
      }

      if (curr_note != 0xFF) {
	set_note_led(curr_note);
	if (dirtyflag)
	  set_led_blink(LED_DONE);
	else
	  clear_led(LED_DONE);
      } else {
	clear_note_leds();
	clear_led_blink(LED_DONE);
	clear_led_blink(LED_UP);
	set_led(LED_DONE);
      }
    }

/* Backwards rotating... */
	if (just_pressed(KEY_PREV) && is_pressed(KEY_NEXT)) {
	  uint8_t i = 0;
	  uint8_t first = pattern_buff[0];
	  while (i < PATT_LENGTH-1 && pattern_buff[i+1] != 0xFF) {
	    pattern_buff[i] = pattern_buff[i+1];
	    i++;
	  }
	  pattern_buff[i] = first;
	  dirtyflag = 1;
	}
	else 
	
	if (just_pressed(KEY_NEXT) && is_pressed(KEY_PREV)) {
	  uint8_t i  = 0;
	  uint8_t next;
	  uint8_t prev = pattern_buff[0];
	  while (i < PATT_LENGTH-1 && pattern_buff[i+1] != 0xFF) {
		next = pattern_buff[i+1];
	    pattern_buff[++i] = prev;
		prev=next;
	  }
	  pattern_buff[0] = next;
	  dirtyflag = 1;
	}
    
	// transpose whole pattern by semitones up/down 110109
    else 
    if (just_pressed(KEY_UP) && is_pressed(KEY_PREV)) {
        uint8_t tu = 0;
        while(tu < PATT_LENGTH && pattern_buff[tu] != 0xFF) {
            if (pattern_buff[tu] != 0x3E) pattern_buff[tu]++;
            tu++;
        }
        dirtyflag = 1;
    }
    else 
    if (just_pressed(KEY_DOWN) && is_pressed(KEY_PREV)) {
        uint8_t td = 0;
        while(td < PATT_LENGTH && pattern_buff[td] != 0xFF) {
            if (pattern_buff[td] != 0x0B) pattern_buff[td]--;
            td++;
        }
        dirtyflag = 1;
    }    
    
    
    // if in step mode & they press 'next' or 'prev, then step fwd/back, otherwise start stepmode
    else 
	if (just_pressed(KEY_NEXT) || just_pressed(KEY_PREV) || note_entered) {
      uint8_t prev_note = curr_note;

      if (in_stepwrite_mode) {
	// turn off the last note
	note_off((curr_note >> 7) & 0x1);
	delay_ms(1);
	//putstring("step");


	if (just_pressed(KEY_PREV)) { 
	  // get previous note from pattern
	  if (curr_pattern_index == 0) {
	    // search thru the buffer -forward- to find the EOP byte
	    while ((curr_pattern_index < PATT_LENGTH-1) && 
		   (pattern_buff[curr_pattern_index] != END_OF_PATTERN))
	      curr_pattern_index++;
	  } else {
	    curr_pattern_index--;
	  }
	} else {
	  // get next note from pattern
	  if (((curr_pattern_index+1) >= PATT_LENGTH) ||
	      (pattern_buff[curr_pattern_index] == END_OF_PATTERN))
	    curr_pattern_index = 0;
	  else
	    curr_pattern_index++;
	}


	clear_bank_leds();
	set_bank_led(curr_pattern_index);


	//putstring("i = "); putnum_ud(curr_pattern_index); putstring("\n\r");
	curr_note = pattern_buff[curr_pattern_index];

	if (curr_note == 0xFF) {
	  clear_led(LED_ACCENT);
	  clear_led(LED_SLIDE);
	  set_led(LED_DONE);
	  clear_led_blink(LED_DONE);
	} else {
	  clear_led(LED_DONE);
	  if (dirtyflag)
	    set_led_blink(LED_DONE);

	  note_on(curr_note & 0x3F,
		  prev_note >> 7,              // slide
		  (curr_note>>6) & 0x1);       // accent
	  
	  set_note_led(curr_note);
	}
      } else {
	    if (just_pressed(KEY_NEXT) || note_entered) {
	      if (!in_runwrite_mode) {
		    start_stepwrite_mode();

		    curr_pattern_index = 0;

		    curr_note = pattern_buff[curr_pattern_index];
		    if (curr_note != 0xFF) {
			  note_on(curr_note & 0x3F,
					  0,              // no slide on first note
					  (curr_note>>6) & 0x1);       // accent
	  
			  set_note_led(curr_note);
		    }
		  }
		  else { 
		    if (!running_stepwrite) {
		      running_stepwrite = TRUE;
			  runstepwrite_pattidx = 0;
			}
			else {
	  if (((runstepwrite_pattidx+1) >= PATT_LENGTH) ||
	      (pattern_buff[runstepwrite_pattidx] == END_OF_PATTERN)) {
		running_stepwrite = FALSE;
	    runstepwrite_pattidx = 0;
		}
	  else
	    runstepwrite_pattidx++;
			}
		  }
        }
		else if (running_stepwrite) {
	  // get previous note from pattern
	  if (runstepwrite_pattidx == 0) {
	    // search thru the buffer -forward- to find the EOP byte
	    while ((runstepwrite_pattidx < PATT_LENGTH-1) && 
		   (pattern_buff[runstepwrite_pattidx] != END_OF_PATTERN))
	      runstepwrite_pattidx++;
	  } else {
	    runstepwrite_pattidx--;
	  }
		
		}
	  }
	  note_entered = FALSE;
    }
      	
    // if they hit done, save buffer to memory
    if (just_pressed(KEY_DONE)) {
      if (in_stepwrite_mode) {
	
	if (curr_pattern_index+1 < PATT_LENGTH)
	  pattern_buff[curr_pattern_index+1] = 0xff;

	stop_stepwrite_mode();
      }
	  running_stepwrite = FALSE;
	  set_led_blink(LED_DONE);
	  while (1) {
	    clock_ticks();
	    read_switches();
		patt_bank = bank;
		i = get_lowest_numkey_pressed();
		if (i != 0) patt_location = i - 1;
		if (just_pressed(KEY_DONE)) break;
	  }
      write_pattern(patt_bank, patt_location);
      dirtyflag = 0; // not dirty anymore, saved!
      clear_led_blink(LED_DONE);
    }


}

void edit_live() {
  in_runwrite_mode = 1;
  edit_pattern();
}

void start_runwrite_mode() {
  //putstring("start runwrite\n\r");
  curr_pattern_index = 0;
  in_runwrite_mode = 1;
  set_led(LED_RS); 
  note_off(0);
  if (sync == INTERNAL_SYNC)
    while ((note_counter & 0x1) || dinsync_counter < DINSYNC_PPQ/4);  // wait for the tempo interrupt to be ready for a note-on
  play_loaded_pattern = 1;
  playing = TRUE;
  midi_putchar(MIDI_START);
}

void clear_leds_and_note_off()
{
  clear_blinking_leds();
  clear_key_leds();
  clear_bank_leds();
  set_bank_led(bank);
  note_off(0);
}

void stop_runwrite_mode() {
  //putstring("stop runwrite\n\r");
  play_loaded_pattern = 0;
  clear_leds_and_note_off();
  in_runwrite_mode = 0;
  clear_led(LED_RS);
  playing = FALSE;
  midi_putchar(MIDI_STOP);
}

void start_stepwrite_mode() {
  //putstring("start stepwrite\n\r");
  in_stepwrite_mode = 1;
  set_led(LED_NEXT); 
  clear_bank_leds();
  set_bank_led(0);
  note_off(0);
}

void stop_stepwrite_mode() {
  //putstring("stop stepwrite\n\r");
  in_stepwrite_mode = 0;
  clear_led(LED_NEXT); 
  dirtyflag = 0;
  clear_led(LED_DONE);
  clear_leds_and_note_off();
}
