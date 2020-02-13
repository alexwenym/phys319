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

#include "msp430_disasm.h"

static char *regs[] = { "PC", "SP", "SR", "CG", "r4", "r5", "r6", "r7", "r8",
                        "r9", "r10", "r11", "r12", "r13", "r14", "r15" };

static int get_source_reg(unsigned char *memory, int address, int reg, int As, int bw, char *reg_str)
{
int count=0;

  reg_str[0]=0;

  if (reg==0)
  {
    if (As==0)
    { strcat(reg_str, regs[reg]); }
      else
    if (As==1)
    {
      unsigned short int a=(memory[address+3]<<8)|memory[address+2];
      count=count+1;
      a=a+(address+(count<<1));
      sprintf(reg_str, "0x%04x", a);
    }
      else
    if (As==2)
    { strcpy(reg_str, "@PC"); }
      else
    if (As==3)
    {
      unsigned short int a=(memory[address+3]<<8)|memory[address+2];
      count=count+1;
      if (bw==0)
      { sprintf(reg_str, "#0x%04x", a); }
        else
      { sprintf(reg_str, "#0x%02x", a); }
    }
  }
    else
  if (reg==2)
  {
    if (As==0)
    { strcat(reg_str, regs[reg]); }
      else
    if (As==1)
    {
      unsigned short int a=(memory[address+3]<<8)|memory[address+2];
      count=count+1;
      sprintf(reg_str, "&0x%04x", a);
    }
      else
    if (As==2)
    { strcat(reg_str, "#4"); }
      else
    if (As==3)
    { strcat(reg_str, "#8"); }
  }
    else
  if (reg==3)
  {
    if (As==0)
    { strcat(reg_str, "#0"); }
      else
    if (As==1)
    { strcat(reg_str, "#1"); }
      else
    if (As==2)
    { strcat(reg_str, "#2"); }
      else
    if (As==3)
    { strcat(reg_str, "#-1"); }
  }
    else
  {
    if (As==0)
    {
      strcat(reg_str, regs[reg]);
    }
      else
    if (As==1)
    {
      unsigned short int a=(memory[address+3]<<8)|memory[address+2];
      count=count+1;
      sprintf(reg_str, "0x%x(%s)", a, regs[reg]);
    }
      else
    if (As==2)
    {
      sprintf(reg_str, "@%s", regs[reg]);
    }
      else
    if (As==3)
    {
      sprintf(reg_str, "@%s+", regs[reg]);
    }
  }

  return count;
}

static int get_dest_reg(unsigned char *memory, int address, int reg, int Ad, char *reg_str, int count)
{
  reg_str[0]=0;

  if (reg==0)
  {
    if (Ad==0)
    { strcat(reg_str, regs[reg]); }
      else
    if (Ad==1)
    {
      unsigned short int a=(memory[address+(count*2)+3]<<8)|memory[address+(count*2)+2];
      count=count+1;
      a=a+(address+(count<<1));
      sprintf(reg_str, "0x%04x", a);
    }
  }
    else
  if (reg==2)
  {
    if (Ad==0)
    { strcat(reg_str, regs[reg]); }
      else
    if (Ad==1)
    {
      unsigned short int a=(memory[address+(count*2)+3]<<8)|memory[address+(count*2)+2];
      count=count+1;
      sprintf(reg_str, "&0x%04x", a);
    }
  }
    else
  if (reg==3)
  {
    strcat(reg_str, regs[reg]);
  }
    else
  {
    if (Ad==0)
    {
      strcat(reg_str, regs[reg]);
    }
      else
    if (Ad==1)
    {
      unsigned short int a=(memory[address+(count*2)+3]<<8)|memory[address+(count*2)+2];
      count=count+1;
      sprintf(reg_str, "0x%x(%s)", a, regs[reg]);
    }
  }

  return count;
}

static int one_operand(unsigned char *memory, int address, char *instruction, unsigned short int opcode)
{
char *instr[] = { "rrc", "swpb", "rra", "sxt", "push", "call", "reti", "???" };
int o;
int reg;
int As;
int count=1;
int bw=0;

  o=(opcode&0x0380)>>7;

  strcpy(instruction, instr[o]);
  if (o==7) { return 1; }
  if (o==6) { return count; }

  As=(opcode&0x0030)>>4;
  reg=opcode&0x000f;

  if ((opcode&0x0040)==0)
  {
    if (((opcode>>7)&1)==0 && ((opcode>>7)&7)!=6)
    {
      strcat(instruction, ".w");
    }
  }
    else
  {
    if (o==1 || o==3 || o==5 || o==6) { strcpy(instruction, "???"); return 1; }
    strcat(instruction, ".b");
    bw=1;
  }

  strcat(instruction, " ");

  char reg_str[128];
  count+=get_source_reg(memory, address, reg, As, bw, reg_str);
  strcat(instruction, reg_str);

  return count;
}

static int relative_jump(unsigned char *memory, int address, char *instruction, unsigned short int opcode)
{
char *instr[] = { "jne", "jeq", "jlo", "jhs", "jn", "jge", "jl", "jmp" };
int o;

  o=(opcode&0x1c00)>>10;
  if (o>7)
  {
    printf("WTF JMP?\n");
    return 1;
  }

  strcpy(instruction, instr[o]);

  int offset=opcode&0x03ff;
  if ((offset&0x0200)!=0)
  {
    offset=-((offset^0x03ff)+1);
  }

  offset*=2;

  char token[128];
  sprintf(token, " 0x%04x  (offset: %d)", ((address+2)+offset)&0xffff, offset);
  strcat(instruction, token);

  return 1;
}

static int two_operand(unsigned char *memory, int address, char *instruction, unsigned short int opcode)
{
char *instr[] = { "mov", "add", "addc", "subc", "sub", "cmp", "dadd", "bit",
                  "bic", "bis", "xor", "and" };
int o;
int Ad,As;
int count=0;
int bw=0;

  o=opcode>>12;
  if (o<4 || o>15)
  {
    strcpy(instruction, "???");
    return 1;
  }

  o=o-4;

  if ((opcode&0x00ff)==0x0003)
  { sprintf(instruction, "nop   --  %s", instr[o]); }
    else
  if (opcode==0x4130)
  { sprintf(instruction, "ret   --  %s", instr[o]); }
    else
  if ((opcode&0xff30)==0x4130)
  { sprintf(instruction, "pop r%d   --  %s", opcode&0x000f, instr[o]); }
    else
  if (opcode==0xc312)
  { sprintf(instruction, "clrc  --  %s", instr[o]); }
    else
  if (opcode==0xc222)
  { sprintf(instruction, "clrn  --  %s", instr[o]); }
    else
  if (opcode==0xc322)
  { sprintf(instruction, "clrz  --  %s", instr[o]); }
    else
  if (opcode==0xc232)
  { sprintf(instruction, "dint  --  %s", instr[o]); }
    else
  if (opcode==0xd312)
  { sprintf(instruction, "setc  --  %s", instr[o]); }
    else
  if (opcode==0xd222)
  { sprintf(instruction, "setn  --  %s", instr[o]); }
    else
  if (opcode==0xd322)
  { sprintf(instruction, "setz  --  %s", instr[o]); }
    else
  if (opcode==0xd232)
  { sprintf(instruction, "eint  --  %s", instr[o]); }
    else
  { strcpy(instruction, instr[o]); }

  Ad=(opcode&0x0080)>>7;
  As=(opcode&0x0030)>>4;

  if ((opcode&0x0040)==0)
  { strcat(instruction, ".w"); }
    else
  { strcat(instruction, ".b"); bw=1; }

  strcat(instruction, " ");

  char reg_str[128];
  count=get_source_reg(memory, address, (opcode&0x0f00)>>8, As, bw, reg_str);
  strcat(instruction, reg_str);

  strcat(instruction, ", ");
  count=get_dest_reg(memory, address, opcode&0x000f, Ad, reg_str, count);
  strcat(instruction, reg_str);

  //int src_reg=(opcode>>8)&0x000f;
  //int dst_reg=opcode&0x000f;

  return count+1;
}

#ifdef SUPPORT_20_BIT
static int get_20bit(unsigned char *memory, int address, unsigned int opcode)
{
  return ((opcode>>8)&0x0f)<<16|
         (memory[address+3]<<8)|
         (memory[address+2]);
}

static int twenty_bit_zero(unsigned char *memory, int address, char *instruction, unsigned short int opcode)
{
char *instr[] = { "rrcm", "rram", "rlam", "rrum" };
char *instr2[] = { "mova", "cmpa", "adda", "suba" };
int o;

  o=(opcode>>4)&0x0f;

  switch(o)
  {
  case 0:
    sprintf(instruction, "mova @r%d, r%d", (opcode>>8)&0x0f, (opcode&0x0f));
    return 1;
  case 1:
    sprintf(instruction, "mova @r%d+, r%d", (opcode>>8)&0x0f, (opcode&0x0f));
    return 1;
  case 2:
    sprintf(instruction, "mova &%05x, r%d", get_20bit(memory,address,opcode), (opcode&0x0f));
    return 2;
  case 3:
    sprintf(instruction, "mova %d(r%d), r%d", (memory[address+3]<<8)|memory[address+2],(opcode>>8)&0xff, (opcode&0x0f));
    return 2;
  case 4:
  case 5:
    sprintf(instruction, "%s.%c #%d, r%d", instr[(opcode>>8)&3], ((opcode&64)==0)?'w':'a', (opcode>>10)&3, opcode&0x000f);
    return 1;
  case 6:
    sprintf(instruction, "mova r%d, &0x%05x", (opcode>>8)&0x0f, ((opcode&0x0f)<<16)|(memory[address+3]<<8)|memory[address+2]);
    return 2;
  case 7:
    sprintf(instruction, "mova r%d, %d(r%d)", (opcode>>8)&0x0f, (memory[address+3]<<8)|memory[address+2],  opcode&0x0f);
    return 2;
  case 8:
  case 9:
  case 10:
  case 11:
    sprintf(instruction, "%s #%d, r%d", instr2[(opcode>>4)&3], get_20bit(memory,address,opcode), opcode&0x0f);
    return 2;
  case 12:
  case 13:
  case 14:
  case 15:
    sprintf(instruction, "%s r%d, r%d", instr2[(opcode>>4)&3], (opcode>>8)&0x0f, opcode&0x0f);
    return 1;
  default:
    strcpy(instruction, "???");
    return 1;
  }
}

static int twenty_bit_call(unsigned char *memory, int address, char *instruction, unsigned short int opcode)
{
  if ((opcode&0x00ff)==0) { strcpy(instruction, "reti"); return 1; }

  int o=(opcode>>6)&0x03;
  int mode=(opcode>>4)&0x03;

/*
  if (o==0x03 || (o==0x02 && mode==0x02))
  {
    strcpy(instruction, "???");
    return 1;
  }
*/

  if (o==0x01) // calla source, As=mode
  {
    int reg=opcode&0x000f;
    char reg_str[128];
 
    count+=get_source_reg(memory, address, mode, As, 0, reg_str);
    sprintf(instruction, "calla %s", reg_str);

/*
    return count;
    if (mode==0) { sprintf(instruction, "calla r%d", reg); }
    else if (mode==1) { sprintf(instruction, "calla x(r%d)", reg); }
    else if (mode==2) { sprintf(instruction, "calla @r%d", reg); }
    else if (mode==3) { sprintf(instruction, "calla @r%d+", reg); }
*/
  }
    else
  if (o==0x02)
  {
    if (mode==0) // calla &abs20
    {
      sprintf(instruction, "calla &%d", get_20bit(memory,address,opcode));
    return 2;
    }
      else
    if (mode==1) // calla x(PC)
    {
      sprintf(instruction, "calla %d(PC)", get_20bit(memory,address,opcode));
    }
      else
    if (mode==3) // calla #immediate
    {
      sprintf(instruction, "calla #%d", get_20bit(memory,address,opcode));
    }
  }

  strcpy(instruction, "???");
  return 1;
}

static int twenty_bit_stack(unsigned char *memory, int address, char *instruction, unsigned short int opcode)
{
char temp[8];
int n=((opcode>>4)&0xf)+1;
int is_push=(opcode&0x0200)==0?1:0;

  sprintf(instruction, "%s", (is_push==1)?"pushm":"popm");
  strcat(instruction, (opcode&0x0100)==0?".w ":".a ");
  sprintf(temp, "#%d, ", n);
  strcat(instruction, temp);
  if (is_push==1)
  {
    sprintf(temp, "r%d", (opcode&0xf));
  }
    else
  {
    sprintf(temp, "r%d", (opcode&0xf)-n+1);
  }
  strcat(instruction, temp);

  return 1;
}
#endif

int get_cycle_count(unsigned short int opcode)
{
int src_reg,dst_reg;
int Ad,As;

  if ((opcode&0xfc00)==0x1000)
  {
    // One operand
    int o=(opcode&0x0380)>>7;
    As=(opcode&0x0030)>>4;
    src_reg=opcode&0x000f;

    if (((opcode&0x0040)!=0) && (o==1 || o==3 || o==5 || o==6)) { return -1; }

    if (src_reg==3 || (src_reg==2 && (As&2)==2)) { As=0; }

    if (o==6) { return 5; }    // RETI
    if (o==7) { return -1; }   // Illegal

    if (o==5) // CALL
    {
      if (As==1) return 5;        // x(Rn), EDE, &EDE
      if (As==2) return 4;        // @Rn
      if (As==3)
      {
        if (src_reg==0) return 5; // #value
        return 5;                 // @Rn+
      }

      return 4;                   // Rn
    }

    if (o==4) // PUSH
    {
      if (As==1) return 5;        // x(Rn), EDE, &EDE
      if (As==2) return 4;        // @Rn
      if (As==3)
      {
        if (src_reg==0) return 4; // #value
        return 5;                 // @Rn+
      }

      return 3;                   // Rn
    }

    // RRA, RRC, SWPB, SXT
    if (As==1) return 4;        // x(Rn), EDE, &EDE
    if (As==2) return 3;        // @Rn
    if (As==3)
    {
      if (src_reg==0) return -1; // #value
      return 3;                  // @Rn+
    }

    return 1;                   // Rn
  }
    else
  if ((opcode&0xe000)==0x2000)
  {
    // Jump
    return 2; 
  }
    else
  {
    // Two operand
    Ad=(opcode&0x0080)>>7;
    As=(opcode&0x0030)>>4;
    src_reg=(opcode>>8)&0x000f;
    dst_reg=opcode&0x000f;

    if (src_reg==3 || (src_reg==2 && (As&2)==2)) { As=0; }
    if (dst_reg==3) { Ad=0; }

    if ((opcode>>12)<4) return -1;

    // Cycle counts
    if (As==1) //Src EDE, &EDE, x(Rn)
    {
      if (Ad==1) return 6; // Dest x(Rn) and TONI and &TONI
      return 3;  // Dest
    }
      else
    if (As==3)  // #value and @Rn+
    {
      if (dst_reg==0) return 3;   // Dest PC
      if (Ad==0) return 2;        // Dest Rm
      return 5;                   // Dest x(Rm), EDE, &EDE
    }
      else
    if (As==2)  // @Rn
    {
      if (dst_reg==0) return 2;   // Dest PC
      if (Ad==0) return 2;        // Dest Rm
      return 5;                   // Dest x(Rm), EDE, &EDE
    }
      else     // Rn
    {
      if (dst_reg==0) return 2;   // Dest PC
      if (Ad==0) return 1;        // Dest Rm
      return 4;                   // Dest x(Rm), EDE, &EDE
    }
  }

  return -1;
}

int msp430_disasm(unsigned char *memory, int address, char *instruction, int *cycles)
{
unsigned short int opcode;

  instruction[0]=0;
  opcode=(memory[address+1]<<8)|memory[address];
  *cycles=get_cycle_count(opcode);

#ifdef SUPPORT_20_BIT
  // Extend 16 bit instruction to 20 bit
  if ((opcode&0xf800)==0x1800)
  {
    printf("Unsupport extension.\n");
  }
#endif

  if ((opcode&0xfc00)==0x1000)
  {
    return one_operand(memory, address, instruction, opcode);
  }
    else
  if ((opcode&0xe000)==0x2000)
  {
    return relative_jump(memory, address, instruction, opcode);
  }
    else
  {
    return two_operand(memory, address, instruction, opcode);
  }
#ifdef SUPPORT_20_BIT
    else
  if ((opcode&0xf000)==0x0000)
  {
    return twenty_bit_zero(memory, address, instruction, opcode);
  }
    else
  if ((opcode&0xff00)==0x1300)
  {
    return twenty_bit_call(memory, address, instruction, opcode);
  }
    else
  if ((opcode&0xf000)==0x1400)
  {
    return twenty_bit_stack(memory, address, instruction, opcode);
  }
#endif
}



