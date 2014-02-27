/* Wiznet W5200 BSD Sockets Interface
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

#ifndef W5200_SOCK_H
#define W5200_SOCK_H

/* Const IP and MAC address values */
extern const uint16_t w52_const_subnet_classA[2];
extern const uint16_t w52_const_subnet_classB[2];
extern const uint16_t w52_const_subnet_classC[2];
extern const uint16_t w52_const_ip_default[2];
extern const uint16_t w52_const_mac_default[3];

extern const char *wiznet_tcp_state[10];
extern const uint8_t wiznet_tcp_state_idx[10];

/* Functions */
int wiznet_irq_getsocket();
#define wiznet_w_command(sock, cmdval) wiznet_w_sockreg(sock, W52_SOCK_CR, cmdval)
int wiznet_phystate();

int wiznet_socket(int);
int wiznet_close(int);
int wiznet_connect(int, uint16_t *, uint16_t);
int wiznet_quickbind(int);
int wiznet_bind(int, uint16_t);
int wiznet_accept(int);
int wiznet_recv(int, void *, uint16_t, uint8_t);
int wiznet_search_recv(int, void *, uint16_t, uint8_t, uint8_t);
int wiznet_peek(int, uint16_t, void *, uint16_t);
int wiznet_flush(int, uint16_t, uint8_t);
int wiznet_recvfrom(int, void *, uint16_t, uint16_t *, uint16_t *, uint8_t);
int wiznet_txcommit(int);
int wiznet_send(int, void *, uint16_t, uint8_t);
int wiznet_sendto(int, void *, uint16_t, uint16_t *, uint16_t, uint8_t);

// Ethernet MACRAW I/O
int wiznet_mac_recvfrom(void *, uint16_t, uint16_t *, uint16_t *, uint16_t *, uint8_t, uint8_t);
int wiznet_mac_sendto(void *, uint16_t, uint16_t *, uint16_t, uint16_t, uint8_t, uint8_t);

int wiznet_init();

#endif
