#ifndef SPRINTF_H
#define SPRINTF_H

#include <stdint.h>

/* Printf() derived from oPossum's code - http://forum.43oh.com/topic/1289-tiny-printf-c-version/
 * Doctored up to make GCC happy.  Implemented as s_printf().
 */
uint16_t s_printf(char *instr, char *format, ...);

#endif
