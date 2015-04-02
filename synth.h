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

#define NOTELATCH_PORT PORTE
#define NOTELATCH_PIN 2
#define NOTE_PORT PORTC
#define NOTE_PIN PINC
#define GATE_PIN 1
#define GATE_PORT PORTE

// one octave is 12 notes
#define OCTAVE 0xC

void note_on(uint8_t note, uint8_t slide, uint8_t accent);
void note_off(uint8_t slide);
uint8_t is_playing (uint8_t note);

#define REST 0x0

/* between 0x0 and 0xA, the VCO voltage pins, so these notes arent really
 * 'effective' in that they all sound the same. 
 */

// lowest octave
#define C1 0x0B
#define C1_SHARP 0x0C
#define D1 0x0D
#define D1_SHARP  0x0E
#define E1 0x0F
#define F1 0x10
#define F1_SHARP 0x11
#define G1 0x12
#define G1_SHARP 0x13

// middle octave
#define A2 0x14
#define A2_SHARP 0x15
#define B2 0x16
#define C2 0x17
#define C2_SHARP 0x18
#define D2 0x19
#define D2_SHARP  0x1A
#define E2 0x1B
#define F2 0x1C
#define F2_SHARP 0x1D
#define G2 0x1E
#define G2_SHARP 0x1F

// high octave
#define A3 0x20
#define A3_SHARP 0x21
#define B3 0x22
#define C3 0x23
#define C3_SHARP 0x24
#define D3 0x25
#define D3_SHARP 0x26
#define E3 0x27
#define F3 0x28
#define F3_SHARP 0x29
#define G3 0x2A
#define G3_SHARP 0x2B

#define A4 0x2C
#define A4_SHARP 0x2D
#define B4 0x2E
#define C4 0x2F
#define C4_SHARP 0x30
#define D4 0x31
#define D4_SHARP 0x32
#define E4 0x33
#define F4 0x34
#define F4_SHARP 0x35
#define G4 0x36
#define G4_SHARP 0x37

#define A5 0x38
#define A5_SHARP 0x39
#define B5 0x3A
#define C5 0x3B
#define C5_SHARP 0x3C
#define D5 0x3D
#define D5_SHARP 0x3E
// no more notes!
