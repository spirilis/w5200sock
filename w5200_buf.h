/* Wiznet W5200 Buffer I/O
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

#ifndef W5200_BUF_H
#define W5200_BUF_H

/* High-level Buffer I/O */
void wiznet_w_txbuf(int, uint16_t, void *);
void wiznet_fill_txbuf(int, uint16_t, uint8_t);

uint16_t wiznet_recvsize(int);
void wiznet_r_rxbuf(int, uint16_t, void *, uint8_t);
void wiznet_peek_rxbuf(int, uint16_t, uint16_t, void *);
void wiznet_flush_rxbuf(int, uint16_t, uint8_t);
uint16_t wiznet_search_r_rxbuf(int, uint16_t, void *, uint8_t, uint8_t);
uint16_t wiznet_read_virtual_fsr(int);
#define wiznet_read_virtual_tsz(sock) (W52_SOCK_MEM_SIZE - wiznet_read_virtual_fsr(sock))

/* IP address binary/string conversion and I/O */
int wiznet_ip2binary(const void *, uint16_t *);
int wiznet_binary2ip(const uint16_t *, void *);
int wiznet_mac2binary(const void *, uint16_t *);
int wiznet_binary2mac(const uint16_t *, void *);

// IP address register & buffer I/O stack
void wiznet_ip_bin_w_reg(uint16_t, const uint16_t *);
#define wiznet_ip_bin_w_sockreg(sock, addr, ptr) wiznet_ip_bin_w_reg(W52_SOCK_REG_RESOLVE(sock, addr), ptr)
void wiznet_ip_bin_r_reg(uint16_t, uint16_t *);
#define wiznet_ip_bin_r_sockreg(sock, addr, ptr) wiznet_ip_bin_r_reg(W52_SOCK_REG_RESOLVE(sock, addr), ptr)

int wiznet_ip_str_w_reg(uint16_t, const void *);
#define wiznet_ip_str_w_sockreg(sock, addr, ptr) wiznet_ip_str_w_reg(W52_SOCK_REG_RESOLVE(sock, addr), ptr)
int wiznet_ip_str_r_reg(uint16_t, void *);
#define wiznet_ip_str_r_sockreg(sock, addr, ptr) wiznet_ip_str_r_reg(W52_SOCK_REG_RESOLVE(sock, addr), ptr)

void wiznet_ip_bin_w_txbuf(int, const uint16_t *);
void wiznet_ip_bin_r_rxbuf(int, uint16_t, uint8_t, uint16_t *);  // Read RX buffer, with optional offset & optional RECV call
void wiznet_ip_str_w_txbuf(int, const uint16_t *);
int wiznet_ip_str_r_rxbuf(int, uint16_t, uint8_t, void *);

// MAC address register & buffer I/O stack
void wiznet_mac_bin_w_reg(uint16_t, const uint16_t *);
#define wiznet_mac_bin_w_sockreg(sock, addr, ptr) wiznet_mac_bin_w_reg(W52_SOCK_REG_RESOLVE(sock, addr), ptr)
void wiznet_mac_bin_r_reg(uint16_t, uint16_t *);
#define wiznet_mac_bin_r_sockreg(sock, addr, ptr) wiznet_mac_bin_r_reg(W52_SOCK_REG_RESOLVE(sock, addr), ptr)

int wiznet_mac_str_w_reg(uint16_t, const void *);
#define wiznet_mac_str_w_sockreg(sock, addr, ptr) wiznet_mac_str_w_reg(W52_SOCK_REG_RESOLVE(sock, addr), ptr)
int wiznet_mac_str_r_reg(uint16_t, void *);
#define wiznet_mac_str_r_sockreg(sock, addr, ptr) wiznet_mac_str_r_reg(W52_SOCK_REG_RESOLVE(sock, addr), ptr)

void wiznet_mac_bin_w_txbuf(int, const uint16_t *);
void wiznet_mac_bin_r_rxbuf(int, uint16_t, uint8_t, uint16_t *);  // Read RX buffer, with optional offset & optional RECV call
void wiznet_mac_str_w_txbuf(int, const uint16_t *);
int wiznet_mac_str_r_rxbuf(int, uint16_t, uint8_t, void *);

/* Port endianness */
void wiznet_htons(uint16_t, uint8_t *);
uint16_t wiznet_ntohs(uint8_t *);


#endif
