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

#ifndef _JTAG_H
#define _JTAG_H

#include "naken430util.h"

int open_jtag(struct _util_context *util_context, char *device);
void close_jtag(struct _util_context *util_context);
int read_memory(struct _util_context *util_context, int address, int len);

#endif

