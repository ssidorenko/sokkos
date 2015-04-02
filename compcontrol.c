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

#include <inttypes.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/signal.h>
#include "pattern.h"
#include "switch.h"
#include "led.h"
#include "main.h"
#include "compcontrol.h"
#include "eeprom.h"

extern uint8_t pattern_buff[PATT_SIZE]; /* removed volatile */

extern uint8_t function, bank, switches[3];
extern uint16_t tempo;

#define function_changed (function != COMPUTER_CONTROL_FUNC)


#define UART_BUFF_SIZE 64
uint8_t recv_msg_buff[UART_BUFF_SIZE];
uint8_t tx_msg_buff[UART_BUFF_SIZE];
uint8_t recv_msg_i=0;
uint8_t valid_msg_in_q = 0;

volatile uint8_t CTS = TRUE; // clear to send -- can receive data on uart
volatile uint16_t uart_timeout = 0; // timeout for messages

/* 
 * Taken from: 
 * http://cell-relay.indiana.edu/mhonarc/cell-relay/1999-Jan/msg00074.html
 * 8 bit CRC Generator, MSB shifted first
 * Polynom: x^8 + x^2 + x^1 + 1
 */
 /*
const unsigned char CRC8Table[256] = {
    0x00,0x07,0x0E,0x09,0x1C,0x1B,0x12,0x15,
    0x38,0x3F,0x36,0x31,0x24,0x23,0x2A,0x2D,
    0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,
    0x48,0x4F,0x46,0x41,0x54,0x53,0x5A,0x5D,
    0xE0,0xE7,0xEE,0xE9,0xFC,0xFB,0xF2,0xF5,
    0xD8,0xDF,0xD6,0xD1,0xC4,0xC3,0xCA,0xCD,
    0x90,0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,
    0xA8,0xAF,0xA6,0xA1,0xB4,0xB3,0xBA,0xBD,
    0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,
    0xFF,0xF8,0xF1,0xF6,0xE3,0xE4,0xED,0xEA,
    0xB7,0xB0,0xB9,0xBE,0xAB,0xAC,0xA5,0xA2,
    0x8F,0x88,0x81,0x86,0x93,0x94,0x9D,0x9A,
    0x27,0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,
    0x1F,0x18,0x11,0x16,0x03,0x04,0x0D,0x0A,
    0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,
    0x6F,0x68,0x61,0x66,0x73,0x74,0x7D,0x7A,
    0x89,0x8E,0x87,0x80,0x95,0x92,0x9B,0x9C,
    0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,
    0xF9,0xFE,0xF7,0xF0,0xE5,0xE2,0xEB,0xEC,
    0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,0xD3,0xD4,
    0x69,0x6E,0x67,0x60,0x75,0x72,0x7B,0x7C,
    0x51,0x56,0x5F,0x58,0x4D,0x4A,0x43,0x44,
    0x19,0x1E,0x17,0x10,0x05,0x02,0x0B,0x0C,
    0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,
    0x4E,0x49,0x40,0x47,0x52,0x55,0x5C,0x5B,
    0x76,0x71,0x78,0x7F,0x6A,0x6D,0x64,0x63,
    0x3E,0x39,0x30,0x37,0x22,0x25,0x2C,0x2B,
    0x06,0x01,0x08,0x0F,0x1A,0x1D,0x14,0x13,
    0xAE,0xA9,0xA0,0xA7,0xB2,0xB5,0xBC,0xBB,
    0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,
    0xDE,0xD9,0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,
    0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,0xF4,0xF3
};
*/

// interrupt on receive char
SIGNAL(SIG_USART1_RECV) {
  uint8_t cmd, crc;
  uint16_t size;
  char c = UDR1;

  if (CTS) {
    if (uart_timeout > 1000) {
      clear_bank_leds();
      clock_leds();
      recv_msg_i = 0;  // start over... but don't send status!
    }

    if (recv_msg_i < UART_BUFF_SIZE) {
      recv_msg_buff[recv_msg_i++] = c;    // place at end of q      
    } else {
      // Receive failure.  Start over.  

      // Meme:  Perhaps this should be a counter timeout rather than an
      // overflow timeout?   -mbroxton

      send_status(recv_msg_i);
      recv_msg_i = 0;
      //set_bank_led(14); clock_leds();
    }

    uart_timeout = 0;
    
    /* The header has been received.  Start grabbing the content
     * and the CRC. */
    if (recv_msg_i >= 3) {
      cmd = recv_msg_buff[0];
      size = recv_msg_buff[1];
      size <<= 8;               // size is just the body size
      size |= recv_msg_buff[2];

      if (recv_msg_i >= 4 + size) { // header+foot is 4 bytes long
	crc = recv_msg_buff[3+size]; // CRC is the last byte of the packet

	if (crc != calc_CRC8(recv_msg_buff, size+3)) {   
	  putnum_uh(calc_CRC8(recv_msg_buff, size+3));

	  recv_msg_i = 0;
	  send_status(0);
	  // set_bank_led(13); clock_leds();   // CRC Error
	  return;
	}

	/* If we get to here, the message has passed the CRC and is
	 * assumed to be valid.  Now we process the message.
	 */

	switch (cmd) {
	case PING_MSG:
	  send_status(0x1);
	  break;
	  
	case GET_TEMPO_MSG:
	  send_tempo(tempo);
	  break;

	case SET_TEMPO_MSG: {
	  uint16_t t;

	  if (recv_msg_buff[2] != TEMPO_MSG_LEN) {
	    send_status(0);
	    break;
	  }
	  t = recv_msg_buff[3];
	  t <<= 8;
	  t += recv_msg_buff[4];

	  change_tempo(t);

	  break;
	}	  
	case RD_PATT_MSG: {
	  uint8_t bank, patt, i;
	  uint16_t addr;
	
	  if (recv_msg_buff[2] != RD_PATT_MSG_LEN) {
	    send_status(0);
	    break;
	  }

	  bank = recv_msg_buff[3];
	  patt = recv_msg_buff[4];
	  addr = PATTERN_MEM + bank*BANK_SIZE + patt*PATT_SIZE;
	  /*
	    putstring("reading patt ["); 
	    putnum_ud(bank); putstring(", "); putnum_ud(patt_location);
	    putstring(" @ 0x");
	    putnum_uh(pattern_addr);
	    putstring("\n\r");
	  */
	  tx_msg_buff[0] = PATT_MSG;
	  tx_msg_buff[1] = 0;
	  tx_msg_buff[2] = PATT_MSG_LEN;

	  for(i=0; i<PATT_SIZE; i++) {
	    tx_msg_buff[3+i] = spieeprom_read(addr + i);
	    //putstring(" 0x"); putnum_uh(tx_msg_buff[1+i]);
	  }
	  //putstring("\n\r");
	  
	  tx_msg_buff[3+PATT_SIZE] = calc_CRC8(tx_msg_buff, 3+PATT_SIZE);
  
	  send_msg(tx_msg_buff, 4+PATT_SIZE);
	  break;
	}
 
	case WR_PATT_MSG: {
	  uint8_t bank, patt, i;
	  uint16_t addr;

	  //set_bank_led(4); clock_leds();
	  if (recv_msg_buff[2] != WR_PATT_MSG_LEN) {
	    send_status(0);
	    break;
	  }

	  bank = recv_msg_buff[3];
	  patt = recv_msg_buff[4];
	  addr = PATTERN_MEM + bank*BANK_SIZE + patt*PATT_SIZE;
	  /*
	    putstring("writing patt ["); 
	    putnum_ud(bank); putstring(", "); putnum_ud(patt_location);
	    putstring(" @ 0x");
	    putnum_uh(pattern_addr);
	    putstring("\n\r");
	  */
	  for(i=0; i<PATT_SIZE; i++) {
	    spieeprom_write(recv_msg_buff[5+i], addr + i);
	    //putstring(" 0x"); putnum_uh(tx_msg_buff[1+i]);
	  }
	  //putstring("\n\r");
	  
	  send_status(1);
	  break;
	}

	default:
	  send_status(0);
	  break;


	}
	recv_msg_i = 0; // start over!
      }
    }
    
  }
}

void send_msg(uint8_t *buff, uint16_t len) {
  uint16_t i;
  for (i=0; i<len; i++) {
    uart_putchar(buff[i]);
  }
}

void send_status(uint8_t stat) {
  tx_msg_buff[0] = STATUS_MSG;
  tx_msg_buff[1] = 0;
  tx_msg_buff[2] = 1;
  tx_msg_buff[3] = stat;
  tx_msg_buff[4] = calc_CRC8(tx_msg_buff, 4);
  
  send_msg(tx_msg_buff, 5);
}

void send_tempo(uint16_t t) {

  tx_msg_buff[0] = TEMPO_MSG;
  tx_msg_buff[1] = 0;
  tx_msg_buff[2] = TEMPO_MSG_LEN;
  tx_msg_buff[3] = t >> 8;
  tx_msg_buff[4] = t & 0xFF;
  tx_msg_buff[5] = calc_CRC8(tx_msg_buff, 5);

  send_msg(tx_msg_buff, TEMPO_MSG_LEN + 4);
}


void do_computer_control(void) {

  while (1) {
    read_switches();
    if (function_changed) {
      // oops i guess they want something else, return!
      clear_all_leds();
      clock_leds();
      return;
    }
    
    //putstring("computer kontrol\n\r");
    
  }
}

     
/* 
 * Adapted from: http://cell-relay.indiana.edu/mhonarc/cell-relay/1999-Jan/msg00074.html
 * 8 bit CRC Generator, MSB shifted first
 * Polynom: x^8 + x^2 + x^1 + 1
 * 
 * Calculates an 8-bit cyclic redundancy check sum for a packet.
 * This function takes care not to include the packet's check sum in calculating 
 * the check sum.  Assumes the CRC is the last byte of the packet header.  Also
 * takes care to look in code space (instead of xdata space) when dealing with a 
 * PFrag.
 */


uint8_t calc_CRC8(uint8_t *buff, uint16_t size) {
  uint8_t x;
  uint8_t crc = 0;

/*
  uint8_t i;
  uint8_t crc = 0;
*/
  /*
   * Add the message header to the CRC.  Don't include the CRC itself
   * when calculating the CRC.
   */
/*   
  for (i=0; i < size; i++) {	
    crc = CRC8Table[crc ^ *(buff + i)];
  }
*/  
/*  
  for (i = 0; i < size; i++)
  {
    crc^=*(buff + i);
    n=8;while(n--) crc=(crc&0x80)?(crc<<1)^0x07:crc<<1; 
  }
*/

  while(size) {
    --size;
	x=crc^*buff++;
	crc=0;
	if(x&0x01) crc^=0x07;
	if(x&0x02) crc^=0x0E;
	if(x&0x04) crc^=0x1C;
	if(x&0x08) crc^=0x38;
	if(x&0x10) crc^=0x70;
	if(x&0x20) crc^=0xE0;
	if(x&0x40) crc^=0xC7;
	if(x&0x80) crc^=0x89;
  }
  
  return crc;
}

