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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <stdio.h>
#include "main.h"
#include "led.h"
#include "switch.h"
#include "delay.h"
#include "pattern.h"
#include "track.h"
#include "compcontrol.h"
#include "keyboard.h"
#include "midi.h"
#include "eeprom.h"
#include "synth.h"
#include "dinsync.h"

#define UART_BAUDRATE 19200UL   
#define MIDI_BAUDRATE 31250UL       // the MIDI spec baudrate
uint8_t rand;

uint16_t tempo;
uint16_t newtempo; // changed by the interrupt then updated to the current tempo?

uint8_t sync = INTERNAL_SYNC;

uint16_t timer3_init;  // the value timer 3 is initialized

extern uint8_t bank, function;  // in switch.c /* removed volatile */
extern uint8_t midi_addr;       // in midi.c

///////////////////////////////////////// TEMPO
uint8_t next_random_note;
volatile uint8_t note_counter = 0;


extern int8_t curr_pitch_shift;
extern int8_t next_pitch_shift;

// from track_edit.c
extern uint8_t curr_track_index;/* removed volatile */
extern uint8_t play_loaded_track;/* removed volatile */
extern uint16_t track_buff[TRACK_SIZE];    // the 'loaded' pattern buffer/* removed volatile */
extern uint16_t curr_patt;     // the current pattern playing in the track/* removed volatile */

// from pattern_edit.c
extern uint8_t curr_pattern_index;/* removed volatile */
extern uint8_t play_loaded_pattern; // are we playing?/* removed volatile */
extern uint8_t pattern_buff[PATT_SIZE];    // the 'loaded' pattern buffer/* removed volatile */
uint8_t curr_note, prev_note = 0;

// from pattern_play.c
extern uint8_t curr_chain[MAX_CHAIN];/* removed volatile */
extern uint8_t next_chain[MAX_CHAIN];/* removed volatile */
extern uint8_t curr_chain_index;/* removed volatile */
extern uint8_t curr_bank, next_bank;/* removed volatile */
extern uint8_t all_accent, all_slide, all_rest; // all the time/* removed volatile */
extern uint8_t playing;/* removed volatile */
extern uint8_t swing_time;
extern uint8_t new_swing_time;

extern volatile uint8_t dinsync_counter;  // defined in dinsync.c

uint8_t swing_it = 0;
extern uint8_t loop;
extern uint8_t loop_end;
extern uint8_t loop_start;
uint8_t running_stepwrite = FALSE;
extern uint8_t runstepwrite_pattidx;

/* 8th note stuff */
extern uint8_t eighths;
uint8_t skipit = FALSE;
uint8_t runhalf = FALSE;
uint8_t onemore = FALSE;

/* settings */
uint8_t settings = 0;

extern uint8_t PATT_LENGTH;
/* */

// the 'tempo' interrupt! (on timer 3) 
// gets called 2*4*DINSYNC_PPQ times per beat (192 calls per beat @ sync24)
// fastest is 300BPM -> 1ms
SIGNAL(SIG_OVERFLOW3) {
  TCNT3 = timer3_init;  // FIXME, use CTC mode
  do_tempo();
}

void dispatch_note_off(uint8_t pitch_shift);
void dispatch_note_on(uint8_t pitch_shift);
void load_next_chain(uint8_t reset);

// for all intents and purposes, this is basically a software interrupt!
// call sei() before returning, since interrupts are disabled during tempo
void do_tempo(void) {
  uint8_t curr_function = function;
  uint8_t division_factor = PATT_LENGTH/4;
    
  cli();

  // if the sync is internal or whatever, we have to generate dinsync/midisync msgs
//  if ((sync != MIDI_SYNC) && (sync != DIN_SYNC)) {// && (curr_function != KEYBOARD_MODE_FUNC)) {
    if (dinsync_counter >= DINSYNC_PPQ/division_factor) {
      dinsync_counter = 0;
	  if (swing_it) swing_time = new_swing_time;
	  if (!(note_counter & 0x1)) swing_it = !swing_it;
	}
    // 24 pulses per quarter, increment
	if ((sync != MIDI_SYNC) && (sync != DIN_SYNC)) {
    if (dinsync_counter & 0x1)
      cbi(DINSYNC_PORT, DINSYNC_CLK);
    else {
      sbi(DINSYNC_PORT, DINSYNC_CLK); // rising edge on note start
      midi_putchar(MIDI_CLOCK);
    }
	}
    // make sure that all notes actually start on the zero count
    // so that tempo and SYNC are aligned. 

	if ((swing_it && dinsync_counter != swing_time) ||
		(!swing_it && dinsync_counter != 0)){
	    dinsync_counter++;
		sei();
		return;
	}
	dinsync_counter++;
//  }

/* 8th note stuff */
skipit = !skipit;

if ((!runhalf && !onemore) || !skipit){
/* */

  // reset note counter
  if( note_counter >= 8 )
    note_counter = 0;
  
  if (note_counter & 0x1) {       // sixteenth notes
  
    switch(curr_function) {
    case RANDOM_MODE_FUNC:
    dispatch_note_off(curr_pitch_shift);
      break;

    case EDIT_TRACK_FUNC:
      if (play_loaded_pattern || play_loaded_track) {
	if (curr_note != 0xFF) {
	  note_off(((curr_note >> 7) & 0x1)  | all_slide);        // slide
	}
      }
      if (play_loaded_track) {
	// last note of this pattern this pattern?
	if ((curr_pattern_index >= PATT_LENGTH) ||
	    (pattern_buff[curr_pattern_index] == 0xFF)) {

	  curr_pattern_index = 0;          // start next pattern in chain
	  curr_track_index++;      // go to next patt in chain
	  // last pattern in this chain?
	  if ((curr_track_index >= TRACK_SIZE) ||
	      (track_buff[curr_track_index] == END_OF_TRACK)) {
	    //putstring("track loop\n\r");
	    curr_track_index = 0;
	  }

	  curr_patt = track_buff[curr_track_index];
	  if (curr_patt == END_OF_TRACK) {
	    // dont load the pattern, but make sure the pattern buffer wont play
	    pattern_buff[0] = END_OF_PATTERN;
	  } else {
	    curr_pitch_shift = load_curr_patt();
	    clear_bank_leds();
	  }
	}
      }
	
      break;

    case A_FUNC:
    case EDIT_PATTERN_FUNC: 
      if (play_loaded_pattern) {
		dispatch_note_off(curr_pitch_shift);

	if ((curr_pattern_index >= PATT_LENGTH) || 
	    (pattern_buff[curr_pattern_index] == 0xFF)) {
	  curr_pattern_index = 0;
	} 
      }
      break;

    case PLAY_PATTERN_MIDISYNC_FUNC:
    case PLAY_PATTERN_DINSYNC_FUNC:
    case PLAY_PATTERN_FUNC: 
      if (playing) {
	    dispatch_note_off(curr_pitch_shift);
		
		
	  if (loop && curr_pattern_index == loop_start-1) {
	    curr_pitch_shift = next_pitch_shift;
/* 8th note stuff */
if (runhalf != eighths)
{
runhalf = eighths;
skipit = runhalf;
onemore = !skipit;
}
/* */
		if (!chains_equiv(next_chain, curr_chain))	{
		  curr_pattern_index = PATT_LENGTH;
		  loop_start = 1;
		  loop_end = PATT_LENGTH;
		  loop = FALSE;
		}
	  }
		
		
	// last note of this pattern?
	if ((curr_pattern_index >= PATT_LENGTH) ||
	    (pattern_buff[curr_pattern_index] == 0xFF)) {

/* 8th note stuff */
if (runhalf != eighths)
{
runhalf = eighths;
skipit = runhalf;
onemore = !skipit;
}
/* */

	  curr_pattern_index = 0;          // start next pattern in chain
	  curr_chain_index++;      // go to next patt in chain
	  // last pattern in this chain?
	  if ((curr_chain_index >= MAX_CHAIN) ||
	      (curr_chain[curr_chain_index] == 0xFF)) {
	    curr_chain_index = 0;
	  }
	  	  
	  load_next_chain(TRUE);

	  load_pattern(curr_bank, curr_chain[curr_chain_index]);
        loop_end=0;
        while (loop_end != PATT_LENGTH && pattern_buff[loop_end] != 0xFF) {
            loop_end++;
        }
	}
      }
	  	  
      break;


    case PLAY_TRACK_MIDISYNC_FUNC:
    case PLAY_TRACK_DINSYNC_FUNC:
    case PLAY_TRACK_FUNC: 
      if (playing) {
	    dispatch_note_off(curr_pitch_shift + get_pitchshift_from_patt(curr_patt));

	// if this is the last note in the pattern, go to the next in track
	if ((curr_pattern_index >= PATT_LENGTH) || 
	    (pattern_buff[curr_pattern_index] == END_OF_PATTERN)) {
	  curr_pattern_index = 0;          // start next pattern in track
	  curr_track_index++;      // go to next patt in chain
	  /*
	  putstring("Next Pattern in track #"); putnum_ud(curr_track_index);
	  putstring(" = 0x"); putnum_ud(track_buff[curr_track_index]);
	  putstring("\n\r");
	  */
	  // if this is the end of the track, go to the next one in the chain
	  if ((curr_track_index >= TRACK_SIZE) ||
	      (track_buff[curr_track_index] == END_OF_TRACK)) {
	    curr_track_index = 0;
	    curr_chain_index++;      // go to next track in chain
	    /*
	    putstring("Next track in chain #"); putnum_ud(curr_chain_index);
	    putstring(" = 0x"); putnum_ud(curr_chain[curr_chain_index]);
	    putstring("\n\r");
	    
	    putstring("curr chain = ");
	    for (i=0; i<MAX_CHAIN; i++) {
	      if (curr_chain[i] >= 8)
		break;
	      putnum_ud(curr_chain[i]);
	      uart_putchar(' ');
	    }
	    putstring("\n\r");
	    */
	    // last pattern in this chain, go to next chain
	    if ((curr_chain_index >= MAX_CHAIN) ||
		(curr_chain[curr_chain_index] == 0xFF)) {
	      curr_chain_index = 0;
		  
		  load_next_chain(FALSE);
	      	      
	    }
	    load_track(curr_bank, curr_chain[curr_chain_index]);
	  }
	  curr_patt = track_buff[curr_track_index];
	  load_curr_patt();
	}
      }
      break;
      
    } //end switch nr 1
	
  } else {
  /* break out */
    prev_note = curr_note;
	
    switch(curr_function) {
    case RANDOM_MODE_FUNC:
      curr_note = next_random_note;
      next_random_note = random();
      dispatch_note_on(curr_pitch_shift);            
      break;

    case EDIT_TRACK_FUNC: 
      if (play_loaded_pattern || play_loaded_track) {

	if (play_loaded_pattern) {
	  // load up the next note
	  if ((curr_pattern_index >= PATT_LENGTH) || 
	      (pattern_buff[curr_pattern_index] == END_OF_PATTERN)) {
	    curr_pattern_index = 0;
	  }
	}
	
	curr_note = pattern_buff[curr_pattern_index];
	curr_pattern_index = get_next_patt_idx();
	
	if (curr_note != 0xFF) {
	  dispatch_note_on(curr_pitch_shift);
	}
      }
      break;
      
    case A_FUNC:
    case EDIT_PATTERN_FUNC: 
      if (play_loaded_pattern) {
	// load up the next note
	clear_bank_leds();
	set_bank_led(curr_pattern_index);
	curr_note = pattern_buff[curr_pattern_index];
	curr_pattern_index = get_next_patt_idx();
	//putstring("\n\rlocation "); putnum_ud(curr_pattern_index);
	//putstring(" note: 0x"); putnum_uh(curr_note);
	
	if (curr_note != 0xFF) {
	  if (!running_stepwrite) set_note_led(curr_note);
	  else set_bank_led(runstepwrite_pattidx);
	  dispatch_note_on(curr_pitch_shift + get_pitchshift_from_patt(curr_patt));
	}
      }
      break;

    case PLAY_PATTERN_MIDISYNC_FUNC:
    case PLAY_PATTERN_DINSYNC_FUNC:
    case PLAY_PATTERN_FUNC: 
      if (playing) {
	// in pattern play we show each note indexed in the pattern
	  clear_bank_leds();
	  set_bank_led(curr_pattern_index);
      }
      // no break here! continue on to shared track/pattern play code...
    case PLAY_TRACK_MIDISYNC_FUNC:
    case PLAY_TRACK_DINSYNC_FUNC:
    case PLAY_TRACK_FUNC: 
      if (playing) {
	// in track play, we blink the track location but thats
	// taken care of in the note off portion (when patterns are loaded)

	curr_note = pattern_buff[curr_pattern_index];
	curr_pattern_index = get_next_patt_idx();

	// end of pattern? (either memory or 0xFF) 
	if (curr_note != 0xFF) {
	  dispatch_note_on(curr_pitch_shift + get_pitchshift_from_patt(curr_patt));
	} 
      }
      break;
    } //end switch nr 2
	
  }

  // blinkie the tempo led & any other LEDs!
  if (note_counter < 4) {
    set_led(LED_TEMPO);
    blink_leds_off();
   }
  else if (note_counter < 8) {
    clear_led(LED_TEMPO);
    blink_leds_on();
  }

  clock_leds();

  note_counter++;

/* 8th note stuff */
} else if (onemore) onemore = FALSE;
/* */

  sei();
}

///////////////////////////////////// 'RTC' 1ms timer/counter
volatile extern uint8_t debounce_timer;         // in switch.c
volatile extern uint16_t tap_tempo_timer;        // in pattern_play.c
extern uint8_t last_dinsync_c;
volatile extern int16_t dinsync_clocked, dinsync_clock_timeout;
volatile extern uint16_t uart_timeout;

volatile uint8_t blinktimer = 0;

SIGNAL(SIG_OUTPUT_COMPARE0) {
  uint8_t curr_dinsync_c;

  if (debounce_timer != 0xFF) 
    debounce_timer++;
  if (tap_tempo_timer != 0xFFFF)
    tap_tempo_timer++;
  if (uart_timeout != 0xFFFF)
    uart_timeout++;

  if ((sync!=DIN_SYNC) && (dinsync_clock_timeout != 0)) {
    dinsync_clock_timeout--;
    if (dinsync_clock_timeout == 0) {
      cbi(DINSYNC_PORT, DINSYNC_CLK);    // lower the clock
    }
  }

  if (sync == DIN_SYNC) {
    curr_dinsync_c = (DINSYNC_PIN >> DINSYNC_CLK) & 0x1;
    
    if (!last_dinsync_c && curr_dinsync_c) {
      dinsync_clocked++;   // notify a clock was recv'd
      midi_putchar(MIDI_CLOCK); // send a midi clock message immediately
      // (DINSYNC to MIDISYNC conversion)
      last_dinsync_c = curr_dinsync_c;
    } else {
      last_dinsync_c = curr_dinsync_c;      
    }
  }

  if (! is_tempo_running()) {
    if (blinktimer == 200) {
      blinktimer = 0;
      // turn off
      blink_leds_off();
    } else if (blinktimer == 100) {
      // turn on
      blink_leds_on();
    }
    blinktimer++;
  }
}


///////////////////////////////////// pin change interrupts
uint8_t last_tempo;
SIGNAL(SIG_PIN_CHANGE0) {

  uint8_t curr_tempo;

  // tempo knob change!

  curr_tempo = TEMPO_PIN & 0x3; // pins A0 and A1

  if (curr_tempo != last_tempo) {
    if ((last_tempo == 3) && (curr_tempo == 2)) {
      newtempo--;
    }
    if ((last_tempo == 2) && (curr_tempo == 3)) {
      newtempo++;
    }

    if (newtempo > MAX_TEMPO)
      newtempo = MAX_TEMPO;
    if (newtempo < MIN_TEMPO)
      newtempo = MIN_TEMPO;
    last_tempo = curr_tempo;
  }
  
}

void do_settings(void) {
    int i = 0;
    uint8_t new_settings = 0;
    new_settings = settings;
    for (i = 0; i < 8; ++i) {
        if (settings & 1<<i ) set_numkey_led(i+1); else clear_numkey_led(i+1);
        if (i == get_lowest_numkey_just_pressed()-1) new_settings ^= 1<<i;
    }
    if (settings != new_settings) {
        internal_eeprom_write8(SETTINGS_EEADDR, new_settings);
        settings = new_settings;
    }
}

////////////////////////////////// main()
int main(void) {
  ioinit();        // set up IO ports and the UART

  // start the tempo timer
  init_tempo();

  // start the 'rtc' timer0
  init_timer0();

  // start the 'dinsync' timer2
  //init_timer2();

  rand = tempo;            // stupid initialization, do better?

  dinsync_set_out(); // output DINSYNC

  init_midi();

  sei();  // enable interrupts

  settings = internal_eeprom_read8(SETTINGS_EEADDR); //read settings from EEPROM
    
  // the main loop!
  while (1) {
    read_switches();
    switch (function) {
    case COMPUTER_CONTROL_FUNC:
      //putstring("CompControl\n\r");
      sync = INTERNAL_SYNC;
      do_computer_control();
      break;
    case EDIT_PATTERN_FUNC:
      //putstring("PattEdit\n\r");
      sync = INTERNAL_SYNC;
      do_pattern_edit();
      break;
    case PLAY_PATTERN_FUNC:
    case PLAY_TRACK_FUNC: 
      //putstring("PattPlay\n\r");
      sync = INTERNAL_SYNC;
      do_patterntrack_play();
      break;
    case PLAY_PATTERN_DINSYNC_FUNC:
    case PLAY_TRACK_DINSYNC_FUNC:
      //putstring("PattPlay DINSYNC\n\r");
      sync = DIN_SYNC;
      do_patterntrack_play();
      break;
    case PLAY_PATTERN_MIDISYNC_FUNC:
    case PLAY_TRACK_MIDISYNC_FUNC:
      //putstring("PattPlay MidiSYNC\n\r");
      sync = MIDI_SYNC;
      do_patterntrack_play();
      break;
    case EDIT_TRACK_FUNC:
      //putstring("TrackEdit\n\r");
      sync = INTERNAL_SYNC;
      do_track_edit();
      break;
//    case PLAY_TRACK_FUNC: 
      //putstring("TrackPlay\n\r");
//      sync = INTERNAL_SYNC;
//      do_patterntrack_play();
//      break;
//    case PLAY_TRACK_DINSYNC_FUNC:
      //putstring("TrackPlay DINSYNC\n\r");
//      sync = DIN_SYNC;
//      do_patterntrack_play();
//      break;
//    case PLAY_TRACK_MIDISYNC_FUNC:
      //putstring("TrackPlay MIDISync\n\r");
//      sync = MIDI_SYNC;
//      do_patterntrack_play();
//      break;
    case MIDI_CONTROL_FUNC:
      //putstring("MIDIControl\n\r");
      sync = MIDI_SYNC;
      do_midi_mode();
      break;
    case KEYBOARD_MODE_FUNC:
      //putstring("Keyboard\n\r");
      sync = INTERNAL_SYNC;
      do_keyboard_mode();
      break;
    case RANDOM_MODE_FUNC: {
      //uint8_t dinsync_started = 0; // stopped
      //uint8_t dinsync_lastpulse = 0; // 
      //putstring("rAnD0m\n\r");
      sync = INTERNAL_SYNC;
      turn_on_tempo();
      clear_all_leds();
      //dinsync_start();
      while (1) {
	read_switches();
	
	if (function != RANDOM_MODE_FUNC) {
	  //dinsync_stop();
	  turn_off_tempo();
	  break;
	}
      }
      break;
    }
    case C_FUNC: 
      do_settings();
      break;
    case A_FUNC:
      // edit the pattern with the midi sync axxxion
//      sync = MIDI_SYNC;
//      do_pattern_edit();
//      break;
    case B_FUNC:
//      clear_all_leds();
//      clock_leds();
    default:
      //putstring("???"); putnum_ud(function);
      // something else
      break;
    }
  }
}

/********************* */
void init_timer0(void) {
  sbi(TIMSK, 0);          // timer0 overflow interrupt enable
  TCCR0 = (1 << WGM01) | 0x3;            // compare mode, clk/64
  OCR0 = 250;             // 1KHz
}

/* Remove? 130818 */
//void init_timer2(void) {
//  sbi(TIMSK, 0);
//  TCCR2 = (1<<WGM21) | 0x3; // compare mode, clk/32
//  OCR2 = 50;             // 10khz
//
//}

void init_tempo(void) {
  sbi(PCMSK0, PCINT0); // detect change on pin A0
  sbi(PCMSK0, PCINT1); // detect change on pin A1
  sbi(GICR, PCIE0);    // enable pin change interrupt for tempo knob detect

  change_tempo((internal_eeprom_read8(TEMPO_EEADDR)<< 8) |
	       internal_eeprom_read8(TEMPO_EEADDR+1) );
  note_counter = 0;
  sbi(ETIMSK, TOIE3); // enable tempo interrupt
}

// reset the note counter. change the tempo back.
void turn_on_tempo() {
  sbi(ETIMSK, TOIE3);
}

void turn_off_tempo() {
  clear_led(LED_TEMPO);
  cbi(ETIMSK, TOIE3);
}

uint8_t is_tempo_running() {
  return (ETIMSK >> TOIE3) & 0x1;
}

void change_tempo(uint16_t set_tempo) {
  uint16_t t3_prescale;
  uint32_t num_instr;
  uint16_t top_num_instr;

  if (set_tempo > MAX_TEMPO) {
    set_tempo = MAX_TEMPO;
  }
  if (set_tempo < MIN_TEMPO) {
    set_tempo = MIN_TEMPO;
  }

  newtempo = tempo = set_tempo;
    if (!(settings & 1<<1)) {
        internal_eeprom_write8(TEMPO_EEADDR, tempo >> 8);
        internal_eeprom_write8(TEMPO_EEADDR+1, tempo & 0xFF);
    }

  /*
    putnum_ud(tempo);
    putstring(" BPM\n\r");
  */
  send_tempo(tempo);

  // figure out what the interrupt should be!
  // (use counter 3 for finest resolution!)
  num_instr = F_CPU * 60;

/*
  num_instr /= set_tempo;
  num_instr /= 4;         // sixteenth notes!
  num_instr /= 2;         // call twice per quarter
  num_instr /= DINSYNC_PPQ/4;  // do dinsync on same interrupt
*/
  num_instr /= set_tempo*2*DINSYNC_PPQ;  
  
  top_num_instr = num_instr >> 16;
  if (!top_num_instr) {
    t3_prescale = 1;
    timer3_init = num_instr;
    TCCR3B = 1;
  } else if ((top_num_instr & ~0x7) == 0) {
    t3_prescale = 8;
    timer3_init = num_instr >> 3;
    TCCR3B = 2;
  } else if ((top_num_instr & ~0xF) == 0) {
    t3_prescale = 16; 
    timer3_init = num_instr >> 4;
    TCCR3B = 6;
  } else if ((top_num_instr & ~0x1F) == 0) {
    t3_prescale = 32;
    timer3_init = num_instr >> 5;
    TCCR3B = 7;
  } else if ((top_num_instr & ~0x3F) == 0) {
    t3_prescale = 64;
    timer3_init = num_instr >> 6;
    TCCR3B = 3;
  } else if ((top_num_instr & ~0xFF) == 0) {
    t3_prescale = 256;
    timer3_init = num_instr >> 8;
    TCCR3B = 4;
  } else if ((top_num_instr & ~0x3FF) == 0) {
    t3_prescale = 1024;
    timer3_init = num_instr >> 10;
    TCCR3B = 5;
  } else {
    t3_prescale = 0;
    TCCR3B = 0;
  }
  
  timer3_init *= -1;
  
  //printf("T3 Prescale: %d. Init: 0x%x\n\r", t3_prescale, timer3_init);
  TCNT3 = timer3_init;
}


void dispatch_note_off(uint8_t pitch_shift)
{
  if (curr_note != 0xFF) {
	if (((curr_note>>7) & 0x1) | all_slide) { 
    // check if the note had slide on it 
	  note_off(1); // slide
	// DONT send a midi note off
	} else {
	  note_off(0); // no slide
	  if ((curr_note & 0x3F) != 0)  // not rest
	    midi_send_note_off(curr_note + pitch_shift);
	  else
	    midi_send_note_off(curr_note);
	}
  }
	if ( (prev_note != 0xFF) &&
	     (((prev_note>>7) & 0x1) | all_slide ) ) {
	  if ((prev_note & 0x3F) != 0)  // not rest
	    midi_send_note_off(prev_note + pitch_shift);
	  else
	    midi_send_note_off(prev_note);
	}
  
}

void dispatch_note_on(uint8_t pitch_shift)
{
    uint8_t ps = pitch_shift;
	
	if (all_rest)
	  curr_note &= 0xC0;


	if ((curr_note & 0x3F) == 0) ps = 0;

	  note_on((curr_note & 0x3F) + ps,
		      (prev_note >> 7) | all_slide,  // slide is from prev note!
		      ((curr_note>>6) & 0x1) | all_accent);       // accent
	  midi_send_note_on(curr_note + ps);
} 

void load_next_chain(uint8_t reset) {
  uint8_t i;
	  if (!chains_equiv(next_chain, curr_chain) ||
	      (curr_bank != next_bank)) {

	    // copy next pattern chain into current pattern chain
	    for (i=0; i<MAX_CHAIN; i++) 
	      curr_chain[i] = next_chain[i];
	    
	    if (reset) curr_chain_index = 0;  // reset to beginning

	    // reset the pitch
	    next_pitch_shift = curr_pitch_shift = 0;

	    clear_notekey_leds();
	    clear_blinking_leds();
	  }
	  
	  curr_bank = next_bank;
	  curr_pitch_shift = next_pitch_shift;
}

/********************* Utilities *********************/
/*
void step() {
  uart_getchar();
}


void halt() {
  putstring("halting");
  // turn off interrupts??
  while (1) {
  }
}
*/


uint8_t random(void) {
  rand = ((((rand >> 7) ^ (rand >> 6) ^ (rand >> 4) ^ (rand >> 2))
	       & 00000001)
	      | (rand << 1)); /*Or with the register shifted right.*/
  return rand; /*Return the first bit.*/
}


/************************** UART *************************/
/*
void putstring(char *str) {
  while (str[0] != 0) {
    uart_putchar(str[0]);
    str++;
  }
}

void putnum_ud(uint16_t n) {
  uint16_t pow;

  for (pow = 10000UL; pow >= 10; pow /= 10) {
    if (n / pow) {
      uart_putchar((n/pow)+'0');
      n %= pow;
      pow/= 10;
      break;
    }
    n %= pow;
  }
  for (;pow != 0; pow /= 10) {
    uart_putchar((n/pow)+'0');
    n %= pow;
  }
  return;
}
*/

void printhex(uint8_t hex) {
  hex &= 0xF;
  if (hex < 10)
    uart_putchar(hex + '0');
  else
    uart_putchar(hex + 'A' - 10);
}

void putnum_uh(uint16_t n) {
  if (n >> 12)
    printhex(n>>12);
  if (n >> 8)
    printhex(n >> 8);
  if (n >> 4)
    printhex(n >> 4);
  printhex(n);

  return;
}
 
int uart_putchar(char c)
{
   loop_until_bit_is_set(UCSR1A, UDRE1);
   UDR1 = c;
   return 0;
}

int uart_getch() {     // checks if there is a character waiting!
  if (bit_is_set(UCSR1A, RXC1))
    return 1;
  return 0;
}


int uart_getchar(void) {
  char c;
  loop_until_bit_is_set(UCSR1A, RXC1);
  c = UDR1;
  return (int)c;
}

//**************************************************
//         Internal EEPROM
//**************************************************

uint8_t internal_eeprom_read8(uint16_t addr) {
  loop_until_bit_is_clear(EECR, EEWE); // wait for last write to finish
  EEAR = addr;
  sbi(EECR, EERE);        // start EEPROM read
  return EEDR;            // takes only 1 cycle
}

void internal_eeprom_write8(uint16_t addr, uint8_t data) {
  //printf("writing %d to addr 0x%x...", data, addr);
  loop_until_bit_is_clear(EECR, EEWE); // wait for last write to finish
  EEAR = addr;
  EEDR = data;
  cli();                // turn off interrupts 
  sbi(EECR, EEMWE);     // these instructions must happen within 4 cycles
  sbi(EECR, EEWE);
  sei();                // turn on interrupts again
  //putstring("done\n\r");
}

void ioinit() {
  uint16_t baud = (F_CPU / (16 * UART_BAUDRATE)) - 1;
  
  /* setup the main UART */
  UCSR1B |= (1<<RXEN1) | (1<<TXEN1);    // read and write & intr
  UBRR1L = (uint8_t)baud;               // set baudrate
  UBRR1H = (uint8_t)(baud>>8);
  // first flush the input
  while (uart_getch()) {
    uart_getchar();
    delay_ms(10);
  }
  UCSR1B |= (1<<RXCIE1); // now turn on interrupts

  /* setup the MIDI UART */
  baud = (F_CPU / (16 * MIDI_BAUDRATE)) - 1;
  UCSR0B |= (1<<RXEN0) | (1<<TXEN0)| (1<<RXCIE0);    // read and write, interrupt on recv.
  UBRR0L = (uint8_t)baud;               // set baudrate
  UBRR0H = (uint8_t)(baud>>8);

  DDRA = 0xC0;              // led latch (o), rotary com (o), rot1, rot2, rot4, ro8, tempoa, tempob 
  PORTA = 0x3C;              // pullups on rotary1,2,4,8
  
  DDRB = 0xBB;              // spi_clk, spi_in, spi_out, NC, TX, RX, NC, switch latch (o)
  PORTB = 0x0;

  DDRC = 0xFF;              // accent, slide, note[0-5]

  DDRD = 0xFF;              // dinsync1, 2, 3, 4 (outputs), NC, NC, MIDI TX & RX
  DDRE = 0xFF;               // note latch, gate, NC
  

  SPCR = (1<<SPE)|(1<<MSTR) | 0x1 ; // master spi, clk=fosc/8 = 2mhz
}
