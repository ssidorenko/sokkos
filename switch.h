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

#define TEMPO_PIN PINA
#define TEMPO_A 1
#define TEMPO_B 0
//#define TEMPO_SW 0
#define SWITCH_LATCH_PORT PORTB
#define SWITCH_LATCH_PIN 0

#define BANK_PIN PINA
#define FUNC_PIN PINA
#define BANK_COMMON_PORT PORTA
#define FUNC_COMMON_PORT PORTA
#define BANK_COMMON_PIN 6
#define FUNC_COMMON_PIN 6
#define BANK_PIN1 5
#define BANK_PIN2 4
#define BANK_PIN4 3
#define BANK_PIN8 2

uint8_t read_tempo(void);
void read_switches(void);
void select_bank_read(void);
uint8_t read_bank(void);
uint8_t has_bank_knob_changed(void);
void read_keypad(uint8_t *switchinput);
void select_func_read(void);
uint8_t read_function(void);
void read_switch_press(void);
uint8_t is_pressed(uint8_t key);
uint8_t just_pressed(uint8_t key);
uint8_t just_released(uint8_t key);
uint8_t get_lowest_numkey_pressed(void);
int8_t get_lowest_notekey_pressed(void);
uint8_t get_lowest_numkey_just_pressed(void);
uint8_t get_lowest_loopkey_just_pressed(void);
uint8_t get_lowest_notekey_just_pressed(void);
uint8_t no_keys_pressed(void);

#define KEY_TEMPO 2

#define KEY_NEXT 8
#define KEY_RS 1
#define KEY_CHAIN 0
#define KEY_PREV 3
#define KEY_C 4
#define KEY_CS 9
#define KEY_D 5
#define KEY_DS 10
#define KEY_E 6
#define KEY_F 7
#define KEY_FS 11
#define KEY_G 17
#define KEY_GS 12
#define KEY_A 18
#define KEY_AS 13
#define KEY_B 16
#define KEY_C2 19
#define KEY_UP 15
#define KEY_DOWN 14
#define KEY_REST 20
#define KEY_ACCENT 21
#define KEY_SLIDE 22
#define KEY_DONE 23

#define KEY_1 KEY_C
#define KEY_2 KEY_D
#define KEY_3 KEY_E
#define KEY_4 KEY_F
#define KEY_5 KEY_G
#define KEY_6 KEY_A
#define KEY_7 KEY_B
#define KEY_8 KEY_C2

#define PLAY_PATTERN_FUNC 15
#define PLAY_PATTERN_MIDISYNC_FUNC 11
#define PLAY_PATTERN_DINSYNC_FUNC 7
#define EDIT_PATTERN_FUNC 3

#define PLAY_TRACK_FUNC 13
#define PLAY_TRACK_DINSYNC_FUNC 5
#define PLAY_TRACK_MIDISYNC_FUNC 9
#define EDIT_TRACK_FUNC 1

#define COMPUTER_CONTROL_FUNC 14

#define MIDI_CONTROL_FUNC 0

#define RANDOM_MODE_FUNC 8

#define KEYBOARD_MODE_FUNC 4

#define A_FUNC 12
#define B_FUNC 2
#define C_FUNC 10

#define BOOTLOAD_FUNC 6

