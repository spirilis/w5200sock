/* Wiznet W5200 Ethernet Controller
 * Register & bit config file
 */

#ifndef W5200_REGS_H
#define W5200_REGS_H


/* SPI opcode */
#define W52_SPI_OPCODE_READ 0x0000
#define W52_SPI_OPCODE_WRITE 0x8000

/* Register file */
#define W52_MR 0x0000

#define W52_GATEWAY 0x0001
#define W52_GAR0 0x0001
#define W52_GAR1 0x0002
#define W52_GAR2 0x0003
#define W52_GAR3 0x0004

#define W52_SUBNETMASK 0x0005
#define W52_SUBR0 0x0005
#define W52_SUBR1 0x0006
#define W52_SUBR2 0x0007
#define W52_SUBR3 0x0008

#define W52_SOURCEMAC 0x0009
#define W52_SHAR0 0x0009
#define W52_SHAR1 0x000A
#define W52_SHAR2 0x000B
#define W52_SHAR3 0x000C
#define W52_SHAR4 0x000D
#define W52_SHAR5 0x000E

#define W52_SOURCEIP 0x000F
#define W52_SIPR0 0x000F
#define W52_SIPR1 0x0010
#define W52_SIPR2 0x0011
#define W52_SIPR3 0x0012

#define W52_IR 0x0015
#define W52_IMR 0x0016

#define W52_RTR0 0x0017
#define W52_RTR1 0x0018

#define W52_RCR 0x0019

#define W52_PPPOE_AUTHTYPE 0x001C
#define W52_PATR0 0x001C
#define W52_PATR1 0x001D
#define W52_PPPALGO 0x001E

#define W52_VERSIONR 0x001F

#define W52_PTIMER 0x0028
#define W52_PMAGIC 0x0029

#define W52_INTLEVEL0 0x0030
#define W52_INTLEVEL1 0x0031

#define W52_IR2 0x0034
#define W52_PHYSTATUS 0x0035
#define W52_IMR2 0x0036

/* Per-Socket register file; location is W52_SOCK_BASE + W52_SOCK_OFFSET*Sock# + register */
#define W52_SOCK_BASE 0x4000
#define W52_SOCK_OFFSET 0x0100
#define W52_SOCK_REG_RESOLVE(a, b) (W52_SOCK_BASE + W52_SOCK_OFFSET * a + b)

#define W52_SOCK_MR 0x0000
#define W52_SOCK_CR 0x0001
#define W52_SOCK_IR 0x0002
#define W52_SOCK_SR 0x0003

#define W52_SOCK_SRCPORT 0x0004
#define W52_SOCK_PORT0 0x0004
#define W52_SOCK_PORT1 0x0005

#define W52_SOCK_DESTMAC 0x0006
#define W52_SOCK_DHAR0 0x0006
#define W52_SOCK_DHAR1 0x0007
#define W52_SOCK_DHAR2 0x0008
#define W52_SOCK_DHAR3 0x0009
#define W52_SOCK_DHAR4 0x000A
#define W52_SOCK_DHAR5 0x000B

#define W52_SOCK_DESTIP 0x000C
#define W52_SOCK_DIPR0 0x000C
#define W52_SOCK_DIPR1 0x000D
#define W52_SOCK_DIPR2 0x000E
#define W52_SOCK_DIPR3 0x000F

#define W52_SOCK_DESTPORT 0x0010
#define W52_SOCK_DPORT0 0x0010
#define W52_SOCK_DPORT1 0x0011

#define W52_SOCK_MSS 0x0012
#define W52_SOCK_MSSR0 0x0012
#define W52_SOCK_MSSR1 0x0013

#define W52_SOCK_PROTO 0x0014
#define W52_SOCK_TOS 0x0015
#define W52_SOCK_TTL 0x0016

#define W52_SOCK_RXMEM_SIZE 0x001E
#define W52_SOCK_TXMEM_SIZE 0x001F

#define W52_SOCK_TXFREE_SIZE 0x0020
#define W52_SOCK_TX_FSR0 0x0020
#define W52_SOCK_TX_FSR1 0x0021

#define W52_SOCK_TX_READPTR 0x0022
#define W52_SOCK_TX_RD0 0x0022
#define W52_SOCK_TX_RD1 0x0023

#define W52_SOCK_TX_WRITEPTR 0x0024
#define W52_SOCK_TX_WR0 0x0024
#define W52_SOCK_TX_WR1 0x0025

#define W52_SOCK_RX_RECVSIZE 0x0026
#define W52_SOCK_RX_RSR0 0x0026
#define W52_SOCK_RX_RSR1 0x0027

#define W52_SOCK_RX_READPTR 0x0028
#define W52_SOCK_RX_RD0 0x0028
#define W52_SOCK_RX_RD1 0x0029

#define W52_SOCK_RX_WRITEPTR 0x002A
#define W52_SOCK_RX_WR0 0x002A
#define W52_SOCK_RX_WR1 0x002B

#define W52_SOCK_IMR 0x002C

#define W52_SOCK_FRAGOFFSET 0x002D
#define W52_SOCK_FRAG0 0x002D
#define W52_SOCK_FRAG1 0x002E

/* TX & RX memory buffers */
#define W52_TXMEM_BASE 0x8000
#define W52_RXMEM_BASE 0xC000

/* Register bits */
#define W52_MR_RST 0x80
#define W52_MR_WOL 0x20
#define W52_MR_PB 0x10
#define W52_MR_PPPOE 0x08

#define W52_IR_CONFLICT 0x80
#define W52_IR_PPPOE 0x20

#define W52_PPPOE_AUTHTYPE_PAP 0xC023
#define W52_PPPOE_AUTHTYPE_CHAP 0xC223

#define W52_PHYSTATUS_LINK 0x20
#define W52_PHYSTATUS_POWERSAVE 0x10
#define W52_PHYSTATUS_POWERDOWN 0x08

#define W52_IMR2_IPCONFLICT 0x80
#define W52_IMR2_PPPOECLOSE 0x20

#define W52_SOCK_MR_MULTI 0x80
#define W52_SOCK_MR_MF 0x40
#define W52_SOCK_MR_ND 0x20
#define W52_SOCK_MR_MC 0x20

#define W52_SOCK_MR_PROTO_CLOSED 0x0
#define W52_SOCK_MR_PROTO_TCP 0x1
#define W52_SOCK_MR_PROTO_UDP 0x2
#define W52_SOCK_MR_PROTO_IPRAW 0x3
#define W52_SOCK_MR_PROTO_MACRAW 0x4
#define W52_SOCK_MR_PROTO_PPPOE 0x5

#define IPPROTO_TCP W52_SOCK_MR_PROTO_TCP
#define IPPROTO_UDP W52_SOCK_MR_PROTO_UDP
#define IPPROTO_IP W52_SOCK_MR_PROTO_IPRAW
#define IPPROTO_NONE W52_SOCK_MR_PROTO_MACRAW
#define IPPROTO_PPPOE W52_SOCK_MR_PROTO_PPPOE

#define W52_SOCK_CMD_OPEN 0x01
#define W52_SOCK_CMD_LISTEN 0x02
#define W52_SOCK_CMD_CONNECT 0x04
#define W52_SOCK_CMD_DISCON 0x08
#define W52_SOCK_CMD_CLOSE 0x10
#define W52_SOCK_CMD_SEND 0x20
#define W52_SOCK_CMD_SEND_MAC 0x21
#define W52_SOCK_CMD_SEND_KEEP 0x22
#define W52_SOCK_CMD_RECV 0x40

#define W52_SOCK_CMD_PCON 0x23
#define W52_SOCK_CMD_PDISCON 0x24
#define W52_SOCK_CMD_PCR 0x25
#define W52_SOCK_CMD_PCN 0x26
#define W52_SOCK_CMD_PCJ 0x27

#define W52_SOCK_IR_PRECV 0x80
#define W52_SOCK_IR_PFAIL 0x40
#define W52_SOCK_IR_PNEXT 0x20
#define W52_SOCK_IR_SEND_OK 0x10
#define W52_SOCK_IR_TIMEOUT 0x08
#define W52_SOCK_IR_RECV 0x04
#define W52_SOCK_IR_DISCON 0x02
#define W52_SOCK_IR_CON 0x01

#define W52_SOCK_SR_SOCK_CLOSED 0x00
#define W52_SOCK_SR_SOCK_INIT 0x13
#define W52_SOCK_SR_SOCK_LISTEN 0x14
#define W52_SOCK_SR_SOCK_ESTABLISHED 0x17
#define W52_SOCK_SR_SOCK_CLOSE_WAIT 0x1C
#define W52_SOCK_SR_SOCK_UDP 0x22
#define W52_SOCK_SR_SOCK_IPRAW 0x32
#define W52_SOCK_SR_SOCK_MACRAW 0x42
#define W52_SOCK_SR_SOCK_PPPOE 0x5F
#define W52_SOCK_SR_SOCK_SYNSENT 0x15
#define W52_SOCK_SR_SOCK_SYNRECV 0x16
#define W52_SOCK_SR_SOCK_FIN_WAIT 0x18
#define W52_SOCK_SR_SOCK_CLOSING 0x1A
#define W52_SOCK_SR_SOCK_TIME_WAIT 0x1B
#define W52_SOCK_SR_SOCK_LAST_ACK 0x1D
#define W52_SOCK_SR_SOCK_ARP 0x01

/* Convenient lookup table with descriptive strings for the states
 * (mostly adapted from Linux kernel's include/net/tcp_states.h)
 * Actual definitions inside w5200.c
 */
#define W52_TCP_STATE_COUNT 10

extern const char *w52_tcp_state[];
extern const uint8_t w52_tcp_state_idx[];

#define W52_SOCK_IMR_PRECV 0x80
#define W52_SOCK_IMR_PFAIL 0x40
#define W52_SOCK_IMR_PNEXT 0x20
#define W52_SOCK_IMR_SEND_OK 0x10
#define W52_SOCK_IMR_TIMEOUT 0x08
#define W52_SOCK_IMR_RECV 0x04
#define W52_SOCK_IMR_DISCON 0x02
#define W52_SOCK_IMR_CON 0x01


#endif
