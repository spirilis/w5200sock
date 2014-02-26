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
#include "w5200_config.h"
#include "w5200_io.h"
#include "w5200_buf.h"


void wiznet_w_txbuf(int sockfd, uint16_t sz, void *buf)
{
	uint16_t tx_wr, real_ptr, i, j;
	uint8_t *bufptr = (uint8_t *)buf;

	if (sz > W52_SOCK_MEM_SIZE)
		return;

	tx_wr = w52_sockets[sockfd].tx_wr;
	i = tx_wr & W52_SOCK_MEM_MASK;
	j = W52_SOCK_MEM_MASK - i;
	if (j < sz) {  // Writing would overflow the buffer
		real_ptr = W52_TXMEM_BASE + W52_SOCK_MEM_SIZE * sockfd + i;
		wiznet_w_buf(real_ptr, j, bufptr);
		tx_wr += j;
		sz -= j;
		bufptr += j;
		i = 0;
	}
	real_ptr = W52_TXMEM_BASE + W52_SOCK_MEM_SIZE * sockfd + i;
	wiznet_w_buf(real_ptr, sz, bufptr);
	tx_wr += sz;
	w52_sockets[sockfd].tx_wr = tx_wr;
	wiznet_w_sockreg16(sockfd, W52_SOCK_TX_WRITEPTR, tx_wr);
}

void wiznet_fill_txbuf(int sockfd, uint16_t sz, uint8_t val)
{
	uint16_t tx_wr, real_ptr, i, j;

	if (sz > W52_SOCK_MEM_SIZE)
		return;

	tx_wr = w52_sockets[sockfd].tx_wr;
	i = tx_wr & W52_SOCK_MEM_MASK;
	j = W52_SOCK_MEM_MASK - i;
	if (j < sz) {  // Writing would overflow the buffer
		real_ptr = W52_TXMEM_BASE + W52_SOCK_MEM_SIZE * sockfd + i;
		wiznet_w_set(real_ptr, j, val);
		tx_wr += j;
		sz -= j;
		i = 0;
	}
	real_ptr = W52_TXMEM_BASE + W52_SOCK_MEM_SIZE * sockfd + i;
	wiznet_w_set(real_ptr, sz, val);
	tx_wr += sz;
	w52_sockets[sockfd].tx_wr = tx_wr;
	wiznet_w_sockreg16(sockfd, W52_SOCK_TX_WRITEPTR, tx_wr);
}

void wiznet_r_rxbuf(int sockfd, uint16_t sz, void *buf, uint8_t do_recv_cmd)
{
	uint16_t rx_rd, real_ptr, i, j;
	uint8_t *bufptr = (uint8_t *)buf;

	if (sz > W52_SOCK_MEM_SIZE)
		return;

	rx_rd = w52_sockets[sockfd].rx_rd;
	i = rx_rd & W52_SOCK_MEM_MASK;
	j = W52_SOCK_MEM_SIZE - i;
	if (j < sz) {  // Reading requires wrap-around
		real_ptr = W52_RXMEM_BASE + W52_SOCK_MEM_SIZE * sockfd + i;
		wiznet_r_buf(real_ptr, j, bufptr);
		rx_rd += j;
		sz -= j;
		bufptr += j;
		i = 0;
	}
	real_ptr = W52_RXMEM_BASE + W52_SOCK_MEM_SIZE * sockfd + i;
	wiznet_r_buf(real_ptr, sz, bufptr);
	rx_rd += sz;
	wiznet_w_sockreg16(sockfd, W52_SOCK_RX_READPTR, rx_rd);

	if (do_recv_cmd) {
		wiznet_w_sockreg(sockfd, W52_SOCK_IR, W52_SOCK_IR_RECV);  // Clear RECV IRQ
		wiznet_w_sockreg(sockfd, W52_SOCK_CR, W52_SOCK_CMD_RECV); // Let more data in!
		w52_sockets[sockfd].rx_rd = wiznet_r_sockreg16(sockfd, W52_SOCK_RX_READPTR);
	} else {
		w52_sockets[sockfd].rx_rd = rx_rd;
	}
}

void wiznet_peek_rxbuf(int sockfd, uint16_t offset, uint16_t sz, void *buf)
{
	uint16_t rx_rd, real_ptr, i, j;
	uint8_t *bufptr = (uint8_t *)buf;

	if ((offset+sz) > W52_SOCK_MEM_SIZE)
		return;

	rx_rd = w52_sockets[sockfd].rx_rd + offset;  // Adjusted readptr
	i = rx_rd & W52_SOCK_MEM_MASK;
	j = W52_SOCK_MEM_SIZE - i;
	if (j < sz) {  // Reading requires wrap-around
		real_ptr = W52_RXMEM_BASE + W52_SOCK_MEM_SIZE * sockfd + i;
		wiznet_r_buf(real_ptr, j, bufptr);
		rx_rd += j;
		sz -= j;
		bufptr += j;
		i = 0;
	}
	real_ptr = W52_RXMEM_BASE + W52_SOCK_MEM_SIZE * sockfd + i;
	wiznet_r_buf(real_ptr, sz, bufptr);
}

void wiznet_flush_rxbuf(int sockfd, uint16_t fsz, uint8_t do_recv_cmd)
{
	uint16_t rx_rd, rsz;

	rsz = wiznet_recvsize(sockfd);
	if (fsz > rsz || fsz < 1)
		return;

	rx_rd = w52_sockets[sockfd].rx_rd + fsz;  // Advance read pointer to ignore its contents

	wiznet_w_sockreg16(sockfd, W52_SOCK_RX_READPTR, rx_rd);

	if (do_recv_cmd) {
		wiznet_w_sockreg(sockfd, W52_SOCK_IR, W52_SOCK_IR_RECV);  // Clear RECV IRQ
		wiznet_w_sockreg(sockfd, W52_SOCK_CR, W52_SOCK_CMD_RECV); // Let more data in!
		w52_sockets[sockfd].rx_rd = wiznet_r_sockreg16(sockfd, W52_SOCK_RX_READPTR);
	} else {
		w52_sockets[sockfd].rx_rd = rx_rd;
	}
}

uint16_t wiznet_search_r_rxbuf(int sockfd, uint16_t sz, void *buf, uint8_t searchchar, uint8_t do_recv_cmd)
{
	uint16_t rx_rd, real_ptr, i, j, retlen, total=0;
	uint8_t *bufptr = (uint8_t *)buf;

	if (sz > W52_SOCK_MEM_SIZE)
		return 0;

	rx_rd = w52_sockets[sockfd].rx_rd;
	i = rx_rd & W52_SOCK_MEM_MASK;
	retlen = j = W52_SOCK_MEM_SIZE - i;
	if (j < sz) {  // Reading requires wrap-around
		real_ptr = W52_RXMEM_BASE + W52_SOCK_MEM_SIZE * sockfd + i;
		retlen = wiznet_search_r_buf(real_ptr, j, bufptr, searchchar);
		rx_rd += retlen;
		sz -= retlen;
		bufptr += retlen;
        total += retlen;
		i = 0;
	}
	if (retlen == j) {  // searchchar wasn't found during initial pre-wraparound read
		real_ptr = W52_RXMEM_BASE + W52_SOCK_MEM_SIZE * sockfd + i;
		retlen = wiznet_search_r_buf(real_ptr, sz, bufptr, searchchar);
		rx_rd += retlen;
		total += retlen;
	}
	wiznet_w_sockreg16(sockfd, W52_SOCK_RX_READPTR, rx_rd);

	if (do_recv_cmd) {
		wiznet_w_sockreg(sockfd, W52_SOCK_IR, W52_SOCK_IR_RECV);  // Clear RECV IRQ
		wiznet_w_sockreg(sockfd, W52_SOCK_CR, W52_SOCK_CMD_RECV); // Let more data in!
		w52_sockets[sockfd].rx_rd = wiznet_r_sockreg16(sockfd, W52_SOCK_RX_READPTR);
	} else {
		w52_sockets[sockfd].rx_rd = rx_rd;
	}

    return total;
}

uint16_t wiznet_recvsize(int sockfd)
{
	uint16_t rx_wr, rx_rd;

	rx_wr = wiznet_r_sockreg16(sockfd, W52_SOCK_RX_WRITEPTR) & W52_SOCK_MEM_MASK;
	rx_rd = w52_sockets[sockfd].rx_rd & W52_SOCK_MEM_MASK;
	if (rx_rd > rx_wr)
		rx_wr += W52_SOCK_MEM_SIZE;
	return (rx_wr - rx_rd);
}

uint16_t wiznet_read_virtual_fsr(int sockfd)
{
	uint16_t tx_rd, tx_wr;

	tx_rd = wiznet_r_sockreg16(sockfd, W52_SOCK_TX_READPTR) & W52_SOCK_MEM_MASK;
	tx_wr = w52_sockets[sockfd].tx_wr & W52_SOCK_MEM_MASK;
	if (tx_wr >= tx_rd)
		tx_rd += W52_SOCK_MEM_SIZE;
	return tx_rd - tx_wr;
}

/* IP string vs Binary conversions */
int wiznet_ip2binary(const void *str, uint16_t *bin)
{
        char *strptr = (char *)str;
        uint8_t *binbyte = (uint8_t *)bin;
        int i=0, val=0, valok=0, idx=0;

        do {
                if (strptr[i] >= '0' && strptr[i] <= '9') {
                        val *= 10;
                        val += strptr[i] - '0';
                        valok = 1;
                } else {
                        if (!valok)  // No chars appropriate without a valid value in the buffer
                                return -EBADF;
                        if (strptr[i] == '.' || strptr[i] == '\0') {  // Delimiter or end-of-string
                                if (val > 255)
                                        return -EBADF;
                                // Little-Endian conversion
                                if (idx % 2 == 0)
                                        binbyte[idx+1] = val;
                                else
                                        binbyte[idx-1] = val;
                                idx++;
                                val = valok = 0;
                                if (strptr[i] == '\0' && idx < 3)
                                        return -EBADF;  // Incomplete IP
                        } else {
                                return -EBADF;  // Junk characters in string
                        }
                }
                i++;
        } while (idx < 4);
        return 0;
}

const uint16_t _wiznet_ipconv_pow10[] = {1, 10, 100};

int wiznet_binary2ip(const uint16_t *bin, void *buf)
{
        char *bufptr = (char *)buf;
        uint8_t *binbyte = (uint8_t *)bin;
        int i=0, j=0, k=0, val=0, idx=0;

        for (idx = 0; idx < 4; idx++) {
                // Little-Endian conversion
                if (idx % 2 == 0)
                        val = binbyte[idx+1];
                else
                        val = binbyte[idx-1];

                k = 0;
                for (j=2; j >= 0; j--) {
                        if ( !j || k || val / _wiznet_ipconv_pow10[j] != 0 ) {
                                bufptr[i++] = (val / _wiznet_ipconv_pow10[j]) + '0';
                                val -= (val / _wiznet_ipconv_pow10[j]) * _wiznet_ipconv_pow10[j];
                                k = 1;
                        }
                }
                if (idx < 3)
                        bufptr[i++] = '.';
        }
        bufptr[i] = '\0';  // NULL-terminated string

        return i;
}

/* MAC address string vs. Binary conversion */
int wiznet_mac2binary(const void *str, uint16_t *bin)
{
        char *strptr = (char *)str;
        uint8_t *binbyte = (uint8_t *)bin;
        int i=0, val=0, valok=0, idx=0;

        do {
                if ( (strptr[i] >= '0' && strptr[i] <= '9') ||
                         (strptr[i] >= 'a' && strptr[i] <= 'f') ||
                         (strptr[i] >= 'A' && strptr[i] <= 'F') ) {
                        val <<= 4;
                        if (strptr[i] >= '0' && strptr[i] <= '9')
                                val |= strptr[i] - '0';
                        if (strptr[i] >= 'a' && strptr[i] <= 'f')
                                val |= strptr[i] - 'a' + 10;
                        if (strptr[i] >= 'A' && strptr[i] <= 'F')
                                val |= strptr[i] - 'A' + 10;
                        valok = 1;
                } else {
                        if (!valok)  // No chars appropriate without a valid value in the buffer
                                return -EBADF;
                        if (strptr[i] == ':' || strptr[i] == '\0') {  // Delimiter or end-of-string
                                if (val & 0xFF00)
                                        return -EBADF;
                                // Little-Endian conversion
                                if (idx % 2 == 0)
                                        binbyte[idx+1] = val;
                                else
                                        binbyte[idx-1] = val;
                                idx++;
                                val = valok = 0;
                                if (strptr[i] == '\0' && idx < 5)
                                        return -EBADF;  // Incomplete IP
                        } else {
                                return -EBADF;  // Junk characters in string
                        }
                }
                i++;
        } while (idx < 6);
        return 0;
}

const uint16_t _wiznet_ipconv_hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

int wiznet_binary2mac(const uint16_t *bin, void *buf)
{
        char *bufptr = (char *)buf;
        uint8_t *binbyte = (uint8_t *)bin;
        int i=0, val=0, idx=0;

        for (idx = 0; idx < 6; idx++) {
                // Little-Endian conversion
                if (idx % 2 == 0)
                        val = binbyte[idx+1];
                else
                        val = binbyte[idx-1];

                bufptr[i++] = _wiznet_ipconv_hex[val >> 4];
                bufptr[i++] = _wiznet_ipconv_hex[val & 0x0F];

                if (idx < 5)
                        bufptr[i++] = ':';
        }
        bufptr[i] = '\0';  // NULL-terminated string

        return i;
}

/* IP address-based buffer I/O */
void wiznet_ip_bin_w_reg(uint16_t addr, const uint16_t *ipbin)
{
	wiznet_w_reg16(addr, ipbin[0]);
	wiznet_w_reg16(addr+2, ipbin[1]);
}

void wiznet_ip_bin_r_reg(uint16_t addr, uint16_t *ipbin)
{
	ipbin[0] = wiznet_r_reg16(addr);
	ipbin[1] = wiznet_r_reg16(addr+2);
}

int wiznet_ip_str_w_reg(uint16_t addr, const void *s)
{
	uint8_t *sptr = (uint8_t *)s;
	uint16_t ipval[2];
	int retval;

	retval = wiznet_ip2binary(sptr, ipval);
	if (retval < 0)
		return retval;
	wiznet_ip_bin_w_reg(addr, ipval);
	return retval;
}

int wiznet_ip_str_r_reg(uint16_t addr, void *ipbuf)
{
	uint8_t *ipstr = (uint8_t *)ipbuf;
	uint16_t ipval[2];

	wiznet_ip_bin_r_reg(addr, ipval);
	return wiznet_binary2ip(ipval, ipstr);
}

void wiznet_ip_bin_w_txbuf(int sockfd, const uint16_t *ipaddr)
{
	uint8_t ipbin[4];

	ipbin[0] = ipaddr[0] >> 8;
	ipbin[1] = ipaddr[0] & 0xFF;
	ipbin[2] = ipaddr[1] >> 8;
	ipbin[3] = ipaddr[1] & 0xFF;

	wiznet_w_txbuf(sockfd, 4, ipbin);
}

void wiznet_ip_bin_r_rxbuf(int sockfd, uint16_t offset, uint8_t do_recv_cmd, uint16_t *ipbuf)
{
	uint8_t inbuf[4];

	if (offset)
		wiznet_peek_rxbuf(sockfd, offset, 4, inbuf);
	else
		wiznet_r_rxbuf(sockfd, 4, inbuf, do_recv_cmd);
	
	ipbuf[0] = (inbuf[0] << 8) | inbuf[1];
	ipbuf[1] = (inbuf[2] << 8) | inbuf[3];
}

void wiznet_ip_str_w_txbuf(int sockfd, const uint16_t *ipaddr)
{
	uint8_t ipstr[17], ipstrlen;

	wiznet_binary2ip(ipaddr, ipstr);
	ipstrlen = strlen((char *)ipstr);
	wiznet_w_txbuf(sockfd, ipstrlen, ipstr);
}

int wiznet_ip_str_r_rxbuf(int sockfd, uint16_t offset, uint8_t do_recv_cmd, void *ipstrbuf)
{
	uint16_t ipbuf[2];

	wiznet_ip_bin_r_rxbuf(sockfd, offset, do_recv_cmd, ipbuf);
	return wiznet_binary2ip(ipbuf, ipstrbuf);
}


/* MAC address-based buffer I/O */
void wiznet_mac_bin_w_reg(uint16_t addr, const uint16_t *macbin)
{
	wiznet_w_reg16(addr, macbin[0]);
	wiznet_w_reg16(addr+2, macbin[1]);
	wiznet_w_reg16(addr+4, macbin[2]);
}

void wiznet_mac_bin_r_reg(uint16_t addr, uint16_t *macbin)
{
	macbin[0] = wiznet_r_reg16(addr);
	macbin[1] = wiznet_r_reg16(addr+2);
	macbin[2] = wiznet_r_reg16(addr+4);
}

int wiznet_mac_str_w_reg(uint16_t addr, const void *s)
{
	uint8_t *sptr = (uint8_t *)s;
	uint16_t macval[2];
	int retval;

	retval = wiznet_mac2binary(sptr, macval);
	if (retval < 0)
		return retval;
	wiznet_mac_bin_w_reg(addr, macval);
	return retval;
}

int wiznet_mac_str_r_reg(uint16_t addr, void *macbuf)
{
	uint8_t *macstr = (uint8_t *)macbuf;
	uint16_t macval[2];

	wiznet_mac_bin_r_reg(addr, macval);
	return wiznet_binary2mac(macval, macstr);
}

void wiznet_mac_bin_w_txbuf(int sockfd, const uint16_t *macaddr)
{
	uint8_t macbin[6];

	macbin[0] = macaddr[0] >> 8;
	macbin[1] = macaddr[0] & 0xFF;
	macbin[2] = macaddr[1] >> 8;
	macbin[3] = macaddr[1] & 0xFF;
	macbin[4] = macaddr[2] >> 8;
	macbin[5] = macaddr[2] & 0xFF;

	wiznet_w_txbuf(sockfd, 4, macbin);
}

void wiznet_mac_bin_r_rxbuf(int sockfd, uint16_t offset, uint8_t do_recv_cmd, uint16_t *macbuf)
{
	uint8_t inbuf[6];

	if (offset)
		wiznet_peek_rxbuf(sockfd, offset, 6, inbuf);
	else
		wiznet_r_rxbuf(sockfd, 6, inbuf, do_recv_cmd);
	
	macbuf[0] = (inbuf[0] << 8) | inbuf[1];
	macbuf[1] = (inbuf[2] << 8) | inbuf[3];
	macbuf[2] = (inbuf[4] << 8) | inbuf[5];
}

void wiznet_mac_str_w_txbuf(int sockfd, const uint16_t *macaddr)
{
	uint8_t macstr[18], macstrlen;

	wiznet_binary2mac(macaddr, macstr);
	macstrlen = strlen((char *)macstr);
	wiznet_w_txbuf(sockfd, macstrlen, macstr);
}

int wiznet_mac_str_r_rxbuf(int sockfd, uint16_t offset, uint8_t do_recv_cmd, void *macstrbuf)
{
	uint16_t macbuf[3];

	wiznet_mac_bin_r_rxbuf(sockfd, offset, do_recv_cmd, macbuf);
	return wiznet_binary2mac(macbuf, macstrbuf);
}

/* Port endianness conversions */
void wiznet_htons(uint16_t num, uint8_t *outbuf)
{
        // Little-to-Big-Endian copy
        outbuf[0] = num >> 8;
        outbuf[1] = num & 0xFF;
}

uint16_t wiznet_ntohs(uint8_t *buf)
{
        return (buf[1] | (buf[0] << 8));
}

