/**
 *  Naken430asm MSP430 assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2011 by Michael Kohn
 *
 */

#ifndef _HEX_PARSE_H
#define _HEX_PARSE_H

int hex_parse(char *filename, unsigned char *memory, int memory_len, int *mem_start, int *mem_end, unsigned char *dirty);

#endif

