/* 
 * WizNet W5200 Ethernet Controller Driver for MSP430
 * UARTCLI-library based register dump
 *
 *
 * Parts derived from Kevin Timmerman's tiny printf from 43oh.com
 * Link: http://forum.43oh.com/topic/1289-tiny-printf-c-version/
 *
 * Copyright (c) 2014, Eric Brundick <spirilis@linux.com>
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

#ifndef W5200_DEBUG_H
#define W5200_DEBUG_H

#include <msp430.h>
#include <stdint.h>
#include "w5200_config.h"
#include "w5200_io.h"


/* Debug levels:
 * 1 - Basic information from libraries
 * 2 - Verbose error reporting from libraries
 * 3 - Gratuitous information from libraries
 * 4 - Verbose detail from base socket library
 * 5 - Extraneous detail from base socket library
 * 6 - Low-level I/O dump from SPI transfer functions
 *
 * Each debug level includes information from all levels below.
 */
#define WIZNET_DEBUG 3

void wiznet_debug_init();
void wiznet_debug_printf(char *format, ...);
void wiznet_debug_dumpregs_main();
void wiznet_debug_dumpregs_sock(int);

void wiznet_debug_putc(unsigned int);
void wiznet_debug_puts(const char *);

#if WIZNET_DEBUG > 0
#define wiznet_debug1_printf(...) wiznet_debug_printf(__VA_ARGS__)
#else
#define wiznet_debug1_printf(...) ;
#endif

#if WIZNET_DEBUG > 1
#define wiznet_debug2_printf(...) wiznet_debug_printf(__VA_ARGS__)
#else
#define wiznet_debug2_printf(...) ;
#endif

#if WIZNET_DEBUG > 2
#define wiznet_debug3_printf(...) wiznet_debug_printf(__VA_ARGS__)
#else
#define wiznet_debug3_printf(...) ;
#endif

#if WIZNET_DEBUG > 3
#define wiznet_debug4_printf(...) wiznet_debug_printf(__VA_ARGS__)
#else
#define wiznet_debug4_printf(...) ;
#endif

#if WIZNET_DEBUG > 4
#define wiznet_debug5_printf(...) wiznet_debug_printf(__VA_ARGS__)
#else
#define wiznet_debug5_printf(...) ;
#endif

#if WIZNET_DEBUG > 5
#define wiznet_debug6_printf(...) wiznet_debug_printf(__VA_ARGS__)
#else
#define wiznet_debug6_printf(...) ;
#endif


#endif
