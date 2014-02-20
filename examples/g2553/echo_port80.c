#include <msp430.h>

#include "uartcli.h"
#include "uartdbg.h"
#include <stdint.h>
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"

// Using our new ip2binary routines for this...
//const uint16_t myip[] = { 0x0A68, 0x73B4 };  // 10.104.115.180
//const uint16_t myip[] = { 0xC0A8, 0x00E6 };  // 192.168.0.230
volatile int res1, res2;
char uartbuf[8];

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
        DCOCTL = CALDCO_16MHZ;
        BCSCTL1 = CALBC1_16MHZ;
        BCSCTL2 = 0x00;
        BCSCTL3 = LFXT1S_2;
        __delay_cycles(160000);

	uartcli_begin(uartbuf, 8);
	wiznet_init();

	while (!wiznet_phystate())
		__delay_cycles(160000);

	wiznet_ip_str_w_reg(W52_SOURCEIP, "10.104.115.180");
	wiznet_ip_bin_w_reg(W52_SUBNETMASK, w52_const_subnet_classC);

	sockfd = wiznet_socket(IPPROTO_TCP);
	if (sockfd < 0)
		LPM4;
	uartcli_print_str("wiznet_socket(): "); uartcli_println_int(sockfd);

	res1 = wiznet_bind(sockfd, 80);  // Bind port 80
	if (res1 < 0)
		LPM4;

	RED_CLEAR;
	while(1) {
		if (!sockopen) {
			res1 = wiznet_accept(sockfd);
			uartcli_print_str("accept; "); uartcli_println_int(res1);
			if (!res1 || res1 == -EISCONN)
				sockopen = 1;
		} else {
			res1 = wiznet_recv(sockfd, netbuf, 32, 1);
			uartcli_print_str("RECV: "); uartcli_println_int(res1);
			//wiznet_debug_uart(sockfd);
			switch (res1) {
				case -ENOTCONN:
				case -ECONNABORTED:
				case -ENETDOWN:
					sockopen = 0;
					break;
				case -EAGAIN:
					break;
				default:
					uartcli_println_str("sending;");
					switch (res2 = wiznet_send(sockfd, netbuf, res1, 1)) {
						case -ENOTCONN:
						case -ECONNABORTED:
						case -ETIMEDOUT:
						case -ENETDOWN:
							uartcli_print_str("send failed: "); uartcli_println_int(res2);
							sockopen = 0;
							break;
						default:
							uartcli_print_str("send returned: "); uartcli_println_int(res2);
					}
			}
		}
		if (wiznet_irq_getsocket() == -EAGAIN && !wiznet_recvsize(sockfd)) {
			uartcli_println_str("LPM0;");
			LPM0;
			uartcli_println_str("wake;");
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
