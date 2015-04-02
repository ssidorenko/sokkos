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
#include <stdio.h>
#include "synth.h"
#include "main.h"

/* Pitch shift variables:
 * These define the 'current' pitch shift (for the currently playing pattern)
 * and the 'next' pitch shift, which will take effect on the next loop 
 * A pitch shift of 0 means no shift, -12 is octave down, 12 is octave up, etc.
 */ 
int8_t curr_pitch_shift = 0;
int8_t next_pitch_shift = 0;

/* Note On:
 * This function takes a 6 bit 'note' (0x0 thru 0x3F), one bit of slide
 * and one bit of accent and performs the proper low level gating. Changing
 * this function can affect how the synth sounds.
 * Note that accent is active low.
 */
void note_on(uint8_t note, uint8_t slide, uint8_t accent) {
  uint8_t i = 0;
  cbi(NOTELATCH_PORT, NOTELATCH_PIN);

  // Do not allow the note to go higher than the highest note (0x3F)
  if (note > 0x3F)
    note = 0x3F;

  // Basically turn slide and accent flags into bit flags for the note port
  if (slide != 0) 
    slide = 0x40;

  if (accent == 0)
    accent = 0x80;
  else
    accent = 0;

  // output the note, set the latch, and strike the gate
  if (note != REST) {
    NOTE_PORT = note | slide | accent;
    // 30ns setup time?
    sbi(NOTELATCH_PORT, NOTELATCH_PIN);
    // 10 uS
    while (i<40) {
      i++;
    }
    sbi(GATE_PORT, GATE_PIN);
  }
  else {
    // gate is not restruck during rest, and note is not latched, but one can
    // slide to/from a rest and rests can have accent (tip to memology)
    NOTE_PORT = slide | accent;
  }

  // Debugging: print out notes as they are played
  /*
    putstring("Note on: 0x");
    putnum_uh(note); uart_putchar(' ');
    if (accent == 0) {
    putstring(" w/accent ");
    }
    if (slide) {
     putstring(" w/slide ");
     }
     putstring("\n\r");
  */

}

/* Note off:
 * This is essentially used to reset the gate/latch pins and also
 * deals with the pecularities of sliding (gate is not reset on slide).
 */
void note_off(uint8_t slide) {
  /*
  putstring("Note off\n\r");  
  */

  if (slide) {
    sbi(NOTE_PORT, 6);
  } else {
    cbi(GATE_PORT, GATE_PIN);
  }

  cbi(NOTELATCH_PORT, NOTELATCH_PIN);
}
