#include <msp430.h>
#include "clockinit.h"

#include <stdint.h>
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"

const uint16_t myip[] = { 0x0A68, 0x73B4 };  // 10.104.115.180

int main() {
	volatile int sockfd80, sockfd443;
	volatile int res1, res2, res3;

	WDTCTL = WDTPW | WDTHOLD;
	ucs_clockinit(16000000, 1, 0);
	__delay_cycles(160000);

	wiznet_init();

	while (wiznet_phystate() < 0)
		__delay_cycles(160000);

	wiznet_ip_bin_w_reg(W52_SOURCEIP, myip);
	wiznet_ip_bin_w_reg(W52_SUBNETMASK, w52_const_subnet_classC);

	sockfd80 = wiznet_socket(IPPROTO_TCP);
	if (sockfd80 < 0)
		LPM4;

	res1 = wiznet_bind(sockfd80, 80);  // Bind port 80
	if (res1 < 0)
		LPM4;

	sockfd443 = wiznet_socket(IPPROTO_TCP);
	if (sockfd443 < 0)
		LPM4;

	res1 = wiznet_bind(sockfd443, 443);  // Bind port 443
	if (res1 < 0)
		LPM4;

	res2 = res3 = -EAGAIN;
	while(1) {
		if (res2 == -EAGAIN)
			res2 = wiznet_accept(sockfd80);
		if (res3 == -EAGAIN)
			res3 = wiznet_accept(sockfd443);
		__delay_cycles(160000);
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
