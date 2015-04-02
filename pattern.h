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

void do_pattern_edit(void);
void do_patterntrack_play(void);


void load_pattern(uint8_t bank, uint8_t patt_location);
void write_pattern(uint8_t bank, uint8_t patt_location);

void edit_pattern(void);
void edit_live(void);

void clock_ticks(void);

void start_runwrite_mode(void);
void stop_runwrite_mode(void);

void start_stepwrite_mode(void);
void stop_stepwrite_mode(void);

uint8_t chains_equiv(volatile uint8_t *chain1, volatile uint8_t *chain2);

uint8_t get_next_patt_idx(void);

#define NUM_BANKS 16
#define NUM_LOCS 8
#define BANK_SIZE (NUM_LOCS*PATT_SIZE)

#define PATT_SIZE 16 // bytes

#define PATTERN_MEM 0x0

#define MAX_CHAIN 32   // hell, make this 100 if you want.

#define END_OF_PATTERN 0xFF
#define END_OF_CHAIN 0xFF
