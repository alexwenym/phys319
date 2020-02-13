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

#ifndef _MSP430_DISASM_H
#define _MSP430_DISASM_H

int get_cycle_count(unsigned short int opcode);
int msp430_disasm(unsigned char *memory, int address, char *instruction, int *cycles);

#endif

