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
#include "main.h"
#include "synth.h"
#include "delay.h"
#include "led.h"
#include "midi.h"
#include "switch.h"
#include "pattern.h"
#include "eeprom.h"

extern uint8_t bank;
extern uint8_t function;
#define function_changed (function != KEYBOARD_MODE_FUNC)

extern const uint8_t loopkey_tab[16];

extern uint8_t midi_out_addr;  // the midi address for outgoing stuff

void do_keyboard_mode(void) {
  signed int shift = 0;
  uint8_t accent=0, slide=0;
  uint8_t i, last_bank;
  
  // turn tempo off!
  turn_off_tempo();
  
  clear_bank_leds();

  read_switches();
  last_bank = bank;
  has_bank_knob_changed(); // ignore startup change

  while (1) {
    read_switches();

    if (function_changed) {
      midi_notesoff();           // turn all notes off
      return;
    }
  
    // show the current MIDI address
    if (!is_bank_led_set(midi_out_addr)) {
      clear_bank_leds();
      set_bank_led(midi_out_addr);
    }

    if (has_bank_knob_changed()) {
      // bank knob was changed, which means they want a different
      // midi addr... OK then!
      midi_out_addr = bank;

      // set the new midi address (burn to EEPROM)
      internal_eeprom_write8(MIDIOUT_ADDR_EEADDR, midi_out_addr);

      last_bank = bank;
    }

    // show the octave
    display_octave_shift(shift);

    for (i=0; i<13; i++) {
      // check if any notes were just pressed
      if (just_pressed(loopkey_tab[i])) {
	note_on((C2+i) + shift*OCTAVE, slide, accent);
	midi_send_note_on( ((C2+i) + shift*OCTAVE) | (accent << 6));
	slide = TRUE;

	// turn on that LED
	set_notekey_led(i);	
      }
      
      // check if any notes were released
      if (just_released(loopkey_tab[i])) {
	midi_send_note_off( ((C2+i) + shift*OCTAVE) | (accent << 6));

	// turn off that LED
	clear_notekey_led(i);
      }
    }

    if (just_pressed(KEY_UP)) {
      if (shift < 2)
	shift++;
    } else if (just_pressed(KEY_DOWN)) {
      if (shift > -1)
	shift--;
    } 

    // check if they turned accent on
    if (just_pressed(KEY_ACCENT)) {
      accent = !accent;
      if (accent)
	set_led(LED_ACCENT);
      else
	clear_led(LED_ACCENT);
    }
      
      // 110109 MIDI sysex send

      if (just_pressed(KEY_NEXT) && is_pressed(KEY_RS)) {
          uint16_t addr = PATTERN_MEM+bank*BANK_SIZE;
          uint8_t msg;

          midi_putchar(0xF0);
          midi_putchar(0x7D);
          midi_putchar(0x03);
          midi_putchar(0x03);
          midi_putchar(0x12);
          // Save the real memory address of the bank (should be safe to send 0xF7 here?)
//          midi_putchar(addr>>8); //high byte
//          midi_putchar(addr&0xFF); // low byte
          // Nah, skip sending the address, let the user save to the current pattern bank instead
          midi_putchar(0x00); 
          midi_putchar(0x00); 
          midi_putchar(0x00); 

          // Need to split every byte in two, MIDI sysex can only be 7-bit, will end on first occurence of F7 otherwise
          for (i=0;i<NUM_LOCS*PATT_SIZE;i++) {
              set_notekey_led(i);//some feedback...130831
              msg=spieeprom_read(addr+i);
              midi_putchar(msg>>4); // high nibble
              midi_putchar(msg&0xF); // low...
//              clear_notekey_led(i>>4 + 1);
          }
          
          midi_putchar(0xF7);
      }

      // 110109 MIDI sysex receive
      
      if (just_pressed(KEY_PREV) && is_pressed(KEY_RS)) {
//          uint8_t msg_buff[8]={0xF0, 0x7D, 0x03, 0x03, 0x12, 0x00, 0x00, 0x00};
          uint16_t addr = PATTERN_MEM+bank*BANK_SIZE;
          uint8_t msg = 0;
          uint8_t msg2 = 0;
          uint8_t patt_buff[128];
//          set_led(LED_DONE);
          while (msg == 0 && !is_pressed(KEY_DONE)) msg=midi_recv_cmd();
          
          if (msg==0xF0 && 0x7D==midi_getchar() && 0x03==midi_getchar() && 0x03==midi_getchar() && 0x12==midi_getchar() 
              && 0x00==midi_getchar() && 0x00==midi_getchar() && 0x00==midi_getchar())
          {
              i=0;
//              while (i<NUM_LOCS*PATT_SIZE && msg!=0xF7 && msg2!=0xF7) {
              while (i<NUM_LOCS*PATT_SIZE && !is_pressed(KEY_DONE)) {
                  msg=midi_getchar();
                  msg2=midi_getchar();
//                  spieeprom_write((msg<<4)+msg2,addr+i);
                  patt_buff[i]=(msg<<4)+msg2;
                  i++;
              }
              for (i=0;i<NUM_LOCS*PATT_SIZE;i++) spieeprom_write(patt_buff[i],addr+i);
          }
//          clear_led(LED_DONE);
      }

 
    // if no keys are held down and there was a note just playing
    // turn off the note.
    if ((NOTE_PIN & 0x3F) && no_keys_pressed()) {
      note_off(0);
      slide = FALSE;
      clear_notekey_leds();
    }
  }
}
