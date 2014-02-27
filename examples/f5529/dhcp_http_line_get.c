#include <msp430.h>
#include "clockinit.h"

#include <stdint.h>
#include <string.h>
#include "dnslib.h"
#include "dhcplib.h"
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"
#include "w5200_debug.h"

volatile int res1, res2;
char *strerror(int);

#define HTTP_HOST "spirilis.net"

const char http_req1[] = { "GET / HTTP/1.0\r\nHost: spirilis.net\r\n\r\n" };

int main() {
	int sockfd, i;
	uint8_t netbuf[128];
	uint16_t myip[2], dns[2];

	WDTCTL = WDTPW | WDTHOLD;
        ucs_clockinit(16000000, 1, 0);
        __delay_cycles(160000);

	wiznet_debug_init();
	wiznet_debug_printf("Init-\n");
	if ( (sockfd = wiznet_init()) < 0) {  // borrowing 'sockfd' for storing return value
		wiznet_debug_printf("FAILED: %s\n", strerror(sockfd));
		LPM4;
	}
	w52_portoffset = 0;
	wiznet_mac_str_w_reg(W52_SOURCEMAC, "54:52:00:05:F8:2D");

	while (wiznet_phystate() < 0)
		__delay_cycles(160000);

	dns[0] = dns[1] = 0xFFFF;
	i = dhcp_loop_configure(dns);
	if (i < 0) {
		wiznet_debug_printf("dhcp_loop_configure_error: %s\ndhcplib_errno: %s\n", strerror(i), dhcp_strerror(dhcplib_errno));
		LPM4;
	}

        dnslib_resolver = dns;
	wiznet_debug_printf("DHCP completed:\n");
	wiznet_debug_dumpregs_main();
	if (dns[0] == 0xFFFF || dns[1] == 0xFFFF) {
		wiznet_debug_printf("DHCP completed but DNS not configured.\n");
		LPM4;
	}

	sockfd = wiznet_socket(IPPROTO_TCP);
	if (sockfd < 0) {
		wiznet_debug_printf("wiznet_socket failed: %s\n", strerror(sockfd));
		LPM4;
	}
	wiznet_debug_printf("wiznet_socket(): %d\n", sockfd);

	if (dnslib_gethostbyname(HTTP_HOST, myip) < 0) {
		wiznet_debug_printf("dnslib_gethostbyname failed: %s\n", dnslib_strerror(dnslib_errno));
		LPM4;
	}

	wiznet_binary2ip(myip, netbuf);
	wiznet_debug_printf("connect(%s):\n", netbuf);
	res1 = wiznet_connect(sockfd, myip, 80);  // Bind port 80
	if (res1 < 0) {
		wiznet_debug_printf("wiznet_connect failed: %s\n", strerror(res1));
		wiznet_debug_dumpregs_sock(sockfd);
		LPM4;
	}

	wiznet_debug_printf("sending: %s\n", http_req1);
	res1 = wiznet_send(sockfd, (char*)http_req1, strlen(http_req1), 1);
	if (res1 < 0) {
		wiznet_debug_printf("send error: %s\n", strerror(res1));
		wiznet_debug_dumpregs_sock(sockfd);
		LPM4;
	}

	wiznet_debug_printf("recv loop:\n");
	while(1) {
		while ( (res1 = wiznet_search_recv(sockfd, netbuf, 126, '\n', 1)) > 0 ) {
			netbuf[res1] = 0;
			if (netbuf[res1-1] == '\n' && netbuf[res1-2] != '\r') {  // Add CR before printing, if necessary.
				netbuf[res1-1] = '\r';
				netbuf[res1] = '\n';
				netbuf[res1+1] = '\0';
			}
			wiznet_debug_printf("line (len=%d): %s", res1, netbuf);
		}
		if (res1 != -EAGAIN) {
			wiznet_debug_printf("recv error: %s\n", strerror(res1));
			wiznet_debug_dumpregs_sock(sockfd);
			LPM4;
		}

		if (wiznet_irq_getsocket() == -EAGAIN && !wiznet_recvsize(sockfd))
			LPM0;  // recv = -EAGAIN means we wait for more.
	}

	return 0;
}

char *strerror(int errno)
{
	switch (errno) {
		case -EBADF:
			return (char *)"Bad file descriptor";
			break;
		case -ENETDOWN:
			return (char *)"Network disconnected";
			break;
		case -ENOTCONN:
			return (char *)"Not connected";
			break;
		case -ENFILE:
			return (char *)"Request too big for socket buffer";
			break;
		case -ETIMEDOUT:
			return (char *)"Connection timed out";
			break;
		case -ECONNABORTED:
			return (char *)"Connection closed by remote host";
			break;
		case -EPROTONOSUPPORT:
			return (char *)"Protocol not supported";
			break;
	}
	return (char *)"UNKNOWN";
}

#pragma vector = PORT2_VECTOR
__interrupt void P2_IRQ (void) {
	if(P2IFG & W52_IRQ_PORTBIT){
		w5200_irq |= 1;
		__bic_SR_register_on_exit(LPM4_bits);    // Wake up
		P2IFG &= ~W52_IRQ_PORTBIT;   // Clear interrupt flag
	}
}
