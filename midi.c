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
#include "midi.h"
#include "switch.h"
#include "synth.h"
#include "main.h"
#include "led.h"
#include "dinsync.h"
#include "delay.h"

uint8_t midion_accent_velocity = 127;
uint8_t midioff_velocity = 32;
uint8_t midion_noaccent_velocity = 100;

uint8_t midi_out_addr;  // store this in EEPROM
uint8_t midi_in_addr;   // store this in EEPROM, too!

extern volatile uint8_t dinsync_counter;
extern volatile int16_t dinsync_clock_timeout;

uint8_t midi_running_status = 0;  // suck!
volatile int16_t midisync_clocked = 0;

extern uint8_t function, bank;
#define function_changed (function != MIDI_CONTROL_FUNC)

extern uint8_t playing;
extern uint8_t sync;

extern uint8_t prev_note;

#define ACCENT_THRESH 100

#define MIDI_Q_SIZE 32
volatile uint8_t midi_q[MIDI_Q_SIZE];      // cyclic queue for midi msgs
volatile static uint8_t head_idx = 0;
volatile static uint8_t tail_idx = 0;

// interrupt on receive char
SIGNAL(SIG_USART0_RECV) {
  char c = UDR0;
  
  if (c == MIDI_CLOCK) {

    // raise dinsync clk immediately, and also sched. to drop clock
    // (MIDISYNC -> DINSYNC conversion);
    if (sync == MIDI_SYNC) {
      sbi(DINSYNC_PORT, DINSYNC_CLK); // rising edge on note start
      dinsync_clock_timeout = 5;      // in 5ms drop the edge, is this enough?
    }

    if (! playing)
      return;
    midisync_clocked++;
    return;
  }

  //putstring("0x"); putnum_uh(c); putstring("   ");
  midi_q[tail_idx++] = c;    // place at end of q
  tail_idx %= MIDI_Q_SIZE;

  if (tail_idx == head_idx) {
    // i.e. there are too many msgs in the q
    // drop the oldest msg?
    head_idx++;
    head_idx %= MIDI_Q_SIZE;
  }
}


uint8_t get_midi_addr(uint8_t eeaddr) {
  uint8_t midi_addr;
  
  midi_addr = internal_eeprom_read8(eeaddr);
  if (midi_addr > 15)
    midi_addr = 15;
  return midi_addr;
}


void init_midi(void) {
  midi_in_addr = get_midi_addr(MIDIIN_ADDR_EEADDR);
  midi_out_addr = get_midi_addr(MIDIOUT_ADDR_EEADDR);
}

void do_midi_mode(void) {
  char c;
  uint8_t last_bank;
  uint8_t note, velocity;

  // turn tempo off!
  turn_off_tempo();

  // show midi addr on bank leds
  clear_bank_leds();
  set_bank_led(midi_in_addr);

  read_switches();
  delay_ms(100);
  read_switches();
  delay_ms(100);
  read_switches();
  last_bank = bank;
  prev_note = 255;        // no notes played yet

  while (1) {
    read_switches();
    if (function_changed) {
      midi_notesoff(); // clear any stuck notes
      return;
    }

    if (last_bank != bank) {
      // bank knob was changed, change the midi address
      midi_in_addr = bank;

      // set the new midi address (burn to EEPROM)
      internal_eeprom_write8(MIDIIN_ADDR_EEADDR, midi_in_addr);

      clear_bank_leds();
      set_bank_led(midi_in_addr);

      last_bank = bank;
    }

    // if theres a char waiting in midi queue...
    if (midi_getch()) {
      // if its a command & either for our address or 0xF,
      // set the midi_running_status
      c = midi_getchar();

      if (c >> 7) {       // if the top bit is high, this is a command
	if ((c >> 4 == 0xF) ||    // universal cmd, no addressing
	    ((c & 0xF) == midi_in_addr)) {  // matches our addr
        switch(c) {
          case MIDI_START:
          { 
            dinsync_counter = 0;
            dinsync_start();
            break;}
          case MIDI_STOP:
          { dinsync_stop();
            break;}
        }
	  midi_running_status = c >> 4;
	} else {
	  // not for us, continue!
	  midi_running_status = MIDI_IGNORE; 
	  continue;
	}
      }

      switch (midi_running_status) {
      case MIDI_IGNORE:
	{
	  // somebody else's data, ignore
	  break;
	} 
      case MIDI_NOTE_ON:
	{
	  if (c >> 7)  // if the last byte was a command then we have to get the note
	    note = midi_getchar();
	  else
	    note = c;  // otherwise, this was a running status, and c is the note

	  velocity = midi_getchar();
	  /*
	    putstring("MIDI note on (note "); putnum_ud(note);
	    putstring(") (velocity "); putnum_ud(velocity);
	    putstring(")\n\r");
	  */

	  midi_note_on(note, velocity);
	  break;
	}
      case MIDI_NOTE_OFF:
	{
	  if (c >> 7) 
	    note = midi_getchar();
	  else
	    note = c;

	  velocity = midi_getchar();
	  /*
	    putstring("MIDI note off (note "); putnum_ud(note);
	    putstring(") (velocity "); putnum_ud(velocity);
	    putstring(")\n\r");
	  */
	  
	  midi_note_off(note, velocity);
	  
	  break;
	} 

      case MIDI_PITCHBEND:
	{
	  //putstring("MIDI Slide\n\r");

	  break;
	}
      default:
	/*putstring("Received Unknown MIDI: 0x"); putnum_uh(c); 
	  putstring("\n\r"); */
	break;
      }
    }
  }
}
  // midi handling code!

uint8_t midi_recv_cmd(void) {
  uint8_t c;

  if (midi_getch()) {
    c = midi_getchar();
    if (c >> 7) {       // if the top bit is high, this is a command
      if (c >> 4 == 0xF)     // universal cmd, no addressing
	return c;
      
      if ((c & 0xF) == midi_in_addr) {
	midi_running_status = c >> 4;
	return c;
      }
    }
  }
  return 0;
}

void midi_note_off(uint8_t note, uint8_t velocity) {
  if (note == prev_note) {
    note_off(0);
    prev_note = 255;
  }
}

void midi_note_on(uint8_t note, uint8_t velocity) {
  uint8_t slide = 0;

  if (velocity == 0 && note != 0) {
    // strange midi thing: velocity 0 -> note off!
    midi_note_off(note, velocity);
  } else {
    if (prev_note != 255)
      slide = 1;
	  prev_note = note;	  
	  if (note == 0) note = 0x19;
    if (velocity > ACCENT_THRESH) {
      note_on(note - 0x19, slide, 1); // with accent
    } else {
      note_on(note - 0x19, slide, 0); // no accent
    }

//    prev_note = note;
  }
}

void midi_send_note_on(uint8_t note) {
  midi_putchar((MIDI_NOTE_ON << 4) | midi_out_addr);

  if ( (note & 0x3F) == 0) {
    midi_putchar(0);                                 // rest
	//080602
    midi_putchar(0);                                 // velocity 0(= note off)
	}
  else {
    midi_putchar((note & 0x3F) + 0x19);              // note

  if ((note >> 6) & 0x1)              // if theres an accent, give high velocity 
    midi_putchar(midion_accent_velocity);
  else
    midi_putchar(midion_noaccent_velocity);
  }
}

void midi_send_note_off(uint8_t note) {
  midi_putchar((MIDI_NOTE_OFF << 4) | midi_out_addr);  // command

  if ((note & 0x3F) == 0)
    midi_putchar(0);                                 // rest
  else 
    midi_putchar((note & 0x3F) + 0x19);              // note


  midi_putchar(midioff_velocity);                   // velocity
}


int midi_putchar(char c)
{
   loop_until_bit_is_set(UCSR0A, UDRE0);
   UDR0 = c;
   return 0;
}

int midi_getch(void) {     // checks if there is a character waiting!
  if (head_idx != tail_idx)
    return 1;
  return 0;
}

int midi_getchar(void) {
  char c;

  while (head_idx == tail_idx);

  cli();
  c = midi_q[head_idx++];
  head_idx %= MIDI_Q_SIZE;
  sei();

  return c;
}

// sends a midi stop and 'all notes off' message
void midi_stop(void) {
  // if we were generating midi, stop all notes and send a clockstop signal
  if (sync != MIDI_SYNC) { 
    midi_putchar(MIDI_STOP);
    midi_notesoff();
  }
}

void midi_notesoff(void) {
  midi_putchar((MIDI_CONTROLLER<<4) | midi_out_addr);
  midi_putchar(MIDI_ALL_NOTES_OFF);
  midi_putchar(0);
}
