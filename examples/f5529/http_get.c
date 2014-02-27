#include <msp430.h>
#include "clockinit.h"

#include <stdint.h>
#include <string.h>
#include "dnslib.h"
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"
#include "w5200_debug.h"

volatile int res1, res2;
char *strerror(int);

#define HTTP_HOST "spirilis.net"

const char http_req1[] = { "GET / HTTP/1.0\r\nHost: spirilis.net\r\n\r\n" };

int main() {
	int sockfd;
	uint8_t netbuf[128];
	uint16_t myip[2], dns[2];

	WDTCTL = WDTPW | WDTHOLD;
        ucs_clockinit(16000000, 1, 0);
        __delay_cycles(160000);

	wiznet_debug_init();
	wiznet_init();

	while (wiznet_phystate() < 0)
		__delay_cycles(160000);

        wiznet_ip_bin_w_reg(W52_SUBNETMASK, w52_const_subnet_classC);
        wiznet_ip_str_w_reg(W52_SOURCEIP, "10.104.115.180");
        wiznet_ip_str_w_reg(W52_GATEWAY, "10.104.115.1");
        wiznet_ip2binary("10.104.8.201", dns);
        dnslib_resolver = dns;

	sockfd = wiznet_socket(IPPROTO_TCP);
	if (sockfd < 0)
		LPM4;
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
		wiznet_debug_printf("%s\n", strerror(res1));
		wiznet_debug_dumpregs_sock(sockfd);
		LPM4;
	}

	wiznet_debug_printf("recv loop:\n");
	while(1) {
		while ( (res1 = wiznet_recv(sockfd, netbuf, 127, 1)) > 0 ) {
			netbuf[res1] = 0;
			wiznet_debug_printf("%s", netbuf);
		}
		if (res1 != -EAGAIN) {
			wiznet_debug_printf("recv error: %s", strerror(res1));
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
