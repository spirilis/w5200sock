/* w5200.c
 * WizNet W5200 Ethernet Controller Driver for MSP430
 * High-level Support I/O Library
 * Dynamic Host Configuration Protocol
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
#include "dhcplib.h"
#include <stdlib.h>
#include <string.h>
#include "w5200_io.h"
#include "w5200_debug.h"

/* DHCPlib-specific errno & descriptions */
int dhcplib_errno;

const char *dhcplib_errno_descriptions[] = {
	"DHCPlib wiznet_socket() Fault",
	"DHCPlib wiznet_bind() UDP Fault",
	"DHCPDISCOVER Send Fault",
	"DHCPOFFER recvfrom Fault",
	"DHCPOFFER UDP Srcport Invalid",
	"DHCPOFFER DHCP Header Read Fault",
	"DHCPOFFER Preamble OP Field Invalid",
	"DHCPOFFER Read First Option Fault",
	"DHCPREQUEST Send Fault",
	"DHCPACK recvfrom Fault",
	"DHCPACK UDP Srcport Invalid",
	"DHCPACK DHCP Header Read Fault",
	"DHCPACK Preamble OP Field Invalid",
	"DHCPACK Read First Option Fault",
	"DHCPACK Addresses Do Not Match DHCPOFFER"
};


/* Functions */

char *dhcp_strerror(int errno)
{
	if (errno < 1 || errno > DHCP_ERRNO_MAX)
		return (char *)" ";

	return (char *)dhcplib_errno_descriptions[errno-1];
}

// DHCP header preamble interpretation
int dhcp_write_preamble(int sockfd, DHCPHeaderPreamble *preamble)
{
	uint8_t header[12];

	header[0] = preamble->op;
	header[1] = preamble->htype;
	header[2] = preamble->hlen;
	header[3] = preamble->hops;
	wiznet_htons(preamble->xid0, header+4);
	wiznet_htons(preamble->xid1, header+6);
	wiznet_htons(preamble->secs, header+8);
	wiznet_htons(preamble->flags, header+10);

	return wiznet_sendto(sockfd, header, 12, NULL, 0, 0);
}

int dhcp_read_preamble(int sockfd, DHCPHeaderPreamble *preamble)
{
	uint8_t header[12];
	int ret;

	ret = wiznet_recv(sockfd, header, 12, 0);
	if (ret < 0)
		return ret;

	preamble->op = header[0];
	preamble->htype = header[1];
	preamble->hlen = header[2];
	preamble->hops = header[3];
	preamble->xid0 = wiznet_ntohs(header+4);
	preamble->xid1 = wiznet_ntohs(header+6);
	preamble->secs = wiznet_ntohs(header+8);
	preamble->flags = wiznet_ntohs(header+10);
	return ret;
}

// Write initial DHCP information
int dhcp_write_header(int sockfd, DHCPHeaderPreamble *preamble, uint16_t *ciaddr, uint16_t *yiaddr, uint16_t *siaddr, uint16_t *giaddr, uint16_t *chaddr)
{
	uint8_t scratch[6];
	int ret;

	dhcp_write_preamble(sockfd, preamble);
	wiznet_htons(ciaddr[0], scratch);
	wiznet_htons(ciaddr[1], scratch+2);
	if ( (ret = wiznet_sendto(sockfd, scratch, 4, NULL, 0, 0)) < 0 )   // CIADDR
		return ret;

	wiznet_htons(yiaddr[0], scratch);
	wiznet_htons(yiaddr[1], scratch+2);
	if ( (ret = wiznet_sendto(sockfd, scratch, 4, NULL, 0, 0)) < 0 )   // YIADDR
		return ret;

	wiznet_htons(siaddr[0], scratch);
	wiznet_htons(siaddr[1], scratch+2);
	if ( (ret = wiznet_sendto(sockfd, scratch, 4, NULL, 0, 0)) < 0 )   // SIADDR
		return ret;

	wiznet_htons(giaddr[0], scratch);
	wiznet_htons(giaddr[1], scratch+2);
	if ( (ret = wiznet_sendto(sockfd, scratch, 4, NULL, 0, 0)) < 0 )   // GIADDR
		return ret;

	wiznet_htons(chaddr[0], scratch);
	wiznet_htons(chaddr[1], scratch+2);
	wiznet_htons(chaddr[2], scratch+4);
	if ( (ret = wiznet_sendto(sockfd, scratch, 6, NULL, 0, 0)) < 0 )   // CHADDR
		return ret;
	// Fill zeroes to pad CHADDR
	wiznet_fill_txbuf(sockfd, 192+10, 0x00);

	// Magic Cookie
	wiznet_htons(DHCP_MAGIC_COOKIE_0, scratch);
	wiznet_htons(DHCP_MAGIC_COOKIE_1, scratch+2);
	if ( (ret = wiznet_sendto(sockfd, scratch, 4, NULL, 0, 0)) < 0 )   // DHCP Magic Cookie
		return ret;

	return 0;
}

// Read initial DHCP information
int dhcp_read_header(int sockfd, DHCPHeaderPreamble *preamble, uint16_t *ciaddr, uint16_t *yiaddr, uint16_t *siaddr, uint16_t *giaddr, uint16_t *chaddr)
{
	uint8_t scratch[6];
	int ret;

	#if WIZNET_DEBUG > 2
	const char *funcname = "dhcp_read_header()";
	#endif

	dhcp_read_preamble(sockfd, preamble);

	if ( (ret = wiznet_recv(sockfd, scratch, 4, 0)) < 0 )   // CIADDR
		return ret;
	ciaddr[0] = wiznet_ntohs(scratch);
	ciaddr[1] = wiznet_ntohs(scratch+2);

	if ( (ret = wiznet_recv(sockfd, scratch, 4, 0)) < 0 )   // YIADDR
		return ret;
	yiaddr[0] = wiznet_ntohs(scratch);
	yiaddr[1] = wiznet_ntohs(scratch+2);

	if ( (ret = wiznet_recv(sockfd, scratch, 4, 0)) < 0 )   // SIADDR
		return ret;
	siaddr[0] = wiznet_ntohs(scratch);
	siaddr[1] = wiznet_ntohs(scratch+2);

	if ( (ret = wiznet_recv(sockfd, scratch, 4, 0)) < 0 )   // GIADDR
		return ret;
	giaddr[0] = wiznet_ntohs(scratch);
	giaddr[1] = wiznet_ntohs(scratch+2);

	if ( (ret = wiznet_recv(sockfd, scratch, 6, 0)) < 0 )   // CHADDR
		return ret;
	chaddr[0] = wiznet_ntohs(scratch);
	chaddr[1] = wiznet_ntohs(scratch+2);
	chaddr[2] = wiznet_ntohs(scratch+4);

	if ( (ret = wiznet_flush(sockfd, 192+10, 0)) < 0 )     // Flush superfluous 0's
		return ret;

	if ( (ret = wiznet_recv(sockfd, scratch, 4, 0)) < 0 )   // Magic Cookie
		return ret;

	// Magic Cookie
	if (wiznet_ntohs(scratch) != DHCP_MAGIC_COOKIE_0 ||
		wiznet_ntohs(scratch+2) != DHCP_MAGIC_COOKIE_1) {
		wiznet_debug3_printf("%s: Read header with invalid magic cookie (%x%x)\n", funcname, wiznet_ntohs(scratch), wiznet_ntohs(scratch+2));
		return -1;
	}

	return 0;
}

int dhcp_write_option(int sockfd, uint8_t option, uint8_t len, void *buf)
{
	uint8_t opthdr[2];
	int ret;

	opthdr[0] = option;
	opthdr[1] = len;
	if ( (ret = wiznet_sendto(sockfd, opthdr, 2, NULL, 0, 0)) < 0 )
		return ret;
	
	return wiznet_sendto(sockfd, (uint8_t *)buf, len, NULL, 0, 0);
}

int dhcp_read_option(int sockfd, uint8_t *option, uint8_t *len, void *buf)
{
	uint8_t opthdr[2];
	int ret;

	#if WIZNET_DEBUG > 1
	const char *funcname = "dhcp_read_option()";
	#endif

	if ( (ret = wiznet_recv(sockfd, opthdr, 2, 0)) < 0 ) {
		wiznet_debug2_printf("%s: Error %d reading 2 byte option header\n", funcname, ret);
		return ret;
	}
	*option = opthdr[0];
	*len = opthdr[1];
	wiznet_debug3_printf("%s: optcode=%u, optlen=%u\n", funcname, opthdr[0], opthdr[1]);
	return wiznet_recv(sockfd, (uint8_t *)buf, opthdr[1], 0);
}

int dhcp_send_dhcpdiscover(int sockfd)
{
	DHCPHeaderPreamble hdr;
	uint16_t ipzero[2], ipone[2], ourmac[3];
	uint8_t scratch[8];
	int ret;

	#if WIZNET_DEBUG > 1
	const char *funcname = "dhcp_send_dhcpdiscover()";
	#endif

	ipzero[0] = ipzero[1] = 0x0000;
	wiznet_mac_bin_r_reg(W52_SOURCEMAC, ourmac);

	wiznet_debug3_printf("%s: Our MAC = %x%x%x\n", funcname, ourmac[0], ourmac[1], ourmac[2]);

	hdr.op = 0x01;
	hdr.htype = 0x01;
	hdr.hlen = 0x06;
	hdr.hops = 0x00;
	hdr.xid0 = 0x3903;
	hdr.xid1 = 0xF326;
	hdr.secs = 0x0000;
	hdr.flags = 0x0000;

	// Write UDP preamble with source IP/port going into registers, dest IP/port sent using wiznet_sendto()
	wiznet_ip_bin_w_reg(W52_SOURCEIP, ipzero);
	wiznet_w_sockreg16(sockfd, W52_SOCK_SRCPORT, 68);
	ipone[0] = ipone[1] = 0xFFFF;
	ret = wiznet_sendto(sockfd, NULL, 0, ipone, 67, 0);
	if (ret < 0) {
		wiznet_debug2_printf("%s: Error %d preparing UDP packet\n", funcname, ret);
		return ret;
	}

	ret = dhcp_write_header(sockfd, &hdr, ipzero, ipzero, ipzero, ipzero, ourmac);
	if (ret < 0) {
		wiznet_debug2_printf("%s: Error %d writing DHCP Header\n", funcname, ret);
		return ret;
	}
	/* Write options */
	
	// DHCP option 53: DHCPDISCOVER
	scratch[0] = DHCP_MSGTYPE_DHCPDISCOVER;
	if ( (ret = dhcp_write_option(sockfd, DHCP_OPTCODE_DHCP_MSGTYPE, 1, scratch)) < 0 ) {
		wiznet_debug2_printf("%s: Error %d writing DHCP MSGTYPE=DHCPDISCOVER option\n", funcname, ret);
		return ret;
	}

	// Option 55, parameter request list.
	// Subnetmask, Router, Domain Name Server
	scratch[0] = DHCP_OPTCODE_SUBNET_MASK;
	scratch[1] = DHCP_OPTCODE_ROUTER;
	scratch[2] = DHCP_OPTCODE_DOMAIN_SERVER;
	if ( (ret = dhcp_write_option(sockfd, DHCP_OPTCODE_PARAM_LIST, 3, scratch)) < 0 ) {
		wiznet_debug2_printf("%s: Error %d writing DHCP OPTCODE PARAM LIST option\n", funcname, ret);
		return ret;
	}
	
	if ( (ret = dhcp_write_option(sockfd, DHCP_OPTCODE_END, 0, NULL)) < 0 ) {
		wiznet_debug2_printf("%s: Error %d writing DHCP OPTCODE END option\n", funcname, ret);
		return ret;
	}
	if ( (ret = wiznet_txcommit(sockfd)) < 0 ) {
		wiznet_debug2_printf("%s: Error %d committing DHCPDISCOVER packet\n", funcname, ret);
		return ret;
	}


	return 0;
}

int dhcp_send_dhcprequest(int sockfd, uint16_t *yiaddr, uint16_t *dhcpserver)
{
	DHCPHeaderPreamble hdr;
	uint16_t ipzero[2], ipone[2], ourmac[3];
	uint8_t scratch[8];
	int ret;

	#if WIZNET_DEBUG > 1
	const char *funcname = "dhcp_send_dhcprequest()";
	#endif

	ipzero[0] = ipzero[1] = 0x0000;
	wiznet_mac_bin_r_reg(W52_SOURCEMAC, ourmac);

	wiznet_debug3_printf("%s: Our MAC = %x%x%x\n", funcname, ourmac[0], ourmac[1], ourmac[2]);

	hdr.op = 0x01;
	hdr.htype = 0x01;
	hdr.hlen = 0x06;
	hdr.hops = 0x00;
	hdr.xid0 = 0x3903;
	hdr.xid1 = 0xF326;
	hdr.secs = 0x0000;
	hdr.flags = 0x0000;

	// Write UDP preamble with source IP/port going into registers, dest IP/port sent using wiznet_sendto()
	wiznet_ip_bin_w_reg(W52_SOURCEIP, ipzero);
	wiznet_w_sockreg16(sockfd, W52_SOCK_SRCPORT, 68);
	ipone[0] = ipone[1] = 0xFFFF;
	ret = wiznet_sendto(sockfd, NULL, 0, ipone, 67, 0);
	if (ret < 0) {
		wiznet_debug2_printf("%s: Error %d preparing UDP packet\n", funcname, ret);
		return ret;
	}

	ret = dhcp_write_header(sockfd, &hdr, ipzero, ipzero, dhcpserver, ipzero, ourmac);
	if (ret < 0) {
		wiznet_debug2_printf("%s: Error %d writing DHCP Header\n", funcname, ret);
		return ret;
	}
	/* Write options */
	
	// DHCP Option 53: DHCP REQUEST
	scratch[0] = DHCP_MSGTYPE_DHCPREQUEST;
	if ( (ret = dhcp_write_option(sockfd, DHCP_OPTCODE_DHCP_MSGTYPE, 1, scratch)) < 0 ) {
		wiznet_debug2_printf("%s: Error %d writing DHCP MSGTYPE=DHCPREQUEST option\n", funcname, ret);
		return ret;
	}
	
	// DHCP Option 50: Request IP address
	wiznet_htons(yiaddr[0], scratch);
	wiznet_htons(yiaddr[1], scratch+2);
	if ( (ret = dhcp_write_option(sockfd, DHCP_OPTCODE_IP_ADDRESS_REQUEST, 4, scratch)) < 0 ) {
		wiznet_debug2_printf("%s: Error %d writing DHCP IP ADDRESS REQUEST option\n", funcname, ret);
		return ret;
	}
	
	// DHCP Option 54: DHCP server IP
	wiznet_htons(dhcpserver[0], scratch);
	wiznet_htons(dhcpserver[1], scratch+2);
	if ( (ret = dhcp_write_option(sockfd, DHCP_OPTCODE_DHCP_SERVER_IP, 4, scratch)) < 0 ) {
		wiznet_debug2_printf("%s: Error %d writing DHCP SERVER IP option\n", funcname, ret);
		return ret;
	}
	
	if ( (ret = dhcp_write_option(sockfd, DHCP_OPTCODE_END, 0, NULL)) < 0 ) {
		wiznet_debug2_printf("%s: Error %d writing DHCP OPTCODE END option\n", funcname, ret);
		return ret;
	}
	if ( (ret = wiznet_txcommit(sockfd)) < 0 ) {
		wiznet_debug2_printf("%s: Error %d committing DHCPREQUEST packet\n", funcname, ret);
		return ret;
	}
	
	return 0;
}

/* Main event loop: Initialize UDP socket, perform DHCP communication, configure Wiznet IP information to match.
 * Returns 0 if success, -1 * errno code if error.
 *
 * Optionally stash DNS server in dnsaddr if do_report_dns is > 0.
 * Configure timeout in dhcplib.h via #define DHCP_LOOP_COUNT_TIMEOUT
 */
int dhcp_loop_configure(uint16_t *dnsaddr)
{
	int sockfd, ret, exitval = 1;
	uint8_t scratch[32], state = 0, optcode, optlen;
	uint16_t yiaddr[2], yiaddr_ack[2], siaddr_ack[2], siaddr[2], giaddr[2], ipzero[2], subnetmask[2], dns[2];
	uint16_t loopcount=0, srcport;
	DHCPHeaderPreamble pamb;

	#if WIZNET_DEBUG > 0
	const char *funcname = "dhcp_loop_configure()";
	uint8_t did_report_dhcpoffer=0, did_report_dhcpack=0;
	#endif

	// Variable prep
	dns[0] = dns[1] = 0xFFFF;
	yiaddr[0] = yiaddr[1] = 0x0000;
	siaddr[0] = siaddr[1] = 0x0000;
	giaddr[0] = giaddr[1] = 0x0000;
	subnetmask[0] = subnetmask[1] = 0xFFFF;
	dhcplib_errno = 0;

	// Init socket
	sockfd = wiznet_socket(IPPROTO_UDP);
	if (sockfd < 0) {
		dhcplib_errno = DHCP_ERRNO_CANNOT_ALLOCATE_SOCKET;
		return sockfd;
	}
	
	if ( (ret = wiznet_bind(sockfd, 68)) < 0 ) {
		wiznet_close(sockfd);
		dhcplib_errno = DHCP_ERRNO_CANNOT_BIND_UDP;
		return ret;
	}

	// Main event loop with timeout support
	while (loopcount < DHCP_LOOP_COUNT_TIMEOUT && exitval == 1) {  // exitval=1 means keep looping, 0 means we're done
		switch (state) {
			case 0:
				// Send DHCPDISCOVER
				wiznet_debug2_printf("%s: Sending DHCPDISCOVER\n", funcname);
				if ( (ret = dhcp_send_dhcpdiscover(sockfd)) < 0 ) {
					dhcplib_errno = DHCP_ERRNO_DHCPDISCOVER_FAULT;
					wiznet_debug2_printf("%s: Faulted with %d\n", funcname, ret);
					exitval = ret;
					continue;
				}
				state = 1;
				break;

			case 1:
				// Check for DHCPOFFER
				#if WIZNET_DEBUG > 0
				if (!did_report_dhcpoffer) {
					wiznet_debug2_printf("%s: Waiting for DHCPOFFER\n", funcname);
					did_report_dhcpoffer = 1;
				}
				#endif

				if (wiznet_recvsize(sockfd)) {
					wiznet_debug3_printf("%s: Packet received\n", funcname);
					if ( (ret = wiznet_recvfrom(sockfd, NULL, 0, ipzero, &srcport, 0)) < 0 ) {
						dhcplib_errno = DHCP_ERRNO_DHCPOFFER_RECVFROM_FAULT;
						exitval = ret;
						wiznet_debug2_printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
						continue;
					}
					if (srcport != 67) {
						dhcplib_errno = DHCP_ERRNO_DHCPOFFER_SRCPORT_INVALID;
						exitval = -EFAULT;
						wiznet_debug2_printf("%s: %s (found %u)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], srcport);
						continue;
					}
					// Read DHCP header
					if ( (ret = dhcp_read_header(sockfd, &pamb, ipzero, yiaddr, siaddr, ipzero, (uint16_t *)scratch)) < 0 ) {
						dhcplib_errno = DHCP_ERRNO_DHCPOFFER_READHEADER_FAULT;
						exitval = ret;
						wiznet_debug2_printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
						continue;
					}
					// Check OP field; should be 0x02, else this isn't a server->client reply!
					if (pamb.op != 0x02) {
						wiznet_flush(sockfd, 10000, 1);  // Flush packet & signal for more to come.
						dhcplib_errno = DHCP_ERRNO_DHCPOFFER_PREAMBLE_OP_INVALID;
						wiznet_debug2_printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
						continue;
					}
					// Read first option; should be option 53 = DHCPOFFER
					if ( (ret = dhcp_read_option(sockfd, &optcode, &optlen, scratch)) < 0 ) {
						dhcplib_errno = DHCP_ERRNO_DHCPOFFER_READOPT_FAULT;
						exitval = ret;
						wiznet_debug2_printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
						continue;
					}
					if (optcode == DHCP_OPTCODE_DHCP_MSGTYPE && optlen == 1 && scratch[0] == DHCP_MSGTYPE_DHCPOFFER) {
						wiznet_debug3_printf("%s: Packet is DHCPOFFER\n", funcname);

						// Read all headers to extract info
						do {
							dhcp_read_option(sockfd, &optcode, &optlen, scratch);
							switch (optcode) {
								case DHCP_OPTCODE_SUBNET_MASK:
									if (optlen == 4) {
										subnetmask[0] = wiznet_ntohs(scratch);
										subnetmask[1] = wiznet_ntohs(scratch+2);
										wiznet_debug2_printf("%s: subnetmask: %u.%u.%u.%u\n", funcname, subnetmask[0] >> 8, subnetmask[0] & 0xFF, subnetmask[1] >> 8, subnetmask[1] & 0xFF);
									}
									break;
								case DHCP_OPTCODE_ROUTER:
									if (optlen == 4) {
										giaddr[0] = wiznet_ntohs(scratch);
										giaddr[1] = wiznet_ntohs(scratch+2);
										wiznet_debug2_printf("%s: gateway: %u.%u.%u.%u\n", funcname, giaddr[0] >> 8, giaddr[0] & 0xFF, giaddr[1] >> 8, giaddr[1] & 0xFF);
									}
									break;
								case DHCP_OPTCODE_DHCP_SERVER_IP:
									if (optlen == 4) {
										siaddr[0] = wiznet_ntohs(scratch);
										siaddr[1] = wiznet_ntohs(scratch+2);
										wiznet_debug2_printf("%s: DHCP server: %u.%u.%u.%u\n", funcname, siaddr[0] >> 8, siaddr[0] & 0xFF, siaddr[1] >> 8, siaddr[1] & 0xFF);
									}
									break;
								case DHCP_OPTCODE_DOMAIN_SERVER:
									if (optlen >= 4) {  // Only extract the first DNS server.
										dns[0] = wiznet_ntohs(scratch);
										dns[1] = wiznet_ntohs(scratch+2);
										wiznet_debug2_printf("%s: DNS server: %u.%u.%u.%u (%u in total)\n", funcname, dns[0] >> 8, dns[0] & 0xFF, dns[1] >> 8, dns[1] & 0xFF, optlen/4);
									}
							}
						} while (optcode != DHCP_OPTCODE_END);
						wiznet_flush(sockfd, 10000, 1);  // Flush any remaining bytes (should be none), signal more may come.
						state = 2;
					} else {
						// Not a DHCPOFFER; flush packet and signal more may come, then loop around.
						wiznet_flush(sockfd, 10000, 1);
						wiznet_debug3_printf("%s: Packet is not DHCPOFFER; MSGTYPE=%u\n", funcname, scratch[0]);
					}
				}
				break;  // If there is no data waiting, loop will just continue another round.

			case 2:  // Send DHCPREQUEST (officially requesting the lease)
				wiznet_debug2_printf("%s: Sending DHCPREQUEST to %u.%u.%u.%u requesting IP=%u.%u.%u.%u\n", funcname,
					siaddr[0] >> 8, siaddr[0] & 0xFF, siaddr[1] >> 8, siaddr[1] & 0xFF,
					yiaddr[0] >> 8, yiaddr[0] & 0xFF, yiaddr[1] >> 8, yiaddr[1] & 0xFF);

				if ( (ret = dhcp_send_dhcprequest(sockfd, yiaddr, siaddr)) < 0 ) {
					dhcplib_errno = DHCP_ERRNO_DHCPREQUEST_FAULT;
					exitval = ret;
					wiznet_debug2_printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
					continue;
				}
				state = 3;
				break;

			case 3:  // Receive DHCPACK (officially acknowledgement that the lease is ours)
				#if WIZNET_DEBUG > 0
				if (!did_report_dhcpack) {
					wiznet_debug2_printf("%s: Waiting for DHCPACK\n", funcname);
					did_report_dhcpack = 1;
				}
				#endif

				if (wiznet_recvsize(sockfd)) {
					wiznet_debug3_printf("%s: Packet received\n", funcname);
					if ( (ret = wiznet_recvfrom(sockfd, NULL, 0, ipzero, &srcport, 0)) < 0 ) {
						dhcplib_errno = DHCP_ERRNO_DHCPACK_RECVFROM_FAULT;
						exitval = ret;
						wiznet_debug2_printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
						continue;
					}
					if (srcport != 67) {
						dhcplib_errno = DHCP_ERRNO_DHCPACK_SRCPORT_INVALID;
						exitval = -EFAULT;
						wiznet_debug2_printf("%s: %s (found %u)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], srcport);
						continue;
					}
					// Read DHCP header
					if ( (ret = dhcp_read_header(sockfd, &pamb, ipzero, yiaddr_ack, siaddr_ack, ipzero, (uint16_t *)scratch)) < 0 ) {
						dhcplib_errno = DHCP_ERRNO_DHCPACK_READHEADER_FAULT;
						exitval = ret;
						wiznet_debug2_printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
						continue;
					}
					// Check OP field; should be 0x02, else this isn't a server->client reply!
					if (pamb.op != 0x02) {
						dhcplib_errno = DHCP_ERRNO_DHCPACK_PREAMBLE_OP_INVALID;
						wiznet_flush(sockfd, 10000, 1);  // Flush packet & signal for more to come.
						wiznet_debug2_printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
						continue;
					}
					// Read first option; should be option 53 = DHCPACK
					if ( (ret = dhcp_read_option(sockfd, &optcode, &optlen, scratch)) < 0 ) {
						dhcplib_errno = DHCP_ERRNO_DHCPACK_READOPT_FAULT;
						exitval = ret;
						wiznet_debug2_printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
						continue;
					}
					if (optcode == DHCP_OPTCODE_DHCP_MSGTYPE && optlen == 1 && scratch[0] == DHCP_MSGTYPE_DHCPACK) {
						wiznet_debug3_printf("%s: Packet is DHCPACK\n", funcname);

						// Read all headers to extract info
						do {
							dhcp_read_option(sockfd, &optcode, &optlen, scratch);

							switch (optcode) {
								case DHCP_OPTCODE_DHCP_SERVER_IP:
									if (optlen >= 4) {  // Only extract the first DNS server.
										siaddr_ack[0] = wiznet_ntohs(scratch);
										siaddr_ack[1] = wiznet_ntohs(scratch+2);
										/* Using debug3 here because we're only reading the DHCP_SERVER_IP option
										 * from DHCPACK merely to validate that we received an ACK from the correct server.
										 */
										wiznet_debug3_printf("%s: DHCP server: %u.%u.%u.%u\n", funcname, siaddr_ack[0] >> 8, siaddr_ack[0] & 0xFF, siaddr_ack[1] >> 8, siaddr_ack[1] & 0xFF);
									}
							}
						} while (optcode != DHCP_OPTCODE_END);
						wiznet_flush(sockfd, 10000, 1);  // Flush any remaining bytes (should be none), signal more may come.

						wiznet_debug3_printf("%s: DHCPACK contents processed; validating:\n", funcname);
						// Validate; does everything match up?
						if (yiaddr[0] != yiaddr_ack[0] || yiaddr[1] != yiaddr_ack[1] ||
							siaddr[0] != siaddr_ack[0] || siaddr[1] != siaddr_ack[1]) {
							dhcplib_errno = DHCP_ERRNO_DHCPACK_ADDRESSES_DO_NOT_MATCH;
							wiznet_debug2_printf("%s: %s\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1]);
							// Invalid ACK; wait for another ACK
							continue;
						} else {
							wiznet_debug3_printf("%s: Valid DHCPACK to our DHCPREQUEST.\n", funcname);
						}

						if (dns[0] != 0xFFFF && dns[1] != 0xFFFF) {
							// Is the user requesting that DNS be reported back?
							if (dnsaddr != NULL) {
								dnsaddr[0] = dns[0];
								dnsaddr[1] = dns[1];
								wiznet_debug2_printf("%s: DNS requested; copying to user pointer\n", funcname);
							}
						}
						// Set IP, gateway, router!
						wiznet_debug1_printf("%s: Configuring WizNet IP settings\n", funcname);

						wiznet_ip_bin_w_reg(W52_GATEWAY, giaddr);
						wiznet_ip_bin_w_reg(W52_SUBNETMASK, subnetmask);
						wiznet_ip_bin_w_reg(W52_SOURCEIP, yiaddr);
						exitval = 0;  // All done!
						state = 4;
						break;
					} else {
						// Not a DHCPACK; flush packet and signal more may come, then loop around.
						wiznet_flush(sockfd, 10000, 1);
						wiznet_debug3_printf("%s: Packet is not DHCPACK; MSGTYPE=%u\n", funcname, scratch[0]);
					}
				}
				break;  // If there is no data waiting, loop will just continue another round.
		}
		__delay_cycles(250000);  // 1/100sec at 25MHz (1/64sec at 16MHz)
		loopcount++;
	}

	if (loopcount >= DHCP_LOOP_COUNT_TIMEOUT) {
		wiznet_debug3_printf("%s: Loop count %u exceeds max of %u\n", funcname, loopcount, DHCP_LOOP_COUNT_TIMEOUT);
		wiznet_debug1_printf("%s: TIMEOUT\n", funcname);
		exitval = -ETIMEDOUT;
	}
	
	wiznet_close(sockfd);
	return exitval;
}

