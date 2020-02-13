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

#ifndef _MSP430_SIMULATE_H
#define _MSP430_SIMULATE_H

#include <unistd.h>

struct _msp_sim
{
  unsigned char *ram;
  unsigned short int reg[16];
  int cycle_count;
  int ret_count;
  useconds_t usec;
  int step_mode;
  int break_point;
  int show;
};

struct _msp_sim *msp430_simulate_init();
void msp430_simulate_push(struct _msp_sim *msp_sim, unsigned int value);
void msp430_simulate_set(struct _msp_sim *msp_sim, int reg, unsigned int value);
void msp430_simulate_reset(struct _msp_sim *msp_sim, unsigned char *memory);
void msp430_simulate_free(struct _msp_sim *msp_sim);
void msp430_simulate_dump_registers(struct _msp_sim *msp_sim);
int msp430_simulate_run(struct _msp_sim *msp_sim, int max_cycles, int step);

#endif

