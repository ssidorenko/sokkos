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

void clock_leds(void);

#define LED_LATCH_PORT PORTA
#define LED_LATCH_PIN  7

#define MAX_LED 40

void clear_led(uint8_t ledno);
void set_led(uint8_t ledno);
void set_led_blink(uint8_t ledno);
void clear_led_blink(uint8_t ledno);

int is_led_set(uint8_t ledno);
uint8_t is_led_blink(uint8_t num);

void set_bank_led(uint8_t num);
void set_bank_led_blink(uint8_t num);
void clear_bank_leds(void);
uint8_t is_bank_led_blink(uint8_t num);
uint8_t is_bank_led_set(uint8_t num);

void set_key_led(uint8_t num);
void set_key_led_blink(uint8_t num);
void clear_key_leds(void);
void clear_key_led(uint8_t num);

void set_numkey_led(uint8_t num);
void set_single_numkey_led(uint8_t num);
void set_numkey_led_blink(uint8_t num);
void clear_numkey_leds(void);
void clear_numkey_led(uint8_t num);
uint8_t is_numkey_led_blink(uint8_t num);
uint8_t is_numkey_led_set(uint8_t num);

void set_notekey_led(uint8_t note);
void set_notekey_led_blink(uint8_t num);
void clear_notekey_led(uint8_t num);
void clear_notekey_leds(void);
uint8_t is_notekey_led_blink(uint8_t num);

void set_note_led(uint8_t note);
void clear_note_leds(void);

void clock_leds(void);
void blink_leds_on(void);
void blink_leds_off(void);
void clear_all_leds(void);
void clear_blinking_leds(void);

void display_octave_shift(int8_t shift);

#define LED_NEXT 36
#define LED_RS 33
#define LED_CHAIN 34
#define LED_PREV 35
#define LED_C 32
#define LED_CS 27
#define LED_D 25
#define LED_DS 28
#define LED_E 26
#define LED_F 24
#define LED_FS 15
#define LED_G 12
#define LED_GS 14
#define LED_A 11
#define LED_AS 13
#define LED_B 10
#define LED_C2 9
#define LED_UP 3
#define LED_DOWN 4
#define LED_REST 8
#define LED_ACCENT 1
#define LED_SLIDE 2
#define LED_DONE 0
#define LED_TEMPO 37

#define LED_BANK1 38
#define LED_BANK2 39
#define LED_BANK3 29
#define LED_BANK4 30
#define LED_BANK5 31
#define LED_BANK6  16
#define LED_BANK7 18
#define LED_BANK8 17
#define LED_BANK9 19
#define LED_BANK10 20
#define LED_BANK11 21
#define LED_BANK12 22
#define LED_BANK13 23
#define LED_BANK14 5
#define LED_BANK15 6
#define LED_BANK16 7

