#include <msp430.h>

#include <stdint.h>
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"

const uint16_t myip[] = { 0x0A68, 0x73B4 };  // 10.104.115.180

int main() {
	volatile int sockfd;
	volatile int res1, res2;

	WDTCTL = WDTPW | WDTHOLD;
        DCOCTL = CALDCO_16MHZ;
        BCSCTL1 = CALBC1_16MHZ;
        BCSCTL2 = 0x00;
        BCSCTL3 = LFXT1S_2;
        __delay_cycles(160000);

	wiznet_init();

	while (wiznet_phystate() < 0)
		__delay_cycles(160000);

	wiznet_ip_bin_w_reg(W52_SOURCEIP, myip);
	wiznet_ip_bin_w_reg(W52_SUBNETMASK, w52_const_subnet_classC);

	sockfd = wiznet_socket(IPPROTO_TCP);
	if (sockfd < 0)
		LPM4;

	res1 = wiznet_bind(sockfd, 80);  // Bind port 80
	if (res1 < 0)
		LPM4;

	while ( (res2 = wiznet_accept(sockfd)) == -EAGAIN ) {
		__delay_cycles(160000);
	}

	// Freeze at this point, with the socket open.  Telnet to port 80 should at least connect.

	LPM4;
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
