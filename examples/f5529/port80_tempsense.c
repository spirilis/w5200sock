#include <msp430.h>
#include "clockinit.h"

#include "sprintf.h"
#include <stdint.h>
#include <string.h>
#include "w5200_config.h"
#include "w5200_buf.h"
#include "w5200_sock.h"
#include "w5200_debug.h"

/* TODO: Port the temperature sensor reading code to the F5xxx series! */

// Using our new ip2binary routines for this...
volatile int res1, res2;

int adc_temp_read();

int main() {
	int sockfd, tempF;
	uint8_t sockopen = 0;
	uint16_t i;
	uint8_t netbuf[32], tempFstr[8];

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

	sockfd = wiznet_socket(IPPROTO_TCP);
	if (sockfd < 0)
		LPM4;
	wiznet_debug_printf("wiznet_socket(): %d\n", sockfd);

	res1 = wiznet_bind(sockfd, 80);  // Bind port 80
	if (res1 < 0)
		LPM4;

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
				case -ENETDOWN:
					sockopen = 0;
					break;
				case -EAGAIN:
					break;
				default:
					for (i=0; i < res1-3; i++) {
						if (netbuf[i] == '\r' && netbuf[i+1] == '\n' && netbuf[i+2] == '\r' && netbuf[i+3] == '\n') {
							// Request submitted by client; server sends reply
							tempF = adc_temp_read();
							i = s_printf((char *)tempFstr, "%d F", tempF);

							strcpy((char *)netbuf, "HTTP/1.1 200 OK\r\n");
							wiznet_send(sockfd, netbuf, strlen((char *)netbuf), 0);
							strcpy((char *)netbuf, "Content-Type: text/plain\r\n");
							wiznet_send(sockfd, netbuf, strlen((char *)netbuf), 0);
							s_printf((char *)netbuf, "Content-Length: %d\r\n\r\n", i);
							wiznet_send(sockfd, netbuf, strlen((char *)netbuf), 0);
							if (wiznet_send(sockfd, tempFstr, strlen((char *)tempFstr), 1) < 0) {
								sockopen = 0;
							}
							wiznet_quickbind(sockfd);  // Close client connection & re-establish port 80 binding
						}
					}
			}
		}
		if (wiznet_irq_getsocket() == -EAGAIN && !wiznet_recvsize(sockfd)) {  // No IRQs pending
			wiznet_debug_printf("LPM0;\n");
			LPM0;
			wiznet_debug_printf("wake;\n");
		}
	}

	return 0;
}

#define TAG_ADC10_1_ 0x10DA /* ADC10_1 calibration tag */
#define CAL_ADC_25T85 (*(unsigned short *)(TAG_ADC10_1_+0x0010))
#define CAL_ADC_25T30 (*(unsigned short *)(TAG_ADC10_1_+0x000E))
#define CAL_ADC_25VREF_FACTOR (*(unsigned short *)(TAG_ADC10_1_+0x000C))
#define CAL_ADC_15T85 (*(unsigned short *)(TAG_ADC10_1_+0x000A))
#define CAL_ADC_15T30 (*(unsigned short *)(TAG_ADC10_1_+0x0008))
#define CAL_ADC_15VREF_FACTOR (*(unsigned short *)(TAG_ADC10_1_+0x0006))
#define CAL_ADC_OFFSET (*(short *)(TAG_ADC10_1_+0x0004))
#define CAL_ADC_GAIN_FACTOR (*(unsigned short *)(TAG_ADC10_1_+0x0002))


int adc_temp_read()
{
	long tempC, tempval;

	// Select temp sensor with ADC10CLK/4
	ADC10CTL1 = INCH_10 | ADC10DIV_3;
	ADC10CTL0 = SREF_1 | ADC10SHT_3 | REFON | ADC10ON;  // 1.5V Vref
	__delay_cycles(8000);

	ADC10CTL0 |= ENC | ADC10SC;  // Start conversion
	while (ADC10CTL0 & ADC10SC)
		;
	ADC10CTL0 &= ~ADC10IFG;

	/* oPossum code doctored up by spirilis */
	tempval = 3604480L / (CAL_ADC_15T85 - CAL_ADC_15T30);  // cc_scale
	tempC = tempval * ADC10MEM + (1998848L - (CAL_ADC_15T30 * tempval));
	tempC >>= 16;

	return (int) ( (tempC * 9) / 5 + 32 );
}

#pragma vector = PORT2_VECTOR
__interrupt void P2_IRQ (void) {
	if(P2IFG & W52_IRQ_PORTBIT){
		w5200_irq |= 1;
		__bic_SR_register_on_exit(LPM4_bits);    // Wake up
		P2IFG &= ~W52_IRQ_PORTBIT;   // Clear interrupt flag
	}
}
