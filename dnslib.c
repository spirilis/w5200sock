/* w5200.c
 * WizNet W5200 Ethernet Controller Driver for MSP430
 * High-level Support I/O Library
 * Domain Name System
 *
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

#include <msp430.h>
#include "dnslib.h"
#include <stdlib.h>
#include <string.h>
#include "w5200_io.h"
#include "w5200_debug.h"

uint16_t *dnslib_resolver;  // Local DNS resolver IP
volatile uint16_t dnslib_errno;

const char *dnslib_errno_strings[] = {
	"Receive Fault",
	"Destination Port Fault",
	"DNS Reply Packet Size Fault",
	"DNS Reply Not QR",
	"DNS Reply ID Mismatch",
	"DNS Reply Contains No Answers",
	"DNS Reply RCODE Fault",
	"ARP Timeout",
	"Invalid DNS Name",
	"No DNS Reply Packet",
	"DNS Reply Not A Record",
	"DNS Reply Not INET",
	"DNS Reply Invalid Size",
	"DNS Reply Parser Truncated",
	"Send Fault",
	"Bind Fault",
};

char *dnslib_strerror(int err)
{
	if (err > DNSLIB_ERRNO_MAX || err < 1)
		return (char *)"UNKNOWN";
	return (char *)dnslib_errno_strings[err-1];
}

int dnslib_send_qname(int sockfd, const char *dnsname, uint16_t qtype, uint16_t qclass)
{
	int i=0, j, found;
	uint8_t len, metabuf[5];

	#if WIZNET_DEBUG > 1
	const char *funcname = "dnslib_send_qname()";
	#endif
	if (dnsname[i] == '\0') {  // Blank DNS name?
		wiznet_debug2_printf("%s: Blank DNS name?\n", funcname);
		return -EBADF;
	}

	do {
		if (dnsname[i] != '.') {
			j = 1;
			found = 0;
			do {
				if (dnsname[i+j] == '.' || dnsname[i+j] == '\0') {
					len = j;
					wiznet_sendto(sockfd, &len, 1, NULL, 0, 0);
					wiznet_sendto(sockfd, (char *)dnsname+i, j, NULL, 0, 0);
					found = 1;
					if (dnsname[i+j] != '\0')
						i += j + 1;
					else
						i += j;
				} else {
					j++;
				}
			} while (!found && j < 64);
		} else {
			wiznet_debug2_printf("%s: DNS addresses can't start with a dot!\n", funcname);
			return -EBADF;  // DNS addresses can't start with a dot!
		}
	} while (dnsname[i]);
	metabuf[0] = 0x00;  // Zero-length field terminates the qname list
	// qtype & qclass HTONS (little-to-big-endian conversion)
	wiznet_htons(qtype, metabuf+1);
	wiznet_htons(qclass, metabuf+3);
	wiznet_sendto(sockfd, metabuf, 5, NULL, 0, 0);

	return 0;
}

int dnslib_recv_qname(int sockfd, char *dnsname, uint16_t maxlen)
{
	int i=0;
	uint8_t c, initial=1;

	#if WIZNET_DEBUG > 1
	const char *funcname = "dnslib_recv_qname()";
	#endif
	do {
		if (wiznet_recv(sockfd, &c, 1, 0) == -EAGAIN) {
			wiznet_debug2_printf("%s: EAGAIN attempting to read socket %d\n", funcname, sockfd);
			return -EFAULT;
		}
		if (c >= 64) {  // Pointer; we don't support these
			dnsname[i] = '\0';
			i += 2;
			wiznet_recv(sockfd, &c, 1, 0);
			wiznet_debug3_printf("%s: Pointer found; not reading further\n", funcname);
			return i;
		}
		if (!c || (i+c > maxlen)) {
			dnsname[i] = '\0';
			return i;
		}
		if (!initial) {  // First entry doesn't get a dot prepended
			dnsname[i++] = '.';
		} else {
			initial = 0;
		}
		if (wiznet_recv(sockfd, dnsname+i, c, 0) == -EAGAIN) {
			wiznet_debug2_printf("%s: EAGAIN attempting to read socket %d\n", funcname, sockfd);
			return -EFAULT;
		}
		i += c;
	} while (i < maxlen);
	return i;
}

int dnslib_flush_qname(int sockfd)
{
	int i=0;
	uint8_t c;

	#if WIZNET_DEBUG > 1
	const char *funcname = "dnslib_flush_qname()";
	#endif
	do {
		if (wiznet_recv(sockfd, &c, 1, 0) == -EAGAIN) {
			wiznet_debug2_printf("%s: EAGAIN attempting to read socket %d\n", funcname, sockfd);
			return -EFAULT;
		}
		if (c < 64) {  // snippet lengths >= 64 signify pointers
			i += c + 1;
			wiznet_flush(sockfd, c, 0);
		} else {  // Pointer
			i += 2;
			wiznet_flush(sockfd, 1, 0);
			wiznet_debug3_printf("%s: Pointer found; reading no further\n", funcname);
			return i;  // Read no further after a pointer
		}
	} while (c);
	return i;
}

int dnslib_gethostbyname(char *dnsname, uint16_t *ip)
{
	int sockfd, pktlen;
	DNSPktHeader dns_qry;
	static uint16_t dns_id = 0;
	int i=0;
	uint8_t netbuf[64], sockimr, ir2bit;
	uint16_t srcip[2], srcport;

	#if WIZNET_DEBUG > 1
	const char *funcname = "dnslib_gethostbyname()";
	#endif

	dnslib_errno = 0;
	sockfd = wiznet_socket(IPPROTO_UDP);
	if (sockfd < 0) {
		wiznet_debug2_printf("%s: Error %d opening UDP socket\n", funcname, sockfd);
		return sockfd;
	}

	sockimr = wiznet_r_reg(W52_IMR);
	ir2bit = 1 << sockfd;
	wiznet_w_reg(W52_IMR, ir2bit);  // Only interrupt on this UDP socket

	dns_qry.id = sockfd * dns_id;
	dns_qry.qdcount = 1;
	dns_qry.ancount = 0;
	dns_qry.nscount = 0;
	dns_qry.arcount = 0;
	dns_qry.bitfields = DNSLIB_PKTHEADER_BITFIELD_RD;

	if ( (i = wiznet_bind(sockfd, 40000)) < 0 ) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_BINDFAULT;
		wiznet_debug2_printf("%s: %s on socket %d\n", funcname, dnslib_errno_strings[dnslib_errno-1], sockfd);
		return i;
	}

	wiznet_htons(dns_qry.id, netbuf+0);
	wiznet_htons(dns_qry.bitfields, netbuf+2);
	wiznet_htons(dns_qry.qdcount, netbuf+4);
	wiznet_htons(dns_qry.ancount, netbuf+6);
	wiznet_htons(dns_qry.nscount, netbuf+8);
	wiznet_htons(dns_qry.arcount, netbuf+10);
	i = wiznet_sendto(sockfd, netbuf, sizeof(DNSPktHeader), dnslib_resolver, 53, 0);  // Load UDP preamble, do not commit packet
	if (i < 0) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_SENDTOFAULT;
		wiznet_debug2_printf("%s: %s on socket %d\n", funcname, dnslib_errno_strings[dnslib_errno-1], sockfd);
		return i;
	}

	wiznet_debug3_printf("%s: Submitting DNS A-record query for domain name (%s)\n", funcname, dnsname);
	if (dnslib_send_qname(sockfd, dnsname, DNSLIB_TYPE_A, DNSLIB_CLASS_IN) < 0) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_BADNAME;
		wiznet_debug2_printf("%s: %s (%s)\n", funcname, dnslib_errno_strings[dnslib_errno-1], dnsname);
		return -EFAULT;
	}

	if ( (i = wiznet_txcommit(sockfd)) < 0 ) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_ARPTIMEOUT;
		wiznet_debug2_printf("%s: %s\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -ETIMEDOUT;
	}

	// Wait for reply
	do {
		i++;
		__delay_cycles(64000);
	} while (!wiznet_recvsize(sockfd) && i < 2500);

	// Timed out?
	if (i >= 2500) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_NOREPLY;
		wiznet_debug2_printf("%s: %s\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -ETIMEDOUT;
	}

	// Received result; process
	wiznet_w_reg(W52_IMR, sockimr);  // Restore interrupt mask

	// Pull packet source addr + port
	if (wiznet_recvfrom(sockfd, NULL, 0, srcip, &srcport, 0) == -EAGAIN) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_NOREPLY;
		wiznet_debug2_printf("%s: %s\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;  // Really shouldn't happen...
	}

	wiznet_debug3_printf("%s: Reply packet received from %u.%u.%u.%u:%u\n", funcname, srcip[0] >> 8, srcip[0] & 0xFF, srcip[1] >> 8, srcip[1] & 0xFF, srcport);
	if (srcport != 53) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_DSTPORTFAULT;
		wiznet_debug2_printf("%s: %s\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;
	}
	pktlen = wiznet_recvsize(sockfd);
	if (pktlen < sizeof(DNSPktHeader)+10) {  // Length of DNS header + A-record Answer frame
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_PKTSIZEFAULT;
		wiznet_debug2_printf("%s: %s (found %d)\n", funcname, dnslib_errno_strings[dnslib_errno-1], pktlen);
		return -EFAULT;
	}

	i = wiznet_recv(sockfd, netbuf, sizeof(DNSPktHeader), 0);
	if (i < 0 || i != sizeof(DNSPktHeader)) {
		dnslib_errno = DNSLIB_ERRNO_PARSE_TRUNCATED;
		wiznet_debug2_printf("%s: %s (found %d, expected %d)\n", funcname, dnslib_errno_strings[dnslib_errno-1], i, sizeof(DNSPktHeader));
		return -EFAULT;
	}

	dns_qry.id = wiznet_ntohs(netbuf+0);
	dns_qry.bitfields = wiznet_ntohs(netbuf+2);
	dns_qry.qdcount = wiznet_ntohs(netbuf+4);
	dns_qry.ancount = wiznet_ntohs(netbuf+6);
	dns_qry.nscount = wiznet_ntohs(netbuf+8);
	dns_qry.arcount = wiznet_ntohs(netbuf+10);
	wiznet_debug3_printf("%s: DNS reply header: ID=%u, bitfields=%x, qdcount=%u, ancount=%u, nscount=%u, arcount=%u\n", funcname,
		dns_qry.id, dns_qry.bitfields, dns_qry.qdcount, dns_qry.ancount, dns_qry.nscount, dns_qry.arcount);

	if ( !(dns_qry.bitfields & DNSLIB_PKTHEADER_BITFIELD_QR) ) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_REPLYNOTQR;
		wiznet_debug2_printf("%s: %s\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;
	}
	if (dns_qry.id != dns_id) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_REPLYIDMISMATCH;
		wiznet_debug2_printf("%s: %s (found %u)\n", funcname, dnslib_errno_strings[dnslib_errno-1], dns_qry.id);
		return -EFAULT;
	}
	dns_id++;  // Increment dns_id here so we use a different unique ID for later requests

	if (!dns_qry.ancount) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_REPLYNOANSWERS;
		wiznet_debug2_printf("%s: %s\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;
	}
	if (dns_qry.bitfields & DNSLIB_PKTHEADER_BITFIELD_RCODE) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_REPLYRCODE;
		wiznet_debug2_printf("%s: %s\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;
	}
	
	// Read answer packet

	if (dns_qry.qdcount) {  // Need to read our original query first
		if (dnslib_flush_qname(sockfd) < 0) {
			wiznet_close(sockfd);
			dnslib_errno = DNSLIB_ERRNO_PARSE_TRUNCATED;
			wiznet_debug2_printf("%s: %s (flushing qname of original query)\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
			return -EFAULT;
		}
		
		if (wiznet_recv(sockfd, netbuf, 2, 0) < 0) {  // Query type
			wiznet_close(sockfd);
			dnslib_errno = DNSLIB_ERRNO_PARSE_TRUNCATED;
			wiznet_debug2_printf("%s: %s (recv query type of original query)\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
			return -EFAULT;
		}
		if (wiznet_recv(sockfd, netbuf, 2, 0) < 0) {  // Query class
			wiznet_close(sockfd);
			dnslib_errno = DNSLIB_ERRNO_PARSE_TRUNCATED;
			wiznet_debug2_printf("%s: %s (recv query class of original query)\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
			return -EFAULT;
		}
	}

	// Answer
	if (dnslib_flush_qname(sockfd) < 0) {  // NAME that was queried
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_PARSE_TRUNCATED;
		wiznet_debug2_printf("%s: %s (flush qname of answer)\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;
	}
	if (wiznet_recv(sockfd, netbuf, 2, 0) < 0) {  // Query type
		dnslib_errno = DNSLIB_ERRNO_PARSE_TRUNCATED;
		wiznet_debug2_printf("%s: %s (recv query type of answer)\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;
	}
	if (wiznet_ntohs(netbuf) != DNSLIB_TYPE_A) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_NOTARECORD;
		wiznet_debug2_printf("%s: %s (found %u)\n", funcname, dnslib_errno_strings[dnslib_errno-1], wiznet_ntohs(netbuf));
		return -EFAULT;
	}
	if (wiznet_recv(sockfd, netbuf, 2, 0) < 0) {  // Query class
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_PARSE_TRUNCATED;
		wiznet_debug2_printf("%s: %s (recv query class of answer)\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;
	}
	if (wiznet_ntohs(netbuf) != DNSLIB_CLASS_IN) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_NOTINETCLASS;
		wiznet_debug2_printf("%s: %s (found %u)\n", funcname, dnslib_errno_strings[dnslib_errno-1], wiznet_ntohs(netbuf));
		return -EFAULT;
	}
	if (wiznet_recv(sockfd, netbuf, 4, 0) < 0) {  // TTL (32-bit integer)
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_PARSE_TRUNCATED;
		wiznet_debug2_printf("%s: %s (recv TTL of answer)\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;
	}
	if (wiznet_recv(sockfd, netbuf, 2, 0) < 0) {  // RDLENGTH
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_PARSE_TRUNCATED;
		wiznet_debug2_printf("%s: %s (recv RDLENGTH of answer)\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;
	}
	if (wiznet_ntohs(netbuf) != 4) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_REPLYINVALIDSIZE;
		wiznet_debug2_printf("%s: %s (found %u)\n", funcname, dnslib_errno_strings[dnslib_errno-1], wiznet_ntohs(netbuf));
		return -EFAULT;
	}

	if (wiznet_recv(sockfd, netbuf, 4, 0) < 0) {
		wiznet_close(sockfd);
		dnslib_errno = DNSLIB_ERRNO_PARSE_TRUNCATED;
		wiznet_debug2_printf("%s: %s (read IP address of answer)\n", funcname, dnslib_errno_strings[dnslib_errno-1]);
		return -EFAULT;
	}
	ip[0] = wiznet_ntohs(netbuf);
	ip[1] = wiznet_ntohs(netbuf+2);
	wiznet_debug3_printf("%s: DNS A record received, IP = %u.%u.%u.%u\n", funcname, ip[0] >> 8, ip[0] & 0xFF, ip[1] >> 8, ip[1] & 0xFF);

	wiznet_close(sockfd);
	return 0;
}
