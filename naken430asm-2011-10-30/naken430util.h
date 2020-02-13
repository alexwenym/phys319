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

#ifndef NAKEN_430_UTIL_H
#define NAKEN_430_UTIL_H

#include "msp430_simulate.h"

struct _util_context
{
  unsigned char bin[65536];
  unsigned char dirty[65536];
  int debug_line[65536];
  int memory_size;
  struct _msp_sim *msp_sim;
  long *debug_line_offset;
  FILE *src_fp;
  int start;
  int end;
  int fd;
};

#endif

