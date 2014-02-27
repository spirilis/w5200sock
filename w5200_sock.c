/* w5200_sock.c
 * WizNet W5200 Ethernet Controller Driver
 *
 * BSD Sockets-inspired API layer for TCP/IP I/O
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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_io.h"
#include "w5200_sock.h"
#include "w5200_debug.h"

/* Default IPs and utility subnets */
const uint16_t w52_const_subnet_classA[2] = {0xFF00, 0x0000};
const uint16_t w52_const_subnet_classB[2] = {0xFFFF, 0x0000};
const uint16_t w52_const_subnet_classC[2] = {0xFFFF, 0xFF00};
const uint16_t w52_const_ip_default[2] = {0xA980, 0x8082};  // 169.128.128.130
const uint16_t w52_const_mac_default[3] = {0x5452, 0x0000, 0xF801};  // 54:52:00:00:F8:01 ... sounds random enough

/* IRQ handler flag */
volatile uint8_t w5200_irq;
uint16_t w52_portoffset;

/* TCP state descriptions */
const char * wiznet_tcp_state[] = {
        "ESTABLISHED",
        "SYN_SENT",
        "SYN_RECV",
        "FIN_WAIT",
        "TIME_WAIT",
        "CLOSE",
        "CLOSE_WAIT",
        "LAST_ACK",
        "LISTEN",
        "CLOSING" };

const uint8_t wiznet_tcp_state_idx[] = {
        W52_SOCK_SR_SOCK_ESTABLISHED,
        W52_SOCK_SR_SOCK_SYNSENT,
        W52_SOCK_SR_SOCK_SYNRECV,
        W52_SOCK_SR_SOCK_FIN_WAIT,
        W52_SOCK_SR_SOCK_TIME_WAIT,
        W52_SOCK_SR_SOCK_CLOSED,
        W52_SOCK_SR_SOCK_CLOSE_WAIT,
        W52_SOCK_SR_SOCK_LAST_ACK,
        W52_SOCK_SR_SOCK_LISTEN,
        W52_SOCK_SR_SOCK_CLOSING };

/* Open socket information */
WIZNETSocketState w52_sockets[W52_MAX_SOCKETS];


/* Which socket did an IRQ refer to */
int wiznet_irq_getsocket()
{
	uint8_t ir2, bit;
	int i, sock = -1;

	if (w5200_irq) {
		ir2 = wiznet_r_reg(W52_IR2);
		if (!ir2) {
			w5200_irq = 0x00;
			return -EAGAIN;  // No IRQs pending but w5200_irq was never cleared.
		}
		/* Return 1 socket at a time, lower socket # = higher priority
		 * If more than 1 socket is pending, w5200_irq will not be cleared.
		 */
		for (i=0; i < W52_MAX_SOCKETS; i++) {
			bit = 1 << i;
			if ( (ir2 & bit) && sock < 0 ) {
				sock = i;
				w5200_irq = 0x00;
			}
			if ( (ir2 & bit) && sock >= 0 ) {
				w5200_irq = 0x01;
			}
		}
		return sock;
	}
	return -EAGAIN;  // No IRQ fired; nothing more to see here!
}

int wiznet_phystate()
{
	uint8_t phy;

	phy = wiznet_r_reg(W52_PHYSTATUS);
	if ((phy & 0x20) == 0x20)
		return 0;
	return -ENETDOWN;
}

int wiznet_socket(int protocol)
{
	int i;

	if (protocol == W52_SOCK_MR_PROTO_MACRAW || protocol == W52_SOCK_MR_PROTO_PPPOE) {
		if (w52_sockets[0].mode)
			return -EADDRINUSE;
	}

	switch (protocol) {
		case W52_SOCK_MR_PROTO_TCP:
		case W52_SOCK_MR_PROTO_UDP:
		case W52_SOCK_MR_PROTO_IPRAW:
			for (i = W52_MAX_SOCKETS - 1; i >= 0; i--) {
				if (w52_sockets[i].mode == 0x00) {
					w52_sockets[i].mode = protocol & 0x0F;
					w52_sockets[i].is_bind = 0;
					w52_sockets[i].tx_wr = wiznet_r_sockreg16(i, W52_SOCK_TX_WRITEPTR);
					w52_sockets[i].rx_rd = wiznet_r_sockreg16(i, W52_SOCK_RX_READPTR);

					wiznet_w_sockreg(i, W52_SOCK_MR, w52_sockets[i].mode);
					wiznet_w_command(i, W52_SOCK_CMD_CLOSE);
					wiznet_w_sockreg(i, W52_SOCK_IMR, 0x1F);
					wiznet_w_reg(W52_IMR, wiznet_r_reg(W52_IMR) | (1 << i));
					return i;
				}
			}
			break;

		case W52_SOCK_MR_PROTO_MACRAW:
		case W52_SOCK_MR_PROTO_PPPOE:
			w52_sockets[0].mode = protocol;
			wiznet_w_sockreg(0, W52_SOCK_MR, w52_sockets[0].mode);
			wiznet_w_command(0, W52_SOCK_CMD_CLOSE);
			wiznet_w_sockreg(0, W52_SOCK_IMR, (protocol == W52_SOCK_MR_PROTO_MACRAW ? 0x1F : 0xFF));
			wiznet_w_reg(W52_IMR2, (protocol == W52_SOCK_MR_PROTO_MACRAW ? 0x00 : 0xA0));
			wiznet_w_reg(W52_IMR, wiznet_r_reg(W52_IMR) | 1);
			w52_sockets[0].tx_wr = wiznet_r_sockreg16(0, W52_SOCK_TX_WRITEPTR);
			w52_sockets[0].rx_rd = wiznet_r_sockreg16(0, W52_SOCK_RX_READPTR);
			return 0;
			break;

		default:
			return -EPROTONOSUPPORT;
	}
	return -ENFILE;
}

int wiznet_close(int sockfd)
{
	uint8_t sr, irq;

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;

	irq = wiznet_r_sockreg(sockfd, W52_SOCK_IR);

	if (w52_sockets[sockfd].mode == W52_SOCK_MR_PROTO_TCP) {
		sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
		// Properly close TCP connections (listen or established)
		w52_sockets[sockfd].is_bind = 0;
		if (irq & W52_SOCK_IR_DISCON) {
			wiznet_w_command(sockfd, W52_SOCK_CMD_CLOSE);
		} else {
			if (sr == W52_SOCK_SR_SOCK_ESTABLISHED) {
				wiznet_w_command(sockfd, W52_SOCK_CMD_DISCON);
				if (!w5200_irq)
					WIZNET_CPU_WAIT;
				irq = wiznet_r_sockreg(sockfd, W52_SOCK_IR);
			}
		}
	}

	if (irq)
		wiznet_w_sockreg(sockfd, W52_SOCK_IR, irq); // Clear any outstanding IRQs
	wiznet_w_command(sockfd, W52_SOCK_CMD_CLOSE);

	// Mask any IRQs from this socket
	wiznet_w_reg(W52_IMR, wiznet_r_reg(W52_IMR) & ~(1 << sockfd));
	// Set socket as unused
	w52_sockets[sockfd].mode = 0x00;
	return 0;
}

int wiznet_connect(int sockfd, uint16_t *addr, uint16_t dport)
{
	uint8_t sr, irq;

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;
	
	// Network up?
	if (wiznet_phystate())
		return -ENETDOWN;

	sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);

	switch (w52_sockets[sockfd].mode) {
		case W52_SOCK_MR_PROTO_TCP:
			switch (sr) {
				case W52_SOCK_SR_SOCK_ESTABLISHED:
					return -EISCONN;
				case W52_SOCK_SR_SOCK_LISTEN:
					return -EADDRINUSE;
			}
			if (sr != W52_SOCK_SR_SOCK_CLOSED)
				wiznet_w_command(sockfd, W52_SOCK_CMD_CLOSE);

			// Source port: 40000 + an incrementing index * # of sockets plus the socket ID.  Each subsequent
			// use of a socket will cause the source port to walk up a range, rolling over after 255 uses.
			w52_sockets[sockfd].srcport_idx++;
			wiznet_w_sockreg16(sockfd, W52_SOCK_SRCPORT, w52_sockets[sockfd].srcport_idx * W52_MAX_SOCKETS + sockfd + W52_TCP_SRCPORT_BASE + w52_portoffset);
			wiznet_w_command(sockfd, W52_SOCK_CMD_OPEN);

			do {
				sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
				__delay_cycles(100);
			} while (sr != W52_SOCK_SR_SOCK_INIT);

			// Load dest IP, port
			wiznet_ip_bin_w_sockreg(sockfd, W52_SOCK_DESTIP, addr);
			wiznet_w_sockreg16(sockfd, W52_SOCK_DESTPORT, dport);

			// Connect
			w52_sockets[sockfd].is_bind = 0;  // This is definitely not a listener port!
			wiznet_w_command(sockfd, W52_SOCK_CMD_CONNECT);

			// Wait and see what happens
			do {
				irq = wiznet_r_sockreg(sockfd, W52_SOCK_IR);

				if (!irq && !w5200_irq) {
					__delay_cycles(1000);
				} else {
					if (irq & (W52_SOCK_IR_TIMEOUT | W52_SOCK_IR_DISCON)) {
						wiznet_w_sockreg(sockfd, W52_SOCK_IR, irq);
						wiznet_w_command(sockfd, W52_SOCK_CMD_CLOSE);
						return (irq & W52_SOCK_IR_TIMEOUT ? -ETIMEDOUT : -ECONNREFUSED);
					}
				}
				sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
			} while (sr != W52_SOCK_SR_SOCK_ESTABLISHED && !(irq & W52_SOCK_IR_CON));

			// Clear connect IRQ
			wiznet_w_sockreg(sockfd, W52_SOCK_IR, W52_SOCK_IR_CON);
			w52_sockets[sockfd].tx_wr = wiznet_r_sockreg16(sockfd, W52_SOCK_TX_WRITEPTR);
			w52_sockets[sockfd].rx_rd = wiznet_r_sockreg16(sockfd, W52_SOCK_RX_READPTR);
			return 0; // Connection established!

			break;

		case W52_SOCK_MR_PROTO_UDP:
			if (sr != W52_SOCK_SR_SOCK_CLOSED)
				wiznet_w_command(sockfd, W52_SOCK_CMD_CLOSE);

			// Source port: 40000 + an incrementing index * # of sockets plus the socket ID.  Each subsequent
			// use of a socket will cause the source port to walk up a range, rolling over after 255 uses.
			w52_sockets[sockfd].srcport_idx++;
			wiznet_w_sockreg16(sockfd, W52_SOCK_SRCPORT, w52_sockets[sockfd].srcport_idx * W52_MAX_SOCKETS + sockfd + W52_TCP_SRCPORT_BASE + w52_portoffset);
			wiznet_w_command(sockfd, W52_SOCK_CMD_OPEN);
			sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
			if (sr != W52_SOCK_SR_SOCK_UDP)
				return -EFAULT;

			// Load dest IP, port
			wiznet_ip_bin_w_sockreg(sockfd, W52_SOCK_DESTIP, addr);
			wiznet_w_sockreg16(sockfd, W52_SOCK_DESTPORT, dport);
			w52_sockets[sockfd].tx_wr = wiznet_r_sockreg16(sockfd, W52_SOCK_TX_WRITEPTR);
			w52_sockets[sockfd].rx_rd = wiznet_r_sockreg16(sockfd, W52_SOCK_RX_READPTR);
			return 0;
	}

	return -EPROTONOSUPPORT;  // Shouldn't normally reach here but may if user tries to connect with MACRAW, IPRAW, PPPOE.
	// It's expected that IPRAW users know what they're doing and can load the dest IP into socket registers by themself.
}

// Quickly re-establish SOCK_LISTEN state after a server connection is disconnected.
// Can be used by user to quickly close an accepted connection when finished and re-establish LISTEN.
int wiznet_quickbind(int sockfd)
{
	uint8_t sr;

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;
	if (w52_sockets[sockfd].mode != W52_SOCK_MR_PROTO_TCP )
		return -EPROTONOSUPPORT;

	sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
	if (sr != W52_SOCK_SR_SOCK_CLOSED)
		wiznet_w_command(sockfd, W52_SOCK_CMD_CLOSE);
	wiznet_w_command(sockfd, W52_SOCK_CMD_OPEN);

	sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
	if (sr != W52_SOCK_SR_SOCK_INIT)
		return -EFAULT;

	wiznet_w_command(sockfd, W52_SOCK_CMD_LISTEN);

	sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
	if (sr != W52_SOCK_SR_SOCK_LISTEN)
		return -EFAULT;

	return 0;
}

int wiznet_bind(int sockfd, uint16_t srcport)
{
	uint8_t sr;

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;
        
	sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);

	switch (w52_sockets[sockfd].mode) {
		case W52_SOCK_MR_PROTO_TCP:
		case W52_SOCK_MR_PROTO_UDP:
			if (sr == W52_SOCK_SR_SOCK_ESTABLISHED || sr == W52_SOCK_SR_SOCK_SYNSENT)
				return -EADDRINUSE;
			if (sr != W52_SOCK_SR_SOCK_CLOSED)
				wiznet_w_command(sockfd, W52_SOCK_CMD_CLOSE);

			// Set srcport, open in LISTEN mode
			wiznet_w_sockreg16(sockfd, W52_SOCK_SRCPORT, srcport);
			wiznet_w_command(sockfd, W52_SOCK_CMD_OPEN);
			sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
			break;
        }

        switch (w52_sockets[sockfd].mode) {
		case W52_SOCK_MR_PROTO_TCP:
			// Listen mode
			if (sr != W52_SOCK_SR_SOCK_INIT)
				return -EFAULT;
			wiznet_w_command(sockfd, W52_SOCK_CMD_LISTEN);
			w52_sockets[sockfd].is_bind = 1;

			sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
			if (sr != W52_SOCK_SR_SOCK_LISTEN) {
				wiznet_w_command(sockfd, W52_SOCK_CMD_CLOSE);
				return -EFAULT;
			}
			return 0;
			break;

		case W52_SOCK_MR_PROTO_UDP:
			if (sr != W52_SOCK_SR_SOCK_UDP)
				return -EFAULT;
			return 0;
			break;

		case W52_SOCK_MR_PROTO_IPRAW:
			// srcport is interpreted as the PROTO field instead
			if (sr != W52_SOCK_SR_SOCK_CLOSED)
				wiznet_w_command(sockfd, W52_SOCK_CMD_CLOSE);

			wiznet_w_sockreg(sockfd, W52_SOCK_PROTO, srcport);
			wiznet_w_command(sockfd, W52_SOCK_CMD_OPEN);
			sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
			if (sr != W52_SOCK_SR_SOCK_IPRAW)
				return -EFAULT;
			return 0;
			break;

		default:
			return -EPROTONOSUPPORT;
	}
}

int wiznet_accept(int sockfd)
{
	uint8_t irq, sr;

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;
        
	if (w52_sockets[sockfd].mode != W52_SOCK_MR_PROTO_TCP)
		return -EPROTONOSUPPORT;

	irq = wiznet_r_sockreg(sockfd, W52_SOCK_IR);
	if (irq & W52_SOCK_IR_CON) {
		wiznet_w_sockreg(sockfd, W52_SOCK_IR, W52_SOCK_IR_CON);
		w52_sockets[sockfd].tx_wr = wiznet_r_sockreg16(sockfd, W52_SOCK_TX_WRITEPTR);
		w52_sockets[sockfd].rx_rd = wiznet_r_sockreg16(sockfd, W52_SOCK_RX_READPTR);
		// Established!
		return 0;
	}
	sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
	if (sr == W52_SOCK_SR_SOCK_ESTABLISHED)
		return -EISCONN;

	if (irq & (W52_SOCK_IR_DISCON | W52_SOCK_IR_TIMEOUT)) {
		wiznet_w_sockreg(sockfd, W52_SOCK_IR, irq);
		if (sr != W52_SOCK_SR_SOCK_LISTEN) {
			if (w52_sockets[sockfd].is_bind)
				wiznet_quickbind(sockfd);  // Re-bind LISTEN port so new connections can come through.
		}
	}
	// Since we'd normally have to block to wait for a connection, we will return instead
	// and let the user determine this via interrupt
	return -EAGAIN;
}

int wiznet_recv(int sockfd, void *buf, uint16_t sz, uint8_t do_recv)
{
	uint16_t rsz, rsr;
	uint8_t irq, sr;

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;

	rsr = wiznet_recvsize(sockfd);
	if (rsr) {
		if (rsr < sz)
			rsz = rsr;
		else
			rsz = sz;

		wiznet_r_rxbuf(sockfd, rsz, buf, do_recv);  // Read contents and possibly acknowledge (do_recv determines this)
		return rsz;
	}

	// Disconnect requested from the other end, timeout detected, or socket in process of closing?  Process.
	if (w52_sockets[sockfd].mode == W52_SOCK_MR_PROTO_TCP) {
		irq = wiznet_r_sockreg(sockfd, W52_SOCK_IR);
		if (irq & (W52_SOCK_IR_DISCON | W52_SOCK_IR_TIMEOUT))
			wiznet_w_command(sockfd, W52_SOCK_CMD_DISCON);
		sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
		if (sr != W52_SOCK_SR_SOCK_ESTABLISHED) {
			if (sr != W52_SOCK_SR_SOCK_LISTEN) {
				if (w52_sockets[sockfd].is_bind)
					wiznet_quickbind(sockfd);  // Re-bind LISTEN port so new connections can come through.
			}
			return -ENOTCONN;
		}
	}

	// This signals that we should try again later; sockets are non-blocking in this library.
	return -EAGAIN;
}

int wiznet_search_recv(int sockfd, void *buf, uint16_t sz, uint8_t searchchar, uint8_t do_recv)
{
	uint16_t rsz, rsr;
	uint8_t irq, sr;

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;

	rsr = wiznet_recvsize(sockfd);
	if (rsr) {
		if (rsr < sz)
			rsz = rsr;
		else
			rsz = sz;

		rsz = wiznet_search_r_rxbuf(sockfd, rsz, buf, searchchar, do_recv);  // Read contents up to 'searchchar' or rsz
                                                                             // and possibly acknowledge (do_recv determines this)
		return rsz;
	}

	// Disconnect requested from the other end, timeout detected, or socket in process of closing?  Process.
	if (w52_sockets[sockfd].mode == W52_SOCK_MR_PROTO_TCP) {
		irq = wiznet_r_sockreg(sockfd, W52_SOCK_IR);
		if (irq & (W52_SOCK_IR_DISCON | W52_SOCK_IR_TIMEOUT))
			wiznet_w_command(sockfd, W52_SOCK_CMD_DISCON);
		sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
		if (sr != W52_SOCK_SR_SOCK_ESTABLISHED) {
			if (sr != W52_SOCK_SR_SOCK_LISTEN) {
				if (w52_sockets[sockfd].is_bind)
					wiznet_quickbind(sockfd);  // Re-bind LISTEN port so new connections can come through.
			}
			return -ENOTCONN;
		}
	}

	// This signals that we should try again later; sockets are non-blocking in this library.
	return -EAGAIN;
}

int wiznet_peek(int sockfd, uint16_t offset, void *buf, uint16_t sz)
{
	uint16_t rsz, rsr;
	uint8_t irq, sr;

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;

	irq = wiznet_r_sockreg(sockfd, W52_SOCK_IR);
	rsr = wiznet_recvsize(sockfd);
	if (rsr) {
		if (rsr <= offset)
			return -EAGAIN;  // Not enough data to satisfy this request
		if ( (rsr-offset) < sz ) {
			rsz = rsr - offset;
		} else {
			rsz = sz;
		}

		wiznet_peek_rxbuf(sockfd, offset, rsz, buf);  // Read contents
		return rsz;
	}

	// Disconnect requested from the other end, timeout detected, or socket in process of closing?  Process.
	if (w52_sockets[sockfd].mode == W52_SOCK_MR_PROTO_TCP) {
		if (irq & (W52_SOCK_IR_DISCON | W52_SOCK_IR_TIMEOUT))
			wiznet_w_command(sockfd, W52_SOCK_CMD_DISCON);
		sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
		if (sr != W52_SOCK_SR_SOCK_ESTABLISHED) {
			if (sr != W52_SOCK_SR_SOCK_LISTEN) {
				if (w52_sockets[sockfd].is_bind)
					wiznet_quickbind(sockfd);  // Re-bind LISTEN port so new connections can come through.
			}
			return -ENOTCONN;
		}
	}

	// This signals that we should try again later; sockets are non-blocking in this library.
	return -EAGAIN;
}

int wiznet_flush(int sockfd, uint16_t sz, uint8_t do_recv)
{
	uint16_t rsz, rsr;
	uint8_t irq, sr;

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;

	rsr = wiznet_recvsize(sockfd);
	if (rsr) {
		if (rsr < sz)
			rsz = rsr;
		else
			rsz = sz;

		wiznet_flush_rxbuf(sockfd, rsz, do_recv);  // Flush contents and possibly acknowledge (do_recv determines this)
		return rsz;
	}

	// Disconnect requested from the other end, timeout detected, or socket in process of closing?  Process.
	if (w52_sockets[sockfd].mode == W52_SOCK_MR_PROTO_TCP) {
		irq = wiznet_r_sockreg(sockfd, W52_SOCK_IR);
		if (irq & (W52_SOCK_IR_DISCON | W52_SOCK_IR_TIMEOUT))
			wiznet_w_command(sockfd, W52_SOCK_CMD_DISCON);
		sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
		if (sr != W52_SOCK_SR_SOCK_ESTABLISHED) {
			if (sr != W52_SOCK_SR_SOCK_LISTEN) {
				if (w52_sockets[sockfd].is_bind)
					wiznet_quickbind(sockfd);  // Re-bind LISTEN port so new connections can come through.
			}
			return -ENOTCONN;
		}
	}

	// This signals that we should try again later; sockets are non-blocking in this library.
	return -EAGAIN;
}

// Variant of wiznet_recv() with auto-fill-in of source IP/port - Reads UDP and IPRAW preambles
int wiznet_recvfrom(int sockfd, void *buf, uint16_t sz, uint16_t *srcaddr, uint16_t *srcport, uint8_t do_recv)
{
	uint16_t rsz, rsr, tmpaddr[2];
	uint8_t header[4];

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;

	switch (w52_sockets[sockfd].mode) {
		case W52_SOCK_MR_PROTO_UDP:
			if (srcaddr != NULL)
				wiznet_ip_bin_r_rxbuf(sockfd, 0, 0, srcaddr);
			else
				wiznet_ip_bin_r_rxbuf(sockfd, 0, 0, tmpaddr);
			wiznet_r_rxbuf(sockfd, 4, header, 0);
			if (srcport != NULL)
				*srcport = wiznet_ntohs(header);
			rsr = wiznet_ntohs(header+2);
			break;

		case W52_SOCK_MR_PROTO_IPRAW:
			if (srcaddr != NULL)
				wiznet_ip_bin_r_rxbuf(sockfd, 0, 0, srcaddr);
			else
				wiznet_ip_bin_r_rxbuf(sockfd, 0, 0, tmpaddr);
			wiznet_r_rxbuf(sockfd, 2, header, 0);
			rsr = wiznet_ntohs(header);
			break;

		default:
			return -EPROTONOSUPPORT;
	}

	if (rsr) {
		if (rsr < sz)
			rsz = rsr;
		else
			rsz = sz;

		wiznet_r_rxbuf(sockfd, rsz, buf, do_recv);  // Read contents and acknowledge so more data can come through.
		return rsz;
	}

	// This signals that we should try again later; sockets are non-blocking in this library.
	return -EAGAIN;
}

int wiznet_mac_recvfrom(void *buf, uint16_t sz, uint16_t *srcmac, uint16_t *dstmac, uint16_t *frametype, uint8_t read_preamble, uint8_t do_recv)
{
	uint8_t header[2];
	uint16_t rsz, rsr;
	int sockfd = 0;  // Makes copying/pasting my code easier

	if (w52_sockets[sockfd].mode != W52_SOCK_MR_PROTO_MACRAW)
		return -EBADF;

	if (read_preamble) {
		wiznet_flush_rxbuf(sockfd, 2, 0);
		rsr = wiznet_ntohs(header) + 4 - 14;
		wiznet_mac_bin_r_rxbuf(sockfd, 0, 0, dstmac);
		wiznet_mac_bin_r_rxbuf(sockfd, 0, 0, srcmac);
		wiznet_r_rxbuf(sockfd, 2, header, 0);
		*frametype = wiznet_ntohs(header);
	}

	rsr = wiznet_recvsize(sockfd);
	if (rsr) {
		if (rsr < sz)
			rsz = rsr;
		else
			rsz = sz;
		wiznet_r_rxbuf(sockfd, rsz, buf, do_recv);
		return rsz;
	}
	// This signals that we should try again later; sockets are non-blocking in this library.
	return -EAGAIN;
}


// Not intended to be used by user applications (internal use only).
int wiznet_txcommit(int sockfd)
{
	uint16_t tsz, tx_rdring, tx_rdring2;
	uint8_t irq;

	// Committing the whole TX buffer-
	tsz = wiznet_read_virtual_tsz(sockfd);
	tx_rdring = wiznet_r_sockreg16(sockfd, W52_SOCK_TX_READPTR) & W52_SOCK_MEM_MASK;

	// Send data and continue sending until TX buffer is fully flushed
	wiznet_w_command(sockfd, W52_SOCK_CMD_SEND);
	do {
		irq = wiznet_r_sockreg(sockfd, W52_SOCK_IR);
		if (!irq && !w5200_irq)  // If other IRQs are waiting, we can't sleep here b/c the IRQ line won't get pulsed
			__delay_cycles(1000);

		if (irq & W52_SOCK_IR_SEND_OK) {
			tx_rdring2 = wiznet_r_sockreg16(sockfd, W52_SOCK_TX_READPTR) & W52_SOCK_MEM_MASK;
			if (tx_rdring2 < tx_rdring)  // Ring buffer wrap-around
				tsz -= (W52_SOCK_MEM_SIZE + tx_rdring2) - tx_rdring;
			else
				tsz -= tx_rdring2 - tx_rdring;
			tx_rdring = tx_rdring2;
			wiznet_w_sockreg(sockfd, W52_SOCK_IR, irq);
			if (tsz)
				wiznet_w_command(sockfd, W52_SOCK_CMD_SEND);
		}

		if (irq & (W52_SOCK_IR_DISCON | W52_SOCK_IR_TIMEOUT)) {
			wiznet_w_sockreg(sockfd, W52_SOCK_IR, irq);
			wiznet_w_command(sockfd, W52_SOCK_CMD_DISCON);
			if (w52_sockets[sockfd].is_bind)
				wiznet_quickbind(sockfd);
			return (irq & W52_SOCK_IR_DISCON ? -ECONNABORTED : -ETIMEDOUT);
		}
	} while (tsz);
	return 0;  // Success
}

int wiznet_send(int sockfd, void *buf, uint16_t sz, uint8_t do_commit)
{
	uint16_t fsr;
	uint8_t irq, sr;

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;
	
	// Other datagram-based modes need to use sendto().
	if (w52_sockets[sockfd].mode != W52_SOCK_MR_PROTO_TCP)
		return -EPROTONOSUPPORT;

	// Disconnect requested or timeout detected?
	irq = wiznet_r_sockreg(sockfd, W52_SOCK_IR);
	if (irq & (W52_SOCK_IR_DISCON | W52_SOCK_IR_TIMEOUT)) {
		wiznet_w_command(sockfd, W52_SOCK_CMD_DISCON);
		wiznet_w_sockreg(sockfd, W52_SOCK_IR, irq);
		if (w52_sockets[sockfd].is_bind)
			wiznet_quickbind(sockfd);  // Re-bind LISTEN port so new connections can come through.
		return (irq & W52_SOCK_IR_DISCON ? -ECONNABORTED : -ETIMEDOUT);
	}

	sr = wiznet_r_sockreg(sockfd, W52_SOCK_SR);
	if (sr != W52_SOCK_SR_SOCK_ESTABLISHED)
		return -ENOTCONN;
	fsr = wiznet_read_virtual_fsr(sockfd);
	if (fsr < sz)
		return -ENFILE;  // Too much for the buffer!

	wiznet_w_txbuf(sockfd, sz, buf);

	if (do_commit)
		return wiznet_txcommit(sockfd);

	return 0;
}

int wiznet_sendto(int sockfd, void *buf, uint16_t sz, uint16_t *address, uint16_t dport, uint8_t do_commit)
{

	if (sockfd < 0 || sockfd >= W52_MAX_SOCKETS)
		return -EBADF;

	if (sz > W52_SOCK_MEM_SIZE)
		return -ENFILE;  // Too much for the buffer!

	switch (w52_sockets[sockfd].mode) {
		case W52_SOCK_MR_PROTO_UDP:
		case W52_SOCK_MR_PROTO_IPRAW:
			if (address != NULL) {
				wiznet_ip_bin_w_sockreg(sockfd, W52_SOCK_DESTIP, address);
				wiznet_w_sockreg16(sockfd, W52_SOCK_DESTPORT, dport);
			}
			wiznet_w_txbuf(sockfd, sz, buf);
			if (do_commit)
				return wiznet_txcommit(sockfd);
			return 0;
	}

	// Not appropriate for TCP.
	return -EPROTONOSUPPORT;
}

int wiznet_mac_sendto(void *buf, uint16_t sz, uint16_t *dstmac, uint16_t frametype, uint16_t totalsize, uint8_t write_preamble, uint8_t do_commit)
{
	uint8_t header[2];
	uint16_t ourmac[3];
	int sockfd = 0;  // Makes copying/pasting my code easier

	if (w52_sockets[sockfd].mode != W52_SOCK_MR_PROTO_MACRAW)
		return -EBADF;
	
	if (write_preamble) {
		// Total packet length is either derived from 'totalsize' or 'sz' if totalsize = 0.
		if (totalsize)
			wiznet_htons(totalsize+14, header);  // DATA packet = payload + size of Src, Dst MACs + frame type
		else
			wiznet_htons(sz+14, header);
		wiznet_w_txbuf(sockfd, 2, header);  // TX size
		// Dest MAC
		wiznet_mac_bin_w_txbuf(sockfd, dstmac);
		// Pre-fill SRCMAC to use our MAC address; assumes we're not trying to spoof anything ;)
		wiznet_mac_bin_r_reg(W52_SOURCEMAC, ourmac);
		wiznet_mac_bin_w_txbuf(sockfd, ourmac);
		// Ethernet FRAME TYPE
		wiznet_htons(frametype, header);
		wiznet_w_txbuf(sockfd, 2, header);
	}

	// Data payload
	wiznet_w_txbuf(sockfd, sz, buf);

	if (do_commit)
		return wiznet_txcommit(sockfd);
	return 0;
}

int wiznet_init()
{
	uint16_t i, ipzero[2];

	wiznet_io_init();

	// Perform device reset
	__delay_cycles(100);  // Assuming 25MHz MCLK; slower clock speeds will just produce longer delays, which is OK.
	w5200_irq = 0x00;
	w52_portoffset = 0;
	W52_RESET_PORTOUT |= W52_RESET_PORTBIT;
	__delay_cycles(4000000);
        
	i = wiznet_r_reg(W52_VERSIONR);
	if (!i)
		return -EFAULT;  // Init failed; can't ascertain chip version, possibly SPI fault?

	wiznet_w_reg(W52_MR, 0x80);
	while (wiznet_r_reg(W52_MR) & 0x80)
		__delay_cycles(10000);

	// Disable sockets from expressing IRQ on pin
	wiznet_w_reg(W52_IMR, 0x00);

	wiznet_w_reg(W52_MR, 0x00);  // Ping enabled
	wiznet_ip_bin_w_reg(W52_SUBNETMASK, w52_const_subnet_classC);
	ipzero[0] = ipzero[1] = 0x0000;
	wiznet_ip_bin_w_reg(W52_GATEWAY, ipzero);
	wiznet_ip_bin_w_reg(W52_SOURCEIP,w52_const_ip_default);
	wiznet_mac_bin_w_reg(W52_SOURCEMAC, w52_const_mac_default);

	// Trusting default values for RTR and RCR (0x07D0, 0x08)

	wiznet_w_reg(W52_PHYSTATUS, 0x00);
	wiznet_w_reg(W52_IMR2, 0x00);  // Don't trigger IRQ for any system-wide errors e.g. IP conflict

	// Close all sockets
	for (i=0; i < W52_MAX_SOCKETS; i++) {
		wiznet_w_command(i, W52_SOCK_CMD_CLOSE);
		wiznet_w_sockreg(i, W52_SOCK_MR, 0x00);
	}

	// Ready to roll
	return 0;
}

/* Example MSP430 Interrupt ISR for handling the WizNet IRQ line and updating w5200_irq correctly:
 *
 *   #pragma vector = PORT2_VECTOR
 *   __interrupt void P2_IRQ (void) {
 *           if(P2IFG & W52_IRQ_PORTBIT){
 *                   w5200_irq |= 0x01;
 *                   __bic_SR_register_on_exit(LPM4_bits);    // Wake up
 *                   P2IFG &= ~W52_IRQ_PORTBIT;   // Clear interrupt flag
 *           }
 *   }
 *
 */
