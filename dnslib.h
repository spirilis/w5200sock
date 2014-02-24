/* dnslib.h
 * WizNet W5200 Ethernet Controller Driver for MSP430
 * High-level Support I/O Library
 * Domain Name System
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

#ifndef DNSLIB_H
#define DNSLIB_H

#include <msp430.h>
#include <stdint.h>
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"


/* DNS packet - Header */
typedef struct {
	uint16_t id;
	uint16_t bitfields;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
} DNSPktHeader;

#define DNSLIB_PKTHEADER_BITFIELD_QR BITF
#define DNSLIB_PKTHEADER_BITFIELD_OPCODE 0x7800
#define DNSLIB_PKTHEADER_BITFIELD_AA BITA
#define DNSLIB_PKTHEADER_BITFIELD_TC BIT9
#define DNSLIB_PKTHEADER_BITFIELD_RD BIT8
#define DNSLIB_PKTHEADER_BITFIELD_RA BIT7
#define DNSLIB_PKTHEADER_BITFIELD_RCODE 0x000F

#define DNSLIB_RCODE_NOERROR 0x0000
#define DNSLIB_RCODE_FMTERROR 0x0001
#define DNSLIB_RCODE_SRVFAIL 0x0002
#define DNSLIB_RCODE_NONAME 0x0003
#define DNSLIB_RCODE_REFUSED 0x0005

typedef struct {
	uint8_t *qname;
	uint16_t qtype;
	uint16_t qclass;
} DNSPktQuestion;

// QNAME individual label format
typedef struct {
	uint8_t size;
	uint8_t *label;
} DNSPktQname;

typedef struct {
	uint8_t *name;
	uint16_t type;
	uint16_t class;
	uint16_t ttl;
	uint16_t rdlength;
	uint8_t *rdata;
} DNSPktAnswer;

#define DNSLIB_TYPE_A 0x0001
#define DNSLIB_TYPE_NS 0x0002
#define DNSLIB_TYPE_CNAME 0x0005
#define DNSLIB_TYPE_SOA 0x0006
#define DNSLIB_TYPE_WKS 0x000B
#define DNSLIB_TYPE_PTR 0x000C
#define DNSLIB_TYPE_HINFO 0x000D
#define DNSLIB_TYPE_MINFO 0x000E
#define DNSLIB_TYPE_MX 0x000F
#define DNSLIB_TYPE_TXT 0x0010
#define DNSLIB_CLASS_IN 0x0001

// RDATA format for MX record
typedef struct {
	uint16_t preference;
	uint8_t *exchange;
} DNSPktMX;

/* Special dnslib-specific errno for troubleshooting */
#define DNSLIB_ERRNO_RECVFAULT 1
#define DNSLIB_ERRNO_DSTPORTFAULT 2
#define DNSLIB_ERRNO_PKTSIZEFAULT 3
#define DNSLIB_ERRNO_REPLYNOTQR 4
#define DNSLIB_ERRNO_REPLYIDMISMATCH 5
#define DNSLIB_ERRNO_REPLYNOANSWERS 6
#define DNSLIB_ERRNO_REPLYRCODE 7
#define DNSLIB_ERRNO_ARPTIMEOUT 8
#define DNSLIB_ERRNO_BADNAME 9
#define DNSLIB_ERRNO_NOREPLY 10
#define DNSLIB_ERRNO_NOTARECORD 11
#define DNSLIB_ERRNO_NOTINETCLASS 12
#define DNSLIB_ERRNO_REPLYINVALIDSIZE 13
#define DNSLIB_ERRNO_PARSE_TRUNCATED 14
#define DNSLIB_ERRNO_SENDTOFAULT 15
#define DNSLIB_ERRNO_BINDFAULT 16

#define DNSLIB_ERRNO_MAX 16

/* Functions */
char *dnslib_strerror(int);  // Return a const char * pointer describing the named error.

int dnslib_send_qname(int sockfd, const char *dnsname, uint16_t qtype, uint16_t qclass);  // Internal
int dnslib_recv_qname(int sockfd, char *dnsname, uint16_t maxlen);  // Internal
int dnslib_flush_qname(int sockfd);

int dnslib_gethostbyname(char *dnsname, uint16_t *ip);  // Resolve hostname to IP address -- A records only!

/* Globals */
extern volatile uint16_t dnslib_errno;
extern uint16_t *dnslib_resolver;  // Must be configured by your app to point to a DNS resolver IP


#endif
