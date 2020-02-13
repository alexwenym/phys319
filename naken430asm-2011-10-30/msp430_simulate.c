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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "msp430_disasm.h"
#include "msp430_simulate.h"

/*

        8   7     6     5       4   3 2 1 0
Status: V SCG1 SCG0 OSCOFF CPUOFF GIE N Z C

*/

#define SHOW_STACK sp, msp_sim->ram[sp+1], msp_sim->ram[sp]

#define GET_V() ((msp_sim->reg[2]>>8)&1)
#define GET_SCG1() ((msp_sim->reg[2]>>7)&1)
#define GET_SCG0() ((msp_sim->reg[2]>>6)&1)
#define GET_OSCOFF() ((msp_sim->reg[2]>>5)&1)
#define GET_CPUOFF() ((msp_sim->reg[2]>>4)&1)
#define GET_GIE() ((msp_sim->reg[2]>>3)&1)
#define GET_N() ((msp_sim->reg[2]>>2)&1)
#define GET_Z() ((msp_sim->reg[2]>>1)&1)
#define GET_C() ((msp_sim->reg[2])&1)

#define SET_V() msp_sim->reg[2]|=256;
#define SET_SCG1() msp_sim->reg[2]|=128;
#define SET_SCG0() msp_sim->reg[2]|=64;
#define SET_OSCOFF() msp_sim->reg[2]|=32;
#define SET_CPUOFF() msp_sim->reg[2]|=16;
#define SET_GIE() msp_sim->reg[2]|=8;
#define SET_N() msp_sim->reg[2]|=4;
#define SET_Z() msp_sim->reg[2]|=2;
#define SET_C() msp_sim->reg[2]|=1;

#define CLEAR_V() msp_sim->reg[2]&=(0xffff^256);
#define CLEAR_SCG1() msp_sim->reg[2]&=(0xffff^128);
#define CLEAR_SCG0() msp_sim->reg[2]&=(0xffff^64);
#define CLEAR_OSCOFF() msp_sim->reg[2]&=(0xffff^32);
#define CLEAR_CPUOFF() msp_sim->reg[2]&=(0xffff^16);
#define CLEAR_GIE() msp_sim->reg[2]&=(0xffff^8);
#define CLEAR_N() msp_sim->reg[2]&=(0xffff^4);
#define CLEAR_Z() msp_sim->reg[2]&=(0xffff^2);
#define CLEAR_C() msp_sim->reg[2]&=(0xffff^1);

#define AFFECTS_NZ(a) \
  if (bw==0) \
  { \
    if (a&0x8000) { SET_N(); } else { CLEAR_N(); } \
  } \
    else \
  { \
    if (a&0x80) { SET_N(); } else { CLEAR_N(); } \
  } \
  if (a==0) { SET_Z(); } else { CLEAR_Z(); }

#define CHECK_CARRY(a) \
  if (bw==0) \
  { \
    if ((a&0xffff0000)==0) { CLEAR_C(); } else { SET_C(); } \
  } \
    else \
  { \
    if ((a&0xffffff00)==0) { CLEAR_C(); } else { SET_C(); } \
  }

#define CHECK_OVERFLOW() \
  if (bw==0) \
  { \
    if (((((unsigned short int)dst)&0x8000)==(((unsigned short int)src)&0x8000)) && (((((unsigned short int)result)&0x8000))!=(((unsigned short int)dst)&0x8000))) { SET_V(); } else { CLEAR_V(); } \
  } \
    else \
  { \
    if (((((unsigned char)dst)&0x80)==(((unsigned char)src)&0x80)) && (((((unsigned char)result)&0x80))!=(((unsigned char)dst)&0x80))) { SET_V(); } else { CLEAR_V(); } \
  }

#define CHECK_OVERFLOW_WITH_C() \
  if (bw==0) \
  { \
    if ((((int)dst+(int)src+GET_C())&0xffff0000)!=0) { SET_V(); } else { CLEAR_V(); } \
  } \
    else \
  { \
    if ((((int)dst+(int)src+GET_C())&0xffffff00)!=0) { SET_V(); } else { CLEAR_V(); } \
  }

#define CHECK_Z() \
  if (bw==0) \
  { \
    if (result&0x8000) { SET_N(); } else { CLEAR_N(); } \
  } \
    else \
  { \
    if (result&0x80) { SET_N(); } else { CLEAR_N(); } \
  }

static int stop_running=0;

void handle_signal(int sig)
{
  stop_running=1;
  signal(SIGINT, SIG_DFL);
}

static void sp_inc(int *sp)
{
  (*sp)+=2;
  if (*sp>0xffff) *sp=0;
}

static unsigned short int get_data(struct _msp_sim *msp_sim, int reg, int As, int bw)
{
//printf("reg=%d As=%d bw=%d\n", reg, As, bw);
  if (reg==3) // CG
  {
    if (As==0)
    { return 0; }
      else
    if (As==1)
    { return 1; }
      else
    if (As==2)
    { return 2; }
      else
    if (As==3)
    {
      if (bw==0)
      { return 0xffff; }
        else
      { return 0xff; }
    }
  }

  if (As==0) // Rn
  {
    if (bw==0)
    { return msp_sim->reg[reg]; }
      else
    { return msp_sim->reg[reg]&0xff; }
  }

  if (reg==2)
  {
    if (As==1) // &LABEL
    {
      //unsigned short int a=msp_sim->ram[msp_sim->reg[0]]|(msp_sim->ram[msp_sim->reg[0]+1]<<8);
      //unsigned short int a=msp_sim->reg[0];
      int PC=msp_sim->reg[0];
      unsigned short int a=msp_sim->ram[PC]|(msp_sim->ram[PC+1]<<8);
//printf("LABEL a=%d\n", a);
      msp_sim->reg[0]+=2;
      if (bw==0)
      { return msp_sim->ram[a]|(msp_sim->ram[a+1]<<8); }
        else
      { return msp_sim->ram[a]; }
    }
      else
    if (As==2)
    { return 4; }
      else
    if (As==3)
    { return 8; }
  }

  if (reg==0) // PC
  {
    if (As==3) // #immediate
    {
      unsigned short int a=msp_sim->ram[msp_sim->reg[0]]|(msp_sim->ram[msp_sim->reg[0]+1]<<8);
      msp_sim->reg[0]+=2;
      if (bw==0)
      { return a; }
        else
      { return a&0xff; }
    }
  }

  if (As==1) // x(Rn)
  {
    unsigned short int a=msp_sim->ram[msp_sim->reg[0]]|(msp_sim->ram[msp_sim->reg[0]+1]<<8);
    unsigned short int index=msp_sim->reg[reg]+a;
    msp_sim->reg[0]+=2;
    if (bw==0)
    { return msp_sim->ram[index]|(msp_sim->ram[index+1]<<8); }
      else
    { return msp_sim->ram[index]; }
  }
    else
  if (As==2) // @Rn
  {
    if (bw==0)
    { return msp_sim->ram[msp_sim->reg[reg]]|(msp_sim->ram[msp_sim->reg[reg]+1]<<8); }
      else
    { return msp_sim->ram[msp_sim->reg[reg]]; }
  }
    else
  if (As==3) // @Rn+
  {
    unsigned short index=msp_sim->reg[reg];
    if (bw==0)
    {
      msp_sim->reg[reg]+=2;
      return msp_sim->ram[index]|(msp_sim->ram[index+1]<<8);
    }
      else
    {
      msp_sim->reg[reg]+=1;
      return msp_sim->ram[index];
    }
  }

  printf("Error: Unrecognized addressing mode\n");

  return 0;
}

static int put_data(struct _msp_sim *msp_sim, int PC, int reg, int Ad, int bw, unsigned int data)
{

/*
  if (reg==3) // CG (should be ignored)
  {
    printf("Warning: Attempt to write to CG register near PC=0x%04x\n",PC);
    return 0;
  }
*/

  if (Ad==0) // Rn
  {
    if (bw==0)
    { msp_sim->reg[reg]=data; }
      else
    {
      //msp_sim->reg[reg]&=0xff00;
      //msp_sim->reg[reg]|=data&0xff;
      msp_sim->reg[reg]=data&0xff;
    }
    return 0;
  }

  if (reg==2)
  {
    if (Ad==1) // &LABEL
    {
      unsigned short int a=msp_sim->ram[PC]|(msp_sim->ram[PC+1]<<8);

      if (bw==0)
      {
        msp_sim->ram[a]=data&0xff;
        msp_sim->ram[a+1]=data>>8;
      }
        else
      { msp_sim->ram[a]=data&0xff; }

      return 0;
    }
  }
  if (reg==0) // PC
  {
    if (Ad==1) // LABEL
    {
      unsigned short int a=msp_sim->ram[PC]|(msp_sim->ram[PC+1]<<8);

      if (bw==0)
      {
        msp_sim->ram[PC+a]=data&0xff;
        msp_sim->ram[PC+a+1]=data>>8;
      }
        else
      {
        msp_sim->ram[PC+a]=data&0xff;
      }

      return 0;
    }
  }

  if (Ad==1) // x(Rn)
  {
    unsigned short int a=msp_sim->ram[PC]|(msp_sim->ram[PC+1]<<8);

    if (bw==0)
    {
      msp_sim->ram[msp_sim->reg[reg]+a]=data&0xff;
      msp_sim->ram[msp_sim->reg[reg]+a+1]=data>>8;
    }
      else
    {
      msp_sim->ram[msp_sim->reg[reg]+a]=data&0xff;
    }

    return 0;
  }

  printf("Error: Unrecognized addressing mode for destination\n");

  return -1;
}

static int one_operand_exe(struct _msp_sim *msp_sim, unsigned short int opcode)
{
int o;
int reg;
int As,bw;
int count=1;
unsigned int result;
int src;
int pc;

  o=(opcode&0x0380)>>7;

  if (o==7) { return 1; }
  if (o==6) { return count; }

  As=(opcode&0x0030)>>4;
  bw=(opcode&0x0040)>>6;
  reg=opcode&0x000f;

  switch(o)
  {
    case 0:  // RRC
      pc=msp_sim->reg[0];
      src=get_data(msp_sim, reg, As, bw);
      int c=GET_C();
      if ((src&1)==1) { SET_C(); } else { CLEAR_C(); }
      if (bw==0)
      { result=(c<<15)|(((unsigned short int)src)>>1); }
        else
      { result=(c<<7)|(((unsigned char)src)>>1); }
      put_data(msp_sim, pc, reg, As, bw, result);
      AFFECTS_NZ(result);
      CLEAR_V();
      break;
    case 1:  // SWPB (no bw)
      pc=msp_sim->reg[0];
      src=get_data(msp_sim, reg, As, bw);
      result=((src&0xff00)>>8)|((src&0xff)<<8);
      put_data(msp_sim, pc, reg, As, bw, result);
      break;
    case 2:  // RRA
      pc=msp_sim->reg[0];
      src=get_data(msp_sim, reg, As, bw);
      if ((src&1)==1) { SET_C(); } else { CLEAR_C(); }
      if (bw==0)
      { result=((short int)src)>>1; }
        else
      { result=((char)src)>>1; }
      put_data(msp_sim, pc, reg, As, bw, result);
      AFFECTS_NZ(result);
      CLEAR_V();
      break;
    case 3:  // SXT (no bw)
      pc=msp_sim->reg[0];
      src=get_data(msp_sim, reg, As, bw);
      result=(short int)((char)((unsigned char)src));
      put_data(msp_sim, pc, reg, As, bw, result);
      AFFECTS_NZ(result);
      CHECK_CARRY(result);
      CLEAR_V();
      break;
    case 4:  // PUSH
      msp_sim->reg[1]-=2;
      src=get_data(msp_sim, reg, As, bw);
      msp_sim->ram[msp_sim->reg[1]]=src&0xff;
      msp_sim->ram[msp_sim->reg[1]+1]=src>>8;
      break;
    case 5:  // CALL (no bw)
      src=get_data(msp_sim, reg, As, bw);
      msp_sim->reg[1]-=2;
      msp_sim->ram[msp_sim->reg[1]]=msp_sim->reg[0]&0xff;
      msp_sim->ram[msp_sim->reg[1]+1]=msp_sim->reg[0]>>8;
      msp_sim->reg[0]=src;
      break;
    case 6:  // RETI
      break;
    default: 
      return -1;
  }

  return 0;
}

static int relative_jump_exe(struct _msp_sim *msp_sim, unsigned short int opcode)
{
//unsigned short int *reg=msp_sim->reg;
int o;

  o=(opcode&0x1c00)>>10;

  int offset=opcode&0x03ff;
  if ((offset&0x0200)!=0)
  {
    offset=-((offset^0x03ff)+1);
  }

  offset*=2;

  switch(o)
  {
    case 0:  // JNE/JNZ  Z==0
      if (GET_Z()==0) msp_sim->reg[0]+=offset;
      break;
    case 1:  // JEQ/JZ   Z==1 
      if (GET_Z()==1) msp_sim->reg[0]+=offset;
      break;
    case 2:  // JNC/JLO  C==0
      if (GET_C()==0) msp_sim->reg[0]+=offset;
      break;
    case 3:  // JC/JHS   C==1
      if (GET_C()==1) msp_sim->reg[0]+=offset;
      break;
    case 4:  // JN       N==1
      if (GET_N()==1) msp_sim->reg[0]+=offset;
      break;
    case 5:  // JGE      (N^V)==0
      if ((GET_N()^GET_V())==0) msp_sim->reg[0]+=offset;
      break;
    case 6:  // JL       (N^V)==1
      if ((GET_N()^GET_V())==1) msp_sim->reg[0]+=offset;
      break;
    case 7:  // JMP
      msp_sim->reg[0]+=offset;
      break;
    default:
      return -1;
  }

  return 0;
}

static int two_operand_exe(struct _msp_sim *msp_sim, unsigned short int opcode)
{
int o;
int src_reg,dst_reg;
int Ad,As,bw;
int dst,src;
int pc;
unsigned int result;

  o=opcode>>12;
  Ad=(opcode&0x0080)>>7;
  As=(opcode&0x0030)>>4;
  bw=(opcode&0x0040)>>6;

  src_reg=(opcode>>8)&0x000f;
  dst_reg=opcode&0x000f;

  switch(o)
  {
    case 0:
    case 1:
    case 2:
    case 3:
      return -1;
    case 4:  // MOV
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      put_data(msp_sim, pc, dst_reg, Ad, bw, src);
      break;
    case 5:  // ADD
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      result=(unsigned short int)dst+(unsigned short int)src;
      CHECK_OVERFLOW();
      dst=result&0xffff;
      put_data(msp_sim, pc, dst_reg, Ad, bw, dst);
      AFFECTS_NZ(dst);
      CHECK_CARRY(result);
      break;
    case 6:  // ADDC
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      result=(unsigned short int)dst+(unsigned short int)src+GET_C();
      //CHECK_OVERFLOW_WITH_C();
      CHECK_OVERFLOW();
      dst=result&0xffff;
      put_data(msp_sim, pc, dst_reg, Ad, bw, dst);
      AFFECTS_NZ(dst);
      CHECK_CARRY(result)
      break;
    case 7:  // SUBC
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      //src=~((unsigned short int)src)+1;
      src=((~((unsigned short int)src))&0xffff);
      //result=(unsigned short int)dst+(unsigned short int)src+GET_C();
      // FIXME - Added GET_C().  Test it.
      result=dst+src+GET_C();
      //CHECK_OVERFLOW_WITH_C();
      CHECK_OVERFLOW();
      dst=result&0xffff;
      put_data(msp_sim, pc, dst_reg, Ad, bw, dst);
      AFFECTS_NZ(dst);
      CHECK_CARRY(result)
      break;
    case 8:  // SUB
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      src=((~((unsigned short int)src))&0xffff)+1;
      //result=(unsigned short int)dst+(unsigned short int)src;
      result=dst+src;
      CHECK_OVERFLOW();
      dst=result&0xffff;
      put_data(msp_sim, pc, dst_reg, Ad, bw, dst);
      AFFECTS_NZ(dst);
      CHECK_CARRY(result)
      break;
    case 9:  // CMP
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      src=((~((unsigned short int)src))&0xffff)+1;
      //result=(unsigned short int)dst+(unsigned short int)src;
      result=dst+src;
      CHECK_OVERFLOW();
      dst=result&0xffff;
      //put_data(msp_sim, pc, dst_reg, Ad, bw, dst);
      AFFECTS_NZ(dst);
      CHECK_CARRY(result)
      break;
    case 10: // DADD
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      result=src+dst+GET_C();
      if (bw==0)
      {
        int a;
        a=(src&0xf)+(dst&0xf)+GET_C();
        a=((((src>>4)&0xf)+((dst>>4)&0xf)+(a>>4))<<4)|(a&0xf);
        a=((((src>>8)&0xf)+((dst>>8)&0xf)+(a>>8))<<8)|(a&0xff);
        a=((((src>>12)&0xf)+((dst>>12)&0xf)+(a>>12))<<12)|(a&0xfff);
        if ((a&0xffff0000)!=0) { SET_C(); } else { CLEAR_C(); }
        result=a;
      }
        else
      {
        int a;
        a=(src&0xf)+(dst&0xf)+GET_C();
        a=((((src>>4)&0xf)+((dst>>4)&0xf)+(a>>4))<<4)|(a&0xf);
        if ((a&0xffffff00)!=0) { SET_C(); } else { CLEAR_C(); }
        result=a;
      }
      put_data(msp_sim, pc, dst_reg, Ad, bw, result);
      AFFECTS_NZ(result);
      break;
    case 11: // BIT (dest & src)
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      result=src&dst;
      AFFECTS_NZ(result);
      if (result!=0) { SET_C(); } else { CLEAR_C(); }
      CLEAR_V();
      break;
    case 12: // BIC (dest &= ~src)
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      result=(~src)&dst;
      put_data(msp_sim, pc, dst_reg, Ad, bw, result);
      break;
    case 13: // BIS (dest |= src)
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      result=src|dst;
      put_data(msp_sim, pc, dst_reg, Ad, bw, result);
      break;
    case 14: // XOR
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      result=src^dst;
      put_data(msp_sim, pc, dst_reg, Ad, bw, result);
      AFFECTS_NZ(result);
      if (result!=0) { SET_C(); } else { CLEAR_C(); }
      if ((src&0x8000) && (dst&=0x8000)) { SET_V(); } else { CLEAR_V(); }
      break;
    case 15: // AND
      src=get_data(msp_sim, src_reg, As, bw);
      pc=msp_sim->reg[0];
      dst=get_data(msp_sim, dst_reg, Ad, bw);
      result=src&dst;
      put_data(msp_sim, pc, dst_reg, Ad, bw, result);
      AFFECTS_NZ(result);
      if (result!=0) { SET_C(); } else { CLEAR_C(); }
      CLEAR_V();
      break;
    default:
      return -1;
  }

  return 0;
}

struct _msp_sim *msp430_simulate_init(unsigned char *memory)
{
struct _msp_sim *msp_sim;

  msp_sim=(struct _msp_sim *)malloc(sizeof(struct _msp_sim));
  msp430_simulate_reset(msp_sim, memory);
  msp_sim->usec=1000000; // 1Hz
  msp_sim->show=1; // Show simulation
  msp_sim->step_mode=0;
  return msp_sim;
}

void msp430_simulate_push(struct _msp_sim *msp_sim, unsigned int value)
{
  msp_sim->reg[1]-=2;
  msp_sim->ram[msp_sim->reg[1]]=value&0xff;
  msp_sim->ram[msp_sim->reg[1]+1]=value>>8;
}

void msp430_simulate_set(struct _msp_sim *msp_sim, int reg, unsigned int value)
{
  msp_sim->reg[reg]=value;
}

void msp430_simulate_reset(struct _msp_sim *msp_sim, unsigned char *memory)
{
  memset(msp_sim, 0, (long)&msp_sim->usec-(long)&msp_sim->ram);
  //memset(msp_sim, 0, (&msp_sim->usec)-(&msp_sim->ram));
  msp_sim->ram=memory;
  msp_sim->reg[0]=memory[0xfffe]|(memory[0xffff]<<8);
  msp_sim->break_point=-1;
}

void msp430_simulate_free(struct _msp_sim *msp_sim)
{
  free(msp_sim);
}

void msp430_simulate_dump_registers(struct _msp_sim *msp_sim)
{
int n,sp=msp_sim->reg[1];
  printf("\nSimulation Register Dump                                  Stack\n");
  printf("-------------------------------------------------------------------\n");

  printf("        8    7    6             4   3 2 1 0              0x%04x: 0x%02x%02x\n", SHOW_STACK);
  sp_inc(&sp);
  printf("Status: V SCG1 SCG0 OSCOFF CPUOFF GIE N Z C              0x%04x: 0x%02x%02x\n", SHOW_STACK);
  sp_inc(&sp);
  printf("        %d    %d    %d      %d      %d   %d %d %d %d              0x%04x: 0x%02x%02x\n",
         (msp_sim->reg[2]>>8)&1,
         (msp_sim->reg[2]>>7)&1,
         (msp_sim->reg[2]>>6)&1,
         (msp_sim->reg[2]>>5)&1,
         (msp_sim->reg[2]>>4)&1,
         (msp_sim->reg[2]>>3)&1,
         (msp_sim->reg[2]>>2)&1,
         (msp_sim->reg[2]>>1)&1,
         (msp_sim->reg[2])&1,
         SHOW_STACK);
  sp_inc(&sp);
  printf("                                                         0x%04x: 0x%02x%02x\n", SHOW_STACK);
  sp_inc(&sp);

  printf(" PC: 0x%04x,  SP: 0x%04x,  SR: 0x%04x,  CG: 0x%04x,",
         msp_sim->reg[0],
         msp_sim->reg[1],
         msp_sim->reg[2],
         msp_sim->reg[3]);

  for (n=4; n<16; n++)
  {
    if ((n%4)==0)
    {
      printf("      0x%04x: 0x%02x%02x", SHOW_STACK);
      printf("\n");
      sp_inc(&sp);
    }
      else
    { printf(" "); }

    char reg[4];
    sprintf(reg, "r%d",n);
    printf("%3s: 0x%04x,", reg, msp_sim->reg[n]);
  }
  printf("      0x%04x: 0x%02x%02x", SHOW_STACK);
  printf("\n\n");
  printf("%d clock cycles have passed since last reset.\n\n", msp_sim->cycle_count);
}

int msp430_simulate_run(struct _msp_sim *msp_sim, int max_cycles, int step)
{
char instruction[128];
unsigned short int opcode;
int cycles=0;
int ret;
int pc;
int c;
int n;

  stop_running=0;
  signal(SIGINT, handle_signal);

  printf("Running... Press Ctl-C to break.\n");

  while(stop_running==0)
  {
    pc=msp_sim->reg[0];
    opcode=msp_sim->ram[pc]|(msp_sim->ram[pc+1]<<8);
    c=get_cycle_count(opcode);
    if (c>0) msp_sim->cycle_count+=c;
    msp_sim->reg[0]+=2;

    if (msp_sim->show==1) printf("\x1b[1J\x1b[1;1H");

    if ((opcode&0xfc00)==0x1000)
    {
      ret=one_operand_exe(msp_sim, opcode);
    }
      else
    if ((opcode&0xe000)==0x2000)
    {
      ret=relative_jump_exe(msp_sim, opcode);
    }
      else
    {
      ret=two_operand_exe(msp_sim, opcode);
    }

    if (c>0) cycles+=c;

    if (msp_sim->show==1)
    {
      msp430_simulate_dump_registers(msp_sim);

      n=0;
      while(n<6)
      {
        int cycles;
        int num;
        num=(msp_sim->ram[pc+1]<<8)|msp_sim->ram[pc];
        int count=msp430_disasm(msp_sim->ram, pc, instruction, &cycles);
        if (cycles==-1) break;

        if (pc==msp_sim->break_point) { printf("*"); }
        else { printf(" "); }

        if (n==0)
        { printf("! "); }
          else
        if (pc==msp_sim->reg[0]) { printf("> "); }
          else
        { printf("  "); }

        printf("0x%04x: 0x%04x %-40s %d\n", pc, num, instruction, cycles);
        n=n+count;
        pc+=2;
        count--;
        while (count>0)
        {
          if (pc==msp_sim->break_point) { printf("*"); }
          else { printf(" "); }
          num=(msp_sim->ram[pc+1]<<8)|msp_sim->ram[pc];
          printf("  0x%04x: 0x%04x\n", pc, num);
          pc+=2;
          count--;
        }
      }
    }

    if (ret==-1)
    {
      printf("Illegal instruction at address 0x%04x\n", pc);
      return -1;
    }

    if (max_cycles!=-1 && cycles>max_cycles) break;
    if (msp_sim->break_point==msp_sim->reg[0])
    {
       printf("Breakpoint hit at 0x%04x\n", msp_sim->break_point);
      break;
    }

    if (msp_sim->usec==0 || step==1)
    {
      //msp_sim->step_mode=0;
      signal(SIGINT, SIG_DFL);
      return 0;
    }

    if (msp_sim->reg[0]==0xffff)
    {
      printf("Function ended.  Total cycles: %d\n", msp_sim->cycle_count);
      msp_sim->step_mode=0;
      msp_sim->reg[0]=msp_sim->ram[0xfffe]|(msp_sim->ram[0xffff]<<8);
      signal(SIGINT, SIG_DFL);
      return 0;
    }

    usleep(msp_sim->usec);
  }

  signal(SIGINT, SIG_DFL);
  printf("Stopped.  PC=0x%04x.\n", msp_sim->reg[0]);
  printf("%d clock cycles have passed since last reset.\n", msp_sim->cycle_count);

  return 0;
}


