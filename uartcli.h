#ifndef _UARTCLI_H
#define _UARTCLI_H

// Needed for "NULL"
#include <sys/cdefs.h>


/* USCI_A 9600bps (4MHz SMCLK) UART code for sending & receiving textual data */


/* User-configurable parameters */
// Line/record-delimiter; define with single-quotes, not double-quotes.
// Note that linefeeds ('\n') are automatically dropped/ignored and won't be present in the final buffer.
#define UARTCLI_NEWLINE_DELIM '\r'

// Newline characters to use when sending serial output; define with double-quotes, it's expected to be a char array (string)
#define UARTCLI_NEWLINE_OUTPUT "\r\n"

// Whitespace delimiter for parsing commands vs. arguments; define with single-quotes, not double-quotes.
#define UARTCLI_TOKEN_DELIM ' '


/* Functions */
// Initialize/disable USCI_A
void uartcli_begin(char *, int);   // Specify a char buffer to hold incoming commands, and the maximum size of said buffer.
void uartcli_end();

// Print different types of datatypes
void uartcli_print_str(const void *);
void uartcli_println_str(const void *);
void uartcli_print_int(int);
void uartcli_println_int(int);
void uartcli_print_uint(unsigned int);
void uartcli_println_uint(unsigned int);
void uartcli_printhex_byte(unsigned char);
void uartcli_printhex_word(int);

// Accept incoming data
extern volatile char uartcli_task;      // Status variable
#define UARTCLI_TASK_AVAILABLE		0x01
#define UARTCLI_TASK_RECEIVING		0x02
#define UARTCLI_TASK_TX			0x04

inline char uartcli_available();   // Check if an incoming message is available
inline void uartcli_clear();       // Notify library last incoming message processed; quit dropping new commands
void uartcli_setbuf(char *, int);  // Set incoming data buffer and maximum size.  Run within uartcli_begin()

// Parse/tokenize data into command + arguments
void uartcli_token_begin();        /* Run this right before tokenizing a newly-received command; it sets some
				    * internal pointers.
				    */
int uartcli_token_cmd(const char**);           /* Parse command string and compare to a list of strings; return
					        * index of found command or -1 if not found.  Intended to make
						* it easy to branch code based on command strings using switch().
						*/
char* uartcli_token_cmdstr(char*, int);  /* Copy command string into char buf, at most (int) buflen bytes. */

char* uartcli_token_arg(unsigned char, char*, int);  /* Find argument by index (starting at 1) and copy into
						      * supplied char buf, at most (int) buflen bytes.  Returns
						      * NULL if argument not found.
						      */

#endif
