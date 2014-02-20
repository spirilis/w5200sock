/* Wiznet W5200 Sys I/O driver for TI MSP430
 *
 *
 * Copyright (c) 2013, Eric Brundick <spirilis@linux.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <msp430.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "msp430_spi.h"
#include "w5200_config.h"


/* Register I/O primitives */

void wiznet_io_init()
{
	W52_CHIPSELECT_PORTOUT |= W52_CHIPSELECT_PORTBIT;
	W52_CHIPSELECT_PORTDIR |= W52_CHIPSELECT_PORTBIT;
	W52_IRQ_PORTOUT |= W52_IRQ_PORTBIT;
	W52_IRQ_PORTDIR &= ~W52_IRQ_PORTBIT;
	W52_IRQ_PORTREN |= W52_IRQ_PORTBIT;
	W52_IRQ_INTERRUPT_FLAGS &= ~W52_IRQ_PORTBIT;
	W52_IRQ_INTERRUPT_LEVEL |= W52_IRQ_PORTBIT;
	W52_IRQ_INTERRUPT_ENABLE |= W52_IRQ_PORTBIT;
	W52_RESET_PORTOUT &= ~W52_RESET_PORTBIT;
	W52_RESET_PORTDIR |= W52_RESET_PORTBIT;

	spi_init();
	_EINT();
}

void wiznet_w_reg(uint16_t addr, uint8_t val)
{
	W52_SPI_SET;
	W52_CS_LOW;
	spi_transfer(addr >> 8);
	spi_transfer(addr & 0xFF);
	spi_transfer(W52_SPI_OPCODE_WRITE >> 8);
	spi_transfer(0x01);
	spi_transfer(val);
	W52_CS_HIGH;
	W52_SPI_UNSET;
}

uint8_t wiznet_r_reg(uint16_t addr)
{
	uint8_t val;

	W52_SPI_SET;
	W52_CS_LOW;
	spi_transfer(addr >> 8);
	spi_transfer(addr & 0xFF);
	spi_transfer(W52_SPI_OPCODE_READ >> 8);
	spi_transfer(0x01);
	val = spi_transfer(0xFF);
	W52_CS_HIGH;
	W52_SPI_UNSET;
	return val;
}

void wiznet_w_reg16(uint16_t addr, uint16_t val)
{
	W52_SPI_SET;
	W52_CS_LOW;
	spi_transfer(addr >> 8);
	spi_transfer(addr & 0xFF);
	spi_transfer(W52_SPI_OPCODE_WRITE >> 8);
	spi_transfer(0x02);
	spi_transfer(val >> 8);
	spi_transfer(val & 0xFF);
	W52_CS_HIGH;
	W52_SPI_UNSET;
}

uint16_t wiznet_r_reg16(uint16_t addr)
{
	uint16_t val;

	W52_SPI_SET;
	W52_CS_LOW;
	spi_transfer(addr >> 8);
	spi_transfer(addr & 0xFF);
	spi_transfer(W52_SPI_OPCODE_READ >> 8);
	spi_transfer(0x02);
	val = spi_transfer(0xFF) << 8;
	val |= spi_transfer(0xFF);
	W52_CS_HIGH;
	W52_SPI_UNSET;
	return val;
}

void wiznet_w_set(uint16_t addr, uint16_t len, uint8_t val)
{
	uint16_t i;

	if (!len)
		return;
	W52_SPI_SET;
	W52_CS_LOW;
	spi_transfer(addr >> 8);
	spi_transfer(addr & 0xFF);
	spi_transfer( (W52_SPI_OPCODE_WRITE >> 8) | (len >> 8) );
	spi_transfer(len & 0xFF);
	for (i=0; i < len; i++) {
		spi_transfer(val);
	}
	W52_CS_HIGH;
	W52_SPI_UNSET;
}

void wiznet_w_buf(uint16_t addr, uint16_t len, void *buf)
{
        uint8_t *bufptr = (uint8_t *)buf;
        uint16_t i;

	if (!len)
		return;
        W52_SPI_SET;
        W52_CS_LOW;
        spi_transfer(addr >> 8);
        spi_transfer(addr & 0xFF);
        spi_transfer( (W52_SPI_OPCODE_WRITE >> 8) | (len >> 8) );
        spi_transfer(len & 0xFF);
        for (i=0; i < len; i++) {
                spi_transfer(bufptr[i]);
        }
        W52_CS_HIGH;
        W52_SPI_UNSET;
}

void wiznet_r_buf(uint16_t addr, uint16_t len, void *buf)
{
        uint8_t *bufptr = (uint8_t *)buf;
        uint16_t i;

	if (!len)
		return;
        W52_SPI_SET;
        W52_CS_LOW;
        spi_transfer(addr >> 8);
        spi_transfer(addr & 0xFF);
        spi_transfer( (W52_SPI_OPCODE_READ >> 8) | (len >> 8) );
        spi_transfer(len & 0xFF);
        for (i=0; i < len; i++) {
                bufptr[i] = spi_transfer(0xFF);
        }
        W52_CS_HIGH;
        W52_SPI_UNSET;
}
