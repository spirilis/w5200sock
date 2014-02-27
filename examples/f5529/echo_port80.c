#include <msp430.h>
#include "clockinit.h"

#include <stdint.h>
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"
#include "w5200_debug.h"

// Using our new ip2binary routines for this...
//const uint16_t myip[] = { 0x0A68, 0x73B4 };  // 10.104.115.180
//const uint16_t myip[] = { 0xC0A8, 0x00E6 };  // 192.168.0.230
volatile int res1, res2;

#define RED_SET P1OUT|=BIT0
#define RED_CLEAR P1OUT&=~BIT0
#define GREEN_SET P4OUT|=BIT7
#define GREEN_CLEAR P4OUT&=~BIT7

int main() {
	int sockfd;
	uint8_t sockopen = 0;
	uint8_t netbuf[32];

	P1DIR |= BIT0;
	P4DIR |= BIT7;
	RED_SET; GREEN_CLEAR;

	WDTCTL = WDTPW | WDTHOLD;
        ucs_clockinit(16000000, 1, 0);
        __delay_cycles(160000);

	wiznet_debug_init();
	wiznet_init();

	while (!wiznet_phystate())
		__delay_cycles(160000);

	wiznet_ip_str_w_reg(W52_SOURCEIP, "10.104.115.180");
	wiznet_ip_bin_w_reg(W52_SUBNETMASK, w52_const_subnet_classC);

	sockfd = wiznet_socket(IPPROTO_TCP);
	if (sockfd < 0)
		LPM4;
	wiznet_debug_printf("wiznet_socket(): %d\n", sockfd);

	res1 = wiznet_bind(sockfd, 80);  // Bind port 80
	if (res1 < 0)
		LPM4;

	RED_CLEAR;
	while(1) {
		if (!sockopen) {
			res1 = wiznet_accept(sockfd);
			wiznet_debug_printf("accept: %d\n", res1);
			if (!res1 || res1 == -EISCONN)
				sockopen = 1;
		} else {
			res1 = wiznet_recv(sockfd, netbuf, 32, 1);
			wiznet_debug_printf("RECV: %d\n", res1);
			switch (res1) {
				case -ENOTCONN:
				case -ECONNABORTED:
				case -ENETDOWN:
					sockopen = 0;
					break;
				case -EAGAIN:
					break;
				default:
					wiznet_debug_printf("sending;\n");
					switch (res2 = wiznet_send(sockfd, netbuf, res1, 1)) {
						case -ENOTCONN:
						case -ECONNABORTED:
						case -ETIMEDOUT:
						case -ENETDOWN:
							wiznet_debug_printf("send failed: %d\n", res2);
							sockopen = 0;
							break;
						default:
							wiznet_debug_printf("send returned: %d\n", res2);
					}
			}
		}
		wiznet_debug_printf("irq_getsocket() = %d; recvsize = %d\n", wiznet_irq_getsocket(), wiznet_recvsize(sockfd));
		if (wiznet_irq_getsocket() == -EAGAIN && !wiznet_recvsize(sockfd)) {
			wiznet_debug_printf("LPM0;\n");
			LPM0;
			wiznet_debug_printf("wake;\n");
		}
	}

	return 0;
}

#pragma vector = PORT2_VECTOR
__interrupt void P2_IRQ (void) {
	if(P2IFG & W52_IRQ_PORTBIT){
		w5200_irq |= 1;
		__bic_SR_register_on_exit(LPM4_bits);    // Wake up
		P2IFG &= ~W52_IRQ_PORTBIT;   // Clear interrupt flag
	}
}
