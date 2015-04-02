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
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "eeprom.h"
#include "main.h"

void spieeprom_write(uint8_t data, uint16_t addr) {
  uint8_t status;

  //printf("writing %x to 0x%x\n\r", data, addr);
  /* check if there is a write in progress, wait */
  cli();

  do {
    cbi(SPIEE_CS_PORT, SPIEE_CS_PIN); // pull CS low
    NOP; NOP; NOP; NOP;

    SPDR = SPI_EEPROM_RDSR;
    while (!(SPSR & (1<<SPIF)));
    NOP; NOP; NOP; NOP;
    SPDR = 0;
    while (!(SPSR & (1<<SPIF)));
    status = SPDR;
    sbi(SPIEE_CS_PORT, SPIEE_CS_PIN); // pull CS high
    //printf("status =  0x%x\n\r", status);
    NOP; NOP;NOP; NOP;
    
  } while ((status & 0x1) != 0);
  /* set the spi write enable latch */

  cbi(SPIEE_CS_PORT, SPIEE_CS_PIN); // pull CS low
  NOP; NOP;


  SPDR = SPI_EEPROM_WREN;           // send command
  while (!(SPSR & (1<<SPIF)));
  NOP; NOP;
  sbi(SPIEE_CS_PORT, SPIEE_CS_PIN); // pull CS low

  NOP; NOP; NOP; NOP;  // wait for write enable latch

  cbi(SPIEE_CS_PORT, SPIEE_CS_PIN); // pull CS low
  NOP; NOP;

  SPDR = SPI_EEPROM_WRITE;           // send command
  while (!(SPSR & (1<<SPIF)));

  SPDR = addr >> 8;                 // send high addr 
  while (!(SPSR & (1<<SPIF)));

  SPDR = addr & 0xFF;               // send low addr
  while (!(SPSR & (1<<SPIF)));

  SPDR = data;               // send data
  while (!(SPSR & (1<<SPIF)));

  NOP;
  NOP;

  sbi(SPIEE_CS_PORT, SPIEE_CS_PIN); // pull CS low  
  sei();
}

uint8_t spieeprom_read(uint16_t addr) {
  uint8_t data;

  cli();

  cbi(SPIEE_CS_PORT, SPIEE_CS_PIN); // pull CS low
  NOP; NOP;

  SPDR = SPI_EEPROM_READ;           // send command
  while (!(SPSR & (1<<SPIF)));

  SPDR = addr >> 8;                 // send high addr 
  while (!(SPSR & (1<<SPIF)));

  SPDR = addr & 0xFF;               // send low addr
  while (!(SPSR & (1<<SPIF)));
  NOP;
  NOP;

  SPDR = 0;
  while (!(SPSR & (1<<SPIF)));
  data = SPDR;
  //printf("got %x\n\r", data);

  sbi(SPIEE_CS_PORT, SPIEE_CS_PIN); // pull CS high
  sei();
  return data;
}
