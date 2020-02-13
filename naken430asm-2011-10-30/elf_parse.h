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

#ifndef _ELF_PARSE_H
#define _ELF_PARSE_H

int elf_parse(char *filename, unsigned char *memory, int memory_len, int *mem_start, int *mem_end, unsigned char *dirty);

#endif

