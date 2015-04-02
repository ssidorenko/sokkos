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

#define MAX_TEMPO 300UL
#define MIN_TEMPO 20UL

#define NOP asm("nop")

#define CLK_CALIBRATION_EEADDR 0x1
#define MIDIIN_ADDR_EEADDR 0x3
#define MIDIOUT_ADDR_EEADDR 0x4
#define TEMPO_EEADDR 0x5  // 2 bytes
#define SETTINGS_EEADDR 0x7 // 1 byte currently, 0..7: 2*UP, SAVE TEMPO, SAVE NEW = CHANGE, 303-MODE(not yet...)

#define BOOTLOADER_ADDR 0x3E00UL // 0x1F00 word, 15872 byte

#define INTERNAL_SYNC 0
#define DIN_SYNC 1
#define MIDI_SYNC 2
#define NO_SYNC 3         // for like, keyboard mode & stuff?

#define FALSE 0
#define TRUE 1

#ifndef sbi
#define sbi(p,b) (p) |= (1<<(b))
#endif

#ifndef cbi
#define cbi(p,b) (p) &= ~(1<<(b))
#endif

#define ANYPATTERNPLAYFUNC ((function == PLAY_PATTERN_FUNC) || (function == PLAY_PATTERN_MIDISYNC_FUNC) || (function ==PLAY_PATTERN_DINSYNC_FUNC))
#define ANYTRACKPLAYFUNC ((function == PLAY_TRACK_FUNC) || (function == PLAY_TRACK_MIDISYNC_FUNC) || (function == PLAY_TRACK_DINSYNC_FUNC))

/************* function prototypes */
void ioinit(void);
void putstring(char *str);
void putnum_uh(uint16_t n);
void putnum_ud(uint16_t n);
int uart_putchar(char c);
int uart_getchar(void);
int uart_getch(void);
void change_tempo(uint16_t newtempo);

void step(void);
void halt(void);
uint8_t random(void);

void turn_off_tempo(void);
void turn_on_tempo(void);
uint8_t is_tempo_running(void);
void init_tempo_detect(void);
void init_tempo(void);
void init_timer0(void);
void init_timer2(void);

void do_tempo(void);

uint8_t internal_eeprom_read8(uint16_t addr);
void internal_eeprom_write8(uint16_t addr, uint8_t data);
