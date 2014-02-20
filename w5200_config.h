/* w5200_config.h
 * WizNet W5200 Ethernet Controller Driver
 * TI MSP430 Edition
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

#ifndef W5200_CONFIG_H
#define W5200_CONFIG_H

#include <msp430.h>
#include <stdint.h>
#include "w5200_regs.h"

/* User configuration - SPI driver, CS pin */

// For USCI-based SPI drivers - leave only 1 uncommented
#define SPI_DRIVER_USCI_B
//#define SPI_DRIVER_USCI_A

// Chip Select pin
#define W52_CHIPSELECT_PORTDIR P6DIR
#define W52_CHIPSELECT_PORTBIT BIT5
#define W52_CHIPSELECT_PORTOUT P6OUT

// IRQ pin
#define W52_IRQ_PORTBIT BIT2
#define W52_IRQ_PORTDIR P2DIR
#define W52_IRQ_PORTREN P2REN
#define W52_IRQ_PORTOUT P2OUT
#define W52_IRQ_INTERRUPT_LEVEL P2IES
#define W52_IRQ_INTERRUPT_ENABLE P2IE
#define W52_IRQ_INTERRUPT_FLAGS P2IFG

// RESET pin
#define W52_RESET_PORTBIT BIT6
#define W52_RESET_PORTDIR P6DIR
#define W52_RESET_PORTOUT P6OUT

/* Custom statements used to temporarily set SPI speed (and restore afterwards)
 * The W5200 supports up to 33MHz SPI guaranteed (80MHz if you can manage the crosstalk).
 * It is advantageous to set SMCLK to the full speed of the DCO if possible.
 * Use this to minimize the SPI bitrate divider and restore it as needed (in case other
 * attached devices require lower SPI speeds)
 */
#define W52_SPI_SET ;
#define W52_SPI_UNSET ;

/* Base port used as source-port for outbound TCP connections */
#define W52_TCP_SRCPORT_BASE 40000

/* Method used to wait between issuing an operation and waiting for IRQ */
#define WIZNET_CPU_WAIT LPM0

/* Use pedantic checking (i.e. verify socket file descriptor is within allowed range,
 * verify PHYSTATUS link is up when performing operations, etc)
 * Comment this out to disable.
 */
#define W52_PEDANTIC_CHECKING 1

/* Socket memory buffer sizes
 * Defaults to 2KB, should be kept here unless you lower the # of max sockets.
 */
#define W52_SOCK_MEM_SIZE 2048
#define W52_SOCK_MEM_MASK 2047

/* End user configuration */

/* IRQ handler flag updated by user's Interrupt Service Routine for the IRQ pin */
extern volatile uint8_t w5200_irq;

// Macros for manipulating the SPI chip select line */
#define W52_CS_LOW W52_CHIPSELECT_PORTOUT &= ~W52_CHIPSELECT_PORTBIT
#define W52_CS_HIGH W52_CHIPSELECT_PORTOUT |= W52_CHIPSELECT_PORTBIT

// Structure for holding socket information
#define W52_MAX_SOCKETS 8

typedef struct {
	uint8_t mode;
	uint8_t srcport_idx;
	uint8_t is_bind;
	uint16_t tx_wr;
	uint16_t rx_rd;
} WIZNETSocketState;

extern WIZNETSocketState w52_sockets[W52_MAX_SOCKETS];

/* Relevant ERRNO values */
#define ENETDOWN 100
#define EBADF 9
#define EFAULT 14
#define EPROTONOSUPPORT 93
#define ENFILE 23

#define EADDRINUSE 98

#define ESHUTDOWN 108
#define ENOTCONN 107
#define ECONNREFUSED 111
#define ECONNABORTED 103
#define ECONNRESET 104
#define EHOSTDOWN 112
#define EHOSTUNREACH 113
#define ETIMEDOUT 110

#define EINPROGRESS 115
#define EISCONN 106
#define EAGAIN 11






#endif
