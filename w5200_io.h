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

#ifndef W5200_IO_H
#define W5200_IO_H

/* Architecture-specific Init */
void wiznet_io_init();

/* Register I/O primitives */
void wiznet_w_reg(uint16_t, uint8_t);
uint8_t wiznet_r_reg(uint16_t);
void wiznet_w_reg16(uint16_t, uint16_t);
uint16_t wiznet_r_reg16(uint16_t);
void wiznet_w_set(uint16_t, uint16_t, uint8_t);
#define wiznet_w_sockreg(sock, addr, val) wiznet_w_reg(W52_SOCK_REG_RESOLVE(sock, addr), val)
#define wiznet_r_sockreg(sock, addr) wiznet_r_reg(W52_SOCK_REG_RESOLVE(sock, addr))
#define wiznet_w_sockreg16(sock, addr, val) wiznet_w_reg16(W52_SOCK_REG_RESOLVE(sock, addr), val)
#define wiznet_r_sockreg16(sock, addr) wiznet_r_reg16(W52_SOCK_REG_RESOLVE(sock, addr))
#define wiznet_w_sockset(sock, addr, len, val) wiznet_w_set(W52_SOCK_REG_RESOLVE(sock, addr), len, val)

/* Low-level Buffer I/O (belongs here, not w5200_buf.h) */
void wiznet_w_buf(uint16_t, uint16_t, void *);
void wiznet_r_buf(uint16_t, uint16_t, void *);
uint16_t wiznet_search_r_buf(uint16_t, uint16_t, void *, uint8_t);



#endif
