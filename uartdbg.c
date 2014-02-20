/* uartdbg.c
 * WizNet W5200 Ethernet Controller Driver for MSP430
 * UARTCLI-library based debug outputter
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
#include "uartcli.h"
#include <stdint.h>
#include "w5200_config.h"
#include "w5200_io.h"
#include "w5200_buf.h"
#include "w5200_sock.h"

void wiznet_debug_uart(int sockfd)
{
	uint8_t buf[29];

	wiznet_debug_dumpregs(-1, buf);  // Pull main registers
	uartcli_println_str("Main registers:");
	uartcli_print_str("Mode: 0x"); uartcli_printhex_byte(buf[0]); uartcli_println_str(" ");
	uartcli_print_str("Sys IRQ: 0x"); uartcli_printhex_byte(buf[1]); uartcli_println_str(" ");
	uartcli_print_str("Sock IRQ Mask: 0x"); uartcli_printhex_byte(buf[2]); uartcli_println_str(" ");
	uartcli_print_str("Retry Count: 0x"); uartcli_printhex_byte(buf[3]); uartcli_println_str(" ");
	uartcli_print_str("Sock IRQ Pending: 0x"); uartcli_printhex_byte(buf[5]); uartcli_println_str(" ");
	uartcli_print_str("PHY status: 0x"); uartcli_printhex_byte(buf[6]); uartcli_println_str(" ");
	uartcli_print_str("Sys IRQ Mask: 0x"); uartcli_printhex_byte(buf[7]); uartcli_println_str(" ");

	wiznet_debug_dumpregs(sockfd, buf);  // Pull intended socket params
	uartcli_print_str("Socket #"); uartcli_print_int(sockfd); uartcli_println_str(":");
	uartcli_print_str("Mode: 0x"); uartcli_printhex_byte(buf[0]); uartcli_println_str(" ");
	uartcli_print_str("IRQ: 0x"); uartcli_printhex_byte(buf[1]); uartcli_println_str(" ");
	uartcli_print_str("Status: 0x"); uartcli_printhex_byte(buf[2]); uartcli_println_str(" ");
	uartcli_print_str("Source port: 0x"); uartcli_printhex_word(*((uint16_t *)(buf+4))); uartcli_println_str(" ");
	uartcli_print_str("Dest IP: 0x"); uartcli_printhex_word(*((uint16_t *)(buf+6))); uartcli_printhex_word(*((uint16_t *)(buf+8))); uartcli_println_str(" ");
	uartcli_print_str("Dest port: 0x"); uartcli_printhex_word(*((uint16_t *)(buf+10))); uartcli_println_str(" ");
	uartcli_print_str("MSS: 0x"); uartcli_printhex_word(*((uint16_t *)(buf+12))); uartcli_println_str(" ");
	uartcli_print_str("IP TOS: 0x"); uartcli_printhex_byte(buf[14]); uartcli_println_str(" ");
	uartcli_print_str("IP TTL: 0x"); uartcli_printhex_byte(buf[15]); uartcli_println_str(" ");
	uartcli_print_str("TX buffer free: 0x"); uartcli_printhex_word(*((uint16_t *)(buf+16))); uartcli_println_str(" ");
	uartcli_print_str("TX readptr: 0x"); uartcli_printhex_word(*((uint16_t *)(buf+18))); uartcli_println_str(" ");
	uartcli_print_str("TX writeptr: 0x"); uartcli_printhex_word(*((uint16_t *)(buf+20))); uartcli_println_str(" ");
	uartcli_print_str("RX recvsize: 0x"); uartcli_printhex_word(*((uint16_t *)(buf+22))); uartcli_println_str(" ");
	uartcli_print_str("RX readptr: 0x"); uartcli_printhex_word(*((uint16_t *)(buf+24))); uartcli_println_str(" ");
	uartcli_print_str("RX writeptr: 0x"); uartcli_printhex_word(*((uint16_t *)(buf+26))); uartcli_println_str(" ");
	uartcli_print_str("IRQ Mask: 0x"); uartcli_printhex_byte(buf[28]); uartcli_println_str(" ");
}
