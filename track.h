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

#define TRACK_SIZE 16
#define TRACK_MEM 0x800UL
#define END_OF_TRACK 0xFFFFUL

void do_track_edit(void);

void start_track_stepwrite_mode(void);
void stop_track_stepwrite_mode(void);
void stop_track_run_mode(void);
void start_track_run_mode(void);
void load_track(uint8_t bank, uint8_t track_loc);
void write_track(uint8_t bank, uint8_t track_loc);

void display_curr_pitch_shift_ud(void);
int8_t get_pitchshift_from_patt(uint16_t patt);

uint8_t load_curr_patt(void);

#define TRACK_REST_FLAG    0x8000UL
#define TRACK_ACCENT_FLAG  0x4000UL
#define TRACK_SLIDE_FLAG   0x2000UL
