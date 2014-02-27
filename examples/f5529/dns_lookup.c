#include <msp430.h>
#include "clockinit.h"

#include "dnslib.h"
#include <stdint.h>
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"
#include "w5200_debug.h"

// Using our new ip2binary routines for this...
//const uint16_t myip[] = { 0x0A68, 0x73B4 };  // 10.104.115.180
//const uint16_t myip[] = { 0xC0A8, 0x00E6 };  // 192.168.0.230
volatile int res1;

int main() {
	uint8_t netbuf[32];
	uint16_t myip[2], dns[2];

	WDTCTL = WDTPW | WDTHOLD;
        ucs_clockinit(16000000, 1, 0);
        __delay_cycles(160000);

	wiznet_debug_init();
	wiznet_debug_printf("W5200 DNS Lookup Init:\n");
	wiznet_init();

	while (wiznet_phystate() < 0)
		__delay_cycles(160000);

	wiznet_ip_bin_w_reg(W52_SUBNETMASK, w52_const_subnet_classC);
	wiznet_ip_str_w_reg(W52_SOURCEIP, "10.104.115.180");
	wiznet_ip_str_w_reg(W52_GATEWAY, "10.104.115.1");
	wiznet_ip2binary("10.104.8.201", dns);
	//wiznet_ip2binary("67.215.241.219", dns);
	dnslib_resolver = dns;

	wiznet_debug_printf("dnslib_gethostbyname(\"spirilis.net\"): ");
	res1 = dnslib_gethostbyname("spirilis.net", myip);
	if (res1 < 0) {
		switch (res1) {
			case -EFAULT:
				wiznet_debug_printf("EFAULT; dnslib_errno = %s\n", dnslib_strerror(dnslib_errno));
				break;
			case -ETIMEDOUT:
				wiznet_debug_printf("ETIMEDOUT; dnslib_errno = %s\n", dnslib_strerror(dnslib_errno));
				break;
			default:
				wiznet_debug_printf("ERROR returned: %d, dnslib_errno = %s\n", res1, dnslib_strerror(dnslib_errno));
		}
	} else {
		wiznet_binary2ip(myip, netbuf);
		wiznet_debug_printf("Success: %s\n", netbuf);
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
