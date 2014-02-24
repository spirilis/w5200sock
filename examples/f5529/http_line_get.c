#include <msp430.h>
#include "clockinit.h"

#include <stdint.h>
#include <string.h>
#include "uartcli.h"
#include "uartdbg.h"
#include "dnslib.h"
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"

volatile int res1, res2;
char uartbuf[8];
void interpret_senderror(int);

#define HTTP_HOST "spirilis.net"

const char http_req1[] = { "GET / HTTP/1.0\r\nHost: spirilis.net\r\n\r\n" };

int main() {
	int sockfd;
	uint8_t netbuf[128];
	uint16_t myip[2], dns[2];

	WDTCTL = WDTPW | WDTHOLD;
        ucs_clockinit(16000000, 1, 0);
        __delay_cycles(160000);

	uartcli_begin(uartbuf, 8);
	uartcli_println_str("Init-");
	if ( (sockfd = wiznet_init()) < 0) {  // borrowing 'sockfd' for storing return value
		uartcli_print_str("FAILED: ");
		interpret_senderror(sockfd);
		LPM4;
	}

	while (wiznet_phystate() < 0)
		__delay_cycles(160000);

        wiznet_ip_bin_w_reg(W52_SUBNETMASK, w52_const_subnet_classC);
        wiznet_ip_str_w_reg(W52_SOURCEIP, "10.104.115.180");
        wiznet_ip_str_w_reg(W52_GATEWAY, "10.104.115.1");
        wiznet_ip2binary("10.104.8.201", dns);
        dnslib_resolver = dns;

	sockfd = wiznet_socket(IPPROTO_TCP);
	if (sockfd < 0) {
		uartcli_print_str("wiznet_socket failed: ");
		interpret_senderror(sockfd);
		LPM4;
	}
	uartcli_print_str("wiznet_socket(): "); uartcli_println_int(sockfd);

	if (dnslib_gethostbyname(HTTP_HOST, myip) < 0) {
		uartcli_print_str("dnslib_gethostbyname failed: ");
		uartcli_println_str(dnslib_strerror(dnslib_errno));
		LPM4;
	}

	wiznet_binary2ip(myip, netbuf);
	uartcli_print_str("connect("); uartcli_print_str(netbuf); uartcli_println_str("):");
	res1 = wiznet_connect(sockfd, myip, 80);  // Bind port 80
	if (res1 < 0) {
		uartcli_print_str("wiznet_connect failed: ");
		interpret_senderror(res1);
		wiznet_debug_uart(sockfd);
		LPM4;
	}

	uartcli_print_str("send: "); uartcli_println_str(http_req1);
	res1 = wiznet_send(sockfd, (char*)http_req1, strlen(http_req1), 1);
	if (res1 < 0) {
		interpret_senderror(res1);
		wiznet_debug_uart(sockfd);
		LPM4;
	}

	uartcli_println_str("recv loop:");
	while(1) {
		while ( (res1 = wiznet_search_recv(sockfd, netbuf, 126, '\n', 1)) > 0 ) {
			netbuf[res1] = 0;
			if (netbuf[res1-1] == '\n' && netbuf[res1-2] != '\r') {  // Add CR before printing, if necessary.
				netbuf[res1-1] = '\r';
				netbuf[res1] = '\n';
				netbuf[res1+1] = '\0';
			}
			uartcli_print_str("line: "); uartcli_print_int(res1); uartcli_print_str(" ");
			uartcli_print_str(netbuf);
		}
		if (res1 != -EAGAIN) {
			uartcli_print_str("recv error: ");
			interpret_senderror(res1);
			wiznet_debug_uart(sockfd);
			LPM4;
		}

		if (wiznet_irq_getsocket() == -EAGAIN && !wiznet_recvsize(sockfd))
			LPM0;  // recv = -EAGAIN means we wait for more.
	}

	return 0;
}

void interpret_senderror(int errno)
{
	switch (errno) {
		case -EBADF:
			uartcli_println_str("Bad file descriptor");
			break;
		case -ENETDOWN:
			uartcli_println_str("Network disconnected");
			break;
		case -ENOTCONN:
			uartcli_println_str("Not connected");
			break;
		case -ENFILE:
			uartcli_println_str("Request too big for socket buffer");
			break;
		case -ETIMEDOUT:
			uartcli_println_str("Connection timed out");
			break;
		case -ECONNABORTED:
			uartcli_println_str("Connection closed by remote host");
			break;
		case -EPROTONOSUPPORT:
			uartcli_println_str("Protocol not supported");
			break;
	}
}

#pragma vector = PORT2_VECTOR
__interrupt void P2_IRQ (void) {
	if(P2IFG & W52_IRQ_PORTBIT){
		w5200_irq |= 1;
		__bic_SR_register_on_exit(LPM4_bits);    // Wake up
		P2IFG &= ~W52_IRQ_PORTBIT;   // Clear interrupt flag
	}
}
