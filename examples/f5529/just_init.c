#include <msp430.h>
#include "clockinit.h"

#include <stdint.h>
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"

int main() {
	volatile uint16_t i, j;

	WDTCTL = WDTPW | WDTHOLD;
        ucs_clockinit(16000000, 1, 0);
        __delay_cycles(160000);

	wiznet_init();

	for (i=0; i < 0x0036; i++) {
		j = wiznet_r_reg(i, 1);
		j++;
	}

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
