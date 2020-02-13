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

#ifndef _MSP430_ASM_H
#define _MSP430_ASM_H

#include "assembler.h"

int parse_instruction(struct _asm_context *asm_context, char *instr);

#endif

