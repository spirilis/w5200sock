/* w5200_debug.c
 * WizNet W5200 Ethernet Controller Driver for MSP430
 * Debug printf routine for WizNet Library
 *
 *
 * Parts derived from Kevin Timmerman's tiny printf from 43oh.com
 * Link: http://forum.43oh.com/topic/1289-tiny-printf-c-version/
 *
 * Copyright (c) 2014, Eric Brundick <spirilis@linux.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <msp430.h>
#include <stdint.h>
#include <stdarg.h>
#include "w5200_debug.h"
#include "w5200_buf.h"
#include "w5200_sock.h"

/* User-provided putc, puts functions go here */
void wiznet_debug_puts(const char *str)
{
	unsigned int c;

	while ( (c = (unsigned int)(*str++)) != '\0' )
		wiznet_debug_putc(c);
}

void wiznet_debug_putc(unsigned int c)
{
	UCA1TXBUF = (uint8_t)c;
	while (!(UCA1IFG & UCTXIFG))
		;
}

void wiznet_debug_init()
{
	// F5529LP USCI_A1 Backchannel UART init
	UCA1IFG = 0;
	UCA1CTL0 = 0x00;
	UCA1CTL1 = UCSSEL_2 | UCSWRST;

	// 115200 @ 16MHz UCOS16=1
	UCA1BR0 = 8;
	UCA1BR1 = 0;
	UCA1MCTL = UCBRS_0 | UCBRF_11 | UCOS16;

	// Port sel
	P4SEL |= BIT4|BIT5;  // USCI_A1

	// Ready
	UCA1CTL1 &= ~UCSWRST;
}



void wiznet_debug_putc(unsigned int);
void wiznet_debug_puts(const char *);

static const unsigned long dv[] = {
//  4294967296      // 32 bit unsigned max
    1000000000,     // +0
     100000000,     // +1
      10000000,     // +2
       1000000,     // +3
        100000,     // +4
//       65535      // 16 bit unsigned max     
         10000,     // +5
          1000,     // +6
           100,     // +7
            10,     // +8
             1,     // +9
};

static void xtoa(unsigned long x, const unsigned long *dp)
{
    char c;
    unsigned long d;
    if(x) {
        while(x < *dp) ++dp;
        do {
            d = *dp++;
            c = '0';
            while(x >= d) ++c, x -= d;
            wiznet_debug_putc(c);
        } while(!(d & 1));
    } else
        wiznet_debug_putc('0');
}

static void puth(unsigned int n)
{
    static const char hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
    wiznet_debug_putc(hex[n & 15]);
}
 
void wiznet_debug_printf(char *format, ...)
{
    char c;
    int i;
    long n;
    
    va_list a;
    va_start(a, format);
    while( (c = *format++) ) {
        if(c == '%') {
            switch(c = *format++) {
                case 's':                       // String
                    wiznet_debug_puts(va_arg(a, char*));
                    break;
                case 'c':                       // Char
                    wiznet_debug_putc(va_arg(a, int));   // Char gets promoted to Int in args, so it's an int we're looking for (GCC warning)
                    break;
                case 'i':                       // 16 bit Integer
                case 'd':                       // 16 bit Integer
                case 'u':                       // 16 bit Unsigned
                    i = va_arg(a, int);
                    if( (c == 'i' || c == 'd') && i < 0 ) i = -i, wiznet_debug_putc('-');
                    xtoa((unsigned)i, dv + 5);
                    break;
                case 'l':                       // 32 bit Long
                case 'n':                       // 32 bit uNsigned loNg
                    n = va_arg(a, long);
                    if(c == 'l' &&  n < 0) n = -n, wiznet_debug_putc('-');
                    xtoa((unsigned long)n, dv);
                    break;
                case 'x':                       // 16 bit heXadecimal
                    i = va_arg(a, int);
                    puth(i >> 12);
                    puth(i >> 8);
                    puth(i >> 4);
                    puth(i);
                    break;
                case 0: return;
                default: goto bad_fmt;
            }
        } else
bad_fmt:    if (c != '\n') { wiznet_debug_putc(c); } else { wiznet_debug_putc('\r'); wiznet_debug_putc('\n'); }  // UART CR-LF support
    }
    va_end(a);
}

/* Nitty-gritty WizNet transceiver debug stuff */
void wiznet_debug_dumpregs_main()
{
	uint8_t buf[0x38];

	wiznet_r_buf(0x0000, 0x0037, buf);  // Read entire main register section

	// Print contents.
	wiznet_debug_printf("Main registers:\n");
	wiznet_debug_printf("Mode: %x\n", buf[0]);
	wiznet_debug_printf("Gateway: %d.%d.%d.%d\n", buf[1], buf[2], buf[3], buf[4]);
	wiznet_debug_printf("Subnet Mask: %d.%d.%d.%d\n", buf[5], buf[6], buf[7], buf[8]);
	wiznet_debug_printf("MAC address: %x%x%x\n", wiznet_ntohs(buf+0x09), wiznet_ntohs(buf+0x0B), wiznet_ntohs(buf+0x0D));
	wiznet_debug_printf("IP address: %d.%d.%d.%d\n", buf[0x0F], buf[0x10], buf[0x11], buf[0x12]);
	wiznet_debug_printf("IR: %x\n", buf[0x15]);
	wiznet_debug_printf("IMR: %x\n", buf[0x16]);
	wiznet_debug_printf("Retry time: %d\n", wiznet_ntohs(buf+0x17));
	wiznet_debug_printf("Retry count: %d\n", buf[0x19]);
	wiznet_debug_printf("Chip version: %d\n", buf[0x1F]);
	wiznet_debug_printf("Interrupt Low Level Timer: %x\n", wiznet_ntohs(buf+0x30));
	wiznet_debug_printf("IR2: %x\n", buf[0x34]);
	wiznet_debug_printf("PHYSTATUS: %x\n", buf[0x35]);
	wiznet_debug_printf("IMR2: %x\n", buf[0x36]);
}

void wiznet_debug_dumpregs_sock(int sockfd)
{
	uint8_t buf[0x30];

	wiznet_r_buf(W52_SOCK_REG_RESOLVE(sockfd, 0x0000), 0x2F, buf);

	// Print contents
	wiznet_debug_printf("Socket %d registers:\n", sockfd);
	wiznet_debug_printf("S%d_MR: %x\n", sockfd, buf[0]);
	wiznet_debug_printf("S%d_CR: %x\n", sockfd, buf[1]);
	wiznet_debug_printf("S%d_IR: %x\n", sockfd, buf[2]);
	wiznet_debug_printf("S%d_SR: %x\n", sockfd, buf[3]);
	wiznet_debug_printf("S%d_SRCPORT: %d\n", sockfd, wiznet_ntohs(buf+0x04));
	wiznet_debug_printf("S%d_DESTMAC: %x%x%x\n", sockfd, wiznet_ntohs(buf+0x06), wiznet_ntohs(buf+0x08), wiznet_ntohs(buf+0x0A));
	wiznet_debug_printf("S%d_DESTIP: %d.%d.%d.%d\n", sockfd, buf[0x0C], buf[0x0D], buf[0x0E], buf[0x0F]);
	wiznet_debug_printf("S%d_DESTPORT: %d\n", sockfd, wiznet_ntohs(buf+0x10));
	wiznet_debug_printf("S%d_MSS: %d\n", sockfd, wiznet_ntohs(buf+0x12));
	wiznet_debug_printf("S%d_PROTO: %x\n", sockfd, buf[0x14]);
	wiznet_debug_printf("S%d_IP_TOS: %d\n", sockfd, buf[0x15]);
	wiznet_debug_printf("S%d_IP_TTL: %d\n", sockfd, buf[0x16]);
	wiznet_debug_printf("S%d_RXMEM_SIZE: %d\n", sockfd, buf[0x1E]);
	wiznet_debug_printf("S%d_TXMEM_SIZE: %d\n", sockfd, buf[0x1F]);
	wiznet_debug_printf("S%d_TX_FSR: %d\n", sockfd, wiznet_ntohs(buf+0x20));
	wiznet_debug_printf("S%d_TX_RD: %d\n", sockfd, wiznet_ntohs(buf+0x22));
	wiznet_debug_printf("S%d_TX_WR: %d\n", sockfd, wiznet_ntohs(buf+0x24));
	wiznet_debug_printf("S%d_RX_RSR %d\n", sockfd, wiznet_ntohs(buf+0x26));
	wiznet_debug_printf("S%d_RX_RD %d\n", sockfd, wiznet_ntohs(buf+0x28));
	wiznet_debug_printf("S%d_RX_WR %d\n", sockfd, wiznet_ntohs(buf+0x2A));
	wiznet_debug_printf("S%d_IMR: %x\n", sockfd, buf[0x2C]);
	wiznet_debug_printf("S%d_FRAG_OFFSET: %d\n", sockfd, wiznet_ntohs(buf+0x2D));
}
