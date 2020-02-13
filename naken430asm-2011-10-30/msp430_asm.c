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

#include "assembler.h"
#include "eval_expression.h"
#include "get_tokens.h"
#include "lookup_tables.h"
#include "msp430_asm.h"

enum
{
  ALIAS_NONE,
  ALIAS_NEG_ONE,
  ALIAS_ZERO,
  ALIAS_ONE,
  ALIAS_TWO,
  ALIAS_SP_PLUS,
  ALIAS_SELF,
  ALIAS_PC
};

#define IS_DATA 0
#define IS_OPCODE 1

static void add_bin(struct _asm_context *asm_context, unsigned short int b, int flags)
{
  if (asm_context->low_address>asm_context->address)
  {
    asm_context->low_address=asm_context->address;
  }

  if (asm_context->high_address<asm_context->address+1)
  {
    asm_context->high_address=asm_context->address+1;
  }

  if (asm_context->pass==1)
  {
    asm_context->debug_line[asm_context->address]=-3;
    asm_context->debug_line[asm_context->address+1]=-3;
  }
    else
  {
    if (flags==IS_OPCODE)
    { asm_context->debug_line[asm_context->address]=asm_context->line; }
      else
    { asm_context->debug_line[asm_context->address]=-3; }
    asm_context->debug_line[asm_context->address+1]=-3;
  }

  // 1 little, 2 little, 3 little endian
  asm_context->bin[asm_context->address++]=b&0xff;
  asm_context->bin[asm_context->address++]=b>>8;

  if (asm_context->address>=asm_context->memory_size)
  { asm_context->address=0; }
}

static int get_register(char *token)
{
  if (token[0]=='r' || token[0]=='R')
  {
    if (token[2]==0 && (token[1]>='0' && token[1]<='9'))
    {
      return token[1]-'0';
    }
      else
    if (token[3]==0 && token[1]=='1' && (token[2]>='0' && token[2]<='5'))
    {
      return 10+(token[2]-'0');
    }
  }

  if (strcasecmp(token, "pc")==0) return 0;
  if (strcasecmp(token, "sp")==0) return 1;
  if (strcasecmp(token, "sr")==0) return 2;
  if (strcasecmp(token, "cg")==0) return 3;

  return -1;
}

static int eat_molecule(struct _asm_context *asm_context)
{
char token[TOKENLEN];
int token_type;

  // Eat all tokens until an ',' or EOL
  while(1)
  {
    token_type=get_token(asm_context, token, TOKENLEN);

    if (IS_TOKEN(token,',') || token_type==TOKEN_EOL)
    {
      pushback(asm_context, token, token_type);
      return 0;
    }
  }

  return -1;
}

static int parse_molecule(struct _asm_context *asm_context, char *token, int token_type, int bw, int *as, int *reg, int *immediate, int is_source, int noconstant_flag)
{
//int token_type;
int r=get_register(token);

  if (r>=0)
  {
#ifdef DEBUG
printf("parse_molecule: r%d\n", r);
#endif
    *reg=r;
  }
    else
  if (IS_TOKEN(token,'#'))
  {
#ifdef DEBUG
printf("parse_molecule: #\n");
#endif
    int num;
    if (eval_expression(asm_context, &num)!=0)
    {
      if (asm_context->pass==1)
      {
         eat_molecule(asm_context);
         *immediate=-2;
         return 0;
      }
      return -1;
    }

    if (num>0xffff || num<-32768)
    {
      print_error("Constant larger than 16 bit.", asm_context);
      return -1;
    }

    if (noconstant_flag!=0)
    {
      *reg=0; *as=3;
      *immediate=(unsigned short int)num;
/*
      if (bw==0)
      { (*immediate)&=0xffff; }
        else
      { (*immediate)&=0xff; }
*/
      return 0;
    }

    if (num==0) { *reg=3; *as=0; }
      else
    if (num==1) { *reg=3; *as=1; }
      else
    if (num==2) { *reg=3; *as=2; }
      else
    if (num==4) { *reg=2; *as=2; }
      else
    if (num==8) { *reg=2; *as=3; }
      else
    if (bw==0 && (unsigned short int)num==0xffff) { *reg=3; *as=3; }
      else
    if (bw==1 && ((unsigned short int)num&0xff)==0xff) { *reg=3; *as=3; }
      else
    {
      *reg=0; *as=3;
      *immediate=((unsigned short int)num);
/*
      if (bw==0)
      { (*immediate)&=0xffff; }
        else
      { (*immediate)&=0xff; }
*/
    }
  }
    else
  if (IS_TOKEN(token,'&'))
  {
    int address;
    if (eval_expression(asm_context, &address)!=0)
    {
      if (asm_context->pass==1)
      {
         eat_molecule(asm_context);
         *immediate=0;
         return 0;
      }
      return -1;
    }
#ifdef DEBUG
printf("parse_molecule: &  address=%d\n", address);
#endif
    *immediate=address;
    *as=1;
    *reg=2;

    if (bw==0 && (address&1)==1)
    {
      printf("Warning: 16bit operation on unaligned address at %s:%d\n", asm_context->filename, asm_context->line);
    }
  }
    else
  if (IS_TOKEN(token,'@'))
  {
    *as=2;
    token_type=get_token(asm_context, token, TOKENLEN);

    *reg=get_register(token);
    if (*reg<0)
    {
      printf("Expecting register and got '%s' on line %d.\n", token, asm_context->line);
      return -1;
    }

    token_type=get_token(asm_context, token, TOKENLEN);
    if (IS_TOKEN(token,'+'))
    { *as=3; }
      else
    { pushback(asm_context, token, token_type); }
#ifdef DEBUG
printf("parse_molecule: @r%d%c\n", *reg, *as==3?'+':' ');
#endif
  }
    else // Check X(Rn) or LABEL instruction
  {
    if (asm_context->pass==1)
    {
      // this molecule should have an immediate..
      *immediate=0;
      eat_molecule(asm_context);
    }
      else
    {
      pushback(asm_context, token, token_type);

      int num;
      if (eval_expression(asm_context, &num)!=0)
      {
        return -1;
      }

      *immediate=num;
      *as=1;

      token_type=get_token(asm_context, token, TOKENLEN);
      if (IS_NOT_TOKEN(token,'('))
      {
        if (bw==0 && (num&1)==1)
        {
          printf("Warning: 16bit operation on unaligned address at %s:%d\n", asm_context->filename, asm_context->line);
        }

        // LABEL format instruction
        pushback(asm_context, token, token_type);
        *reg=0;
      }
        else
      {
        // Test for x(Rn) format
        token_type=get_token(asm_context, token, TOKENLEN);
        *reg=get_register(token);
        if (*reg<0)
        {
          printf("Expecting register and got '%s' on line %d.\n", token, asm_context->line);
          return -1;
        }

        token_type=get_token(asm_context, token, TOKENLEN);
        if (IS_NOT_TOKEN(token,')'))
        {
          print_error_unexp(token, asm_context);
          return -1;
        }
      }
    }
  }

  if (is_source==0)
  {
    if (*as==2)
    {
      if (asm_context->pass==2)
      {
        printf("Warning: Addressing mode of @r%d being changed to 0(r%d) on line %d.\n", *reg, *reg, asm_context->line);
      }
      *as=1;
      *immediate=0;
    }
      else
    if (*as==3)
    {
      print_error("Illegal addressing mode for destination", asm_context);
      return -1;
    }
  }

  return 0;
}

static int get_bw_bit(struct _asm_context *asm_context, char *token, int *token_type, int *bw)
{
  *bw=0;

  *token_type=get_token(asm_context, token, TOKENLEN);

  if (IS_TOKEN(token,'.'))
  {
    *token_type=get_token(asm_context, token, TOKENLEN);
    if (token[1]==0)
    {
      if (token[0]=='b' || token[0]=='B') { *bw=1; }
        else
      if (token[0]=='w' || token[0]=='W') { *bw=0; }
        else
      {
        print_error("Expecting 'b' or 'w'", asm_context);
        return -1;
      }
    }

    *token_type=get_token(asm_context, token, TOKENLEN);
  }

  return 0;
}

static int asm_single_operand_arithmetic(struct _asm_context *asm_context, unsigned short int opcode)
{
char token[TOKENLEN];
int token_type;
int bw;
int as=0;
int reg=0;
int immediate=-1;
int noconstant_flag=0;

  if (opcode==0x1100 || opcode==0x1200 || opcode==0x1000)
  {
    if (get_bw_bit(asm_context, token, &token_type, &bw)!=0)
    {
      return -1;
    }
  }
    else
  {
    token_type=get_token(asm_context, token, TOKENLEN);
    if (IS_TOKEN(token,'.'))
    {
      print_error("Instruction does not take a .b or .w", asm_context);
      return -1;
    }

    bw=0;
  }

  if (asm_context->pass==2)
  {
    noconstant_flag=asm_context->bin[asm_context->address];
  }

  if (parse_molecule(asm_context, token, token_type, bw, &as, &reg, &immediate, 1, noconstant_flag)!=0)
  {
    return -1;
  }

  if (asm_context->pass==1)
  {
    // Mark this address so it always takes the long form if immediate==-2
    if (immediate==-2)
    { add_bin(asm_context, 1, IS_DATA); }
      else
    { add_bin(asm_context, 0, IS_DATA); }
  }
    else
  {
    opcode|=(bw<<6)|(as<<4)|reg;
    add_bin(asm_context, opcode, IS_OPCODE);
  }

  if (immediate!=-1)
  {
    if (reg==0 && as==1)
    {
      int offset=immediate-(asm_context->address);
      add_bin(asm_context, (unsigned short int)(offset&0xffff), IS_DATA);
    }
      else
    {
      add_bin(asm_context, (unsigned short int)immediate, IS_DATA);
    }
  }

  return 0;
}

static int asm_conditional(struct _asm_context *asm_context, unsigned short int opcode)
{
int offset;

  if (eval_expression(asm_context, &offset)!=0)
  {
    if (asm_context->pass==2)
    { print_error("Unknown symbol", asm_context); }
    return -1;
  }

  if ((offset&1)==1)
  {
    print_error("Jump offset is odd", asm_context);
    return -1;
  }

  offset=(offset-(asm_context->address+2))/2;

  if (offset>511)
  {
    printf("Error: Jump offset off by %d at %s:%d.\n", offset-511, asm_context->filename, asm_context->line);
    return -1;
  }
    else
  if (offset<-512)
  {
    printf("Error: Jump offset off by %d at %s:%d.\n", (-offset)-512, asm_context->filename, asm_context->line);
    return -1;
  }

  opcode|=((unsigned int)offset)&0x03ff;

  add_bin(asm_context, opcode, IS_OPCODE);

  return 0;
}

static int asm_two_operand_arithmetic(struct _asm_context *asm_context, unsigned short int opcode, int alias)
{
char token[TOKENLEN];
int token_type;
int bw=0;
int as=0;
int ad=0;
int src_reg=0;
int dst_reg=0;
int src_immediate=-1;
int dst_immediate=-1;
int noconstant_flag=0;

  if (get_bw_bit(asm_context, token, &token_type, &bw)!=0)
  {
    return -1;
  }

  if (asm_context->pass==2)
  {
    noconstant_flag=asm_context->bin[asm_context->address];
#ifdef DEBUG
printf("debug> address=%04x noconstant_flag=%d\n",asm_context->address, noconstant_flag);
#endif
  }

  if (alias==ALIAS_NONE || alias==ALIAS_PC)
  {
    if (parse_molecule(asm_context, token, token_type, bw, &as, &src_reg, &src_immediate, 1, noconstant_flag&1)!=0)
    {
      return -1;
    }

    if (alias!=ALIAS_PC)
    {
      token_type=get_token(asm_context, token, TOKENLEN);
      if (IS_NOT_TOKEN(token,','))
      {
        printf("Error: Expecting ',' but got '%s' instead at %s:%d\n", token, asm_context->filename, asm_context->line);
        return -1;
      }
      token_type=get_token(asm_context, token, TOKENLEN);
    }
  }
    else
  {
    switch(alias)
    {
      case ALIAS_NEG_ONE:
        src_reg=3;
        as=3;
        break;
      case ALIAS_ZERO:
        src_reg=3;
        as=0;
        break;
      case ALIAS_ONE:
        src_reg=3;
        as=1;
        break;
      case ALIAS_TWO:
        src_reg=3;
        as=2;
        break;
      case ALIAS_SP_PLUS:
        src_reg=1;
        as=3;
        break;
      case ALIAS_SELF:
      case ALIAS_PC:
        break;
      default:
        printf("Internal error: %s:%d\n", __FILE__, __LINE__);
        exit(1);
    }
  }

  if (alias!=ALIAS_PC)
  {
    if (parse_molecule(asm_context, token, token_type, bw, &ad, &dst_reg, &dst_immediate, 0, noconstant_flag&2)!=0)
    {
      return -1;
    }
  }
    else
  {
    ad=0;
    dst_reg=0;
  }

  if (alias==ALIAS_SELF)
  {
    as=ad;
    src_reg=dst_reg;
    src_immediate=dst_immediate;
  }

  if (asm_context->pass==1)
  {
    int args=0;

    // Mark this address so it always takes the long form if immediate==-2
    if (src_immediate==-2) args|=1;
    if (dst_immediate==-2) args|=2;
    add_bin(asm_context, args, IS_DATA);
  }
    else
  {
    opcode|=(src_reg<<8)|(ad<<7)|(bw<<6)|(as<<4)|(dst_reg);
#ifdef DEBUG
printf("opcode=%04x src_reg=%d dst_reg=%d ad=%d, bw=%d as=%d dst_rg=%d\n       src_immediate=%d(0x%04x) dst_immediate=%d(0x%04x)\n", opcode, src_reg, dst_reg, ad,bw,as,dst_reg, src_immediate, src_immediate, dst_immediate, dst_immediate);
#endif
    add_bin(asm_context, opcode, IS_OPCODE);
  }

  if (src_immediate!=-1)
  {
//printf("%d %d %d %d\n", src_immediate, bw, src_reg, as);

    if (src_immediate>0xffff)
    {
      print_error("Source is bigger than 16 bit.", asm_context);
      return -1;
    }

    if (src_immediate>0xff && bw==1 && src_reg==0 && as==3)
    {
      print_error("Source is bigger than 8 bit.", asm_context);
      return -1;
    }

    if (as==1 && src_reg==0)
    {
      int offset=src_immediate-(asm_context->address);
      add_bin(asm_context, (unsigned short int)(offset&0xffff), IS_DATA);
    }
      else
    {
      add_bin(asm_context, (unsigned short int)src_immediate, IS_DATA);
    }
  }

  if (dst_immediate!=-1)
  {
    if (dst_immediate>0xffff)
    {
      print_error("Destination is bigger than 16 bit.", asm_context);
      return -1;
    }

    if (ad==1 && dst_reg==0)
    {
      int offset=dst_immediate-(asm_context->address);
      add_bin(asm_context, (unsigned short int)(offset&0xffff), IS_DATA);
    }
      else
    {
      add_bin(asm_context, (unsigned short int)dst_immediate, IS_DATA);
    }
  }

  return 0;
}

int parse_instruction(struct _asm_context *asm_context, char *instr)
{
char *one_oper[] = { "rrc", "swpb", "rra", "sxt", "push", "call", "reti", 0 };
char *jumps[] = { "jne", "jeq", "jlo", "jhs", "jn", "jge", "jl", "jmp", 0 };
char *jumps_a[] = { "jnz", "jz", "jnc", "jc", 0, 0, 0, 0, 0 };
char *two_oper[] = { "mov", "add", "addc", "subc", "sub", "cmp", "dadd", "bit",
                     "bic", "bis", "xor", "and", 0 };
unsigned short int opcode;  // really more than opcode, but...
int n;

  // Not sure if this is a good area for this.  If there isn't an instruction
  // here then it pads for no reason.
  if ((asm_context->address&0x01)!=0)
  {
    if (asm_context->pass==2)
    {
      printf("Warning: Instruction doesn't start on 16 bit boundary at %s:%d.  Padding with a 0.\n", asm_context->filename, asm_context->line);
    }
    //asm_context->debug_line[asm_context->address]=-2;
    //asm_context->data_count++;
    //asm_context->bin[asm_context->address++]=0;
    asm_context->address++;
    if (asm_context->address>=asm_context->memory_size)
    { asm_context->address=0; asm_context->high_address=asm_context->memory_size-1; }
  }

  if (asm_context->address>=asm_context->memory_size)
  {
    printf("Error: Assembly address 0x%04x is higher than memory size %d bytes.\n", asm_context->address, asm_context->memory_size);
    return -1;
  }

#ifdef DEBUG
  printf("###### pass %d> instr=%s address=%04x\n", asm_context->pass, instr, asm_context->address);
#endif

/*
  if (asm_context->pass==2)
  {
if (asm_context->line==396) { printf("ZOMG! %d\n", asm_context->address); }
else { printf("okay %d\n", asm_context->address); }
    asm_context->debug_line[asm_context->address]=asm_context->line;
  }
*/

  // Check single operand
  n=0;
  while(1)
  {
    if (one_oper[n]==0) break;
    if (strcasecmp(instr,one_oper[n])==0)
    {
      opcode=0x1000|(n<<7);
      asm_context->instruction_count++;
      if (n!=6)
      {
        if (asm_single_operand_arithmetic(asm_context, opcode)==-1) return -1;
      }
        else
      {
        int token_type;
        char token[TOKENLEN];
        token_type=get_token(asm_context, token, TOKENLEN);
        if (token_type!=TOKEN_EOL && token_type!=TOKEN_EOF)
        {
          print_error_unexp(token, asm_context);
        }
        if (asm_context->defines_heap.stack_ptr==0) { asm_context->line++; }
        add_bin(asm_context, opcode, IS_OPCODE);
      }
      return 0;
    }

    n++;
  }

  // Check jump
  n=0;
  while(1)
  {
    if (jumps[n]==0) break;
    if (strcasecmp(instr,jumps[n])==0 ||
        (jumps_a[n]!=0 && strcasecmp(instr,jumps_a[n])==0))
    {
      opcode=0x2000|(n<<10);
      asm_context->instruction_count++;

      if (asm_context->pass==1)
      {
        eat_molecule(asm_context);
        add_bin(asm_context, opcode, IS_OPCODE);
      }
        else
      {
        if (asm_conditional(asm_context, opcode)==-1) return -1;
      }
      return 0;
    }

    n++;
  }

  // Check two operand

  n=0;
  while(1)
  {
    if (two_oper[n]==0) break;
    if (strcasecmp(instr,two_oper[n])==0)
    {
      opcode=((n+4)<<12);
      asm_context->instruction_count++;
      if (asm_two_operand_arithmetic(asm_context, opcode, ALIAS_NONE)==-1) return -1;
      return 0;
    }

    n++;
  }

  // Aliases

  if (strcasecmp(instr, "nop")==0) // ** mov r3, r3 = nop
  {
    opcode=0x4303;
    add_bin(asm_context, opcode, IS_OPCODE);
  }
    else
  if (strcasecmp(instr, "pop")==0) // ** mov @SP+, dst = pop dst 
  {
    if (asm_two_operand_arithmetic(asm_context, 0x4000, ALIAS_SP_PLUS)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "br")==0) // ** mov dst, PC = br dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0x4000, ALIAS_PC)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "ret")==0) // ** mov @SP+, PC = ret
  {
    add_bin(asm_context, 0x4130, IS_OPCODE);
  }
    else
  if (strcasecmp(instr, "clrc")==0) // ** bic #1, SR = clrc
  {
    add_bin(asm_context, 0xc312, IS_OPCODE);
  }
    else
  if (strcasecmp(instr, "setc")==0) // ** bis #1, SR = setc
  {
    add_bin(asm_context, 0xd312, IS_OPCODE);
  }
    else
  if (strcasecmp(instr, "clrz")==0) // ** bic #2, SR = clrz
  {
    add_bin(asm_context, 0xc322, IS_OPCODE);
  }
    else
  if (strcasecmp(instr, "setz")==0) // ** bis #2, SR = setz
  {
    add_bin(asm_context, 0xd322, IS_OPCODE);
  }
    else
  if (strcasecmp(instr, "clrn")==0) // ** bic #4, SR = clrn
  {
    add_bin(asm_context, 0xc222, IS_OPCODE);
  }
    else
  if (strcasecmp(instr, "setn")==0) // ** bis #4, SR = setn
  {
    add_bin(asm_context, 0xd222, IS_OPCODE);
  }
    else
  if (strcasecmp(instr, "dint")==0) // ** bic #8, SR = dint
  {
    add_bin(asm_context, 0xc232, IS_OPCODE);
  }
    else
  if (strcasecmp(instr, "eint")==0) // ** bis #8, SR = eint
  {
    add_bin(asm_context, 0xd232, IS_OPCODE);
  }
    else
  if (strcasecmp(instr, "rla")==0) // ** rla(.b) dst = add(.b) dst,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0x5000, ALIAS_SELF)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "rlc")==0) // ** rlc(.b) dst = addc(.b) dst,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0x6000, ALIAS_SELF)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "inv")==0) // ** inv(.b) dst = xor(.b) #-1,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0xe000, ALIAS_NEG_ONE)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "clr")==0) // ** clr(.b) dst = mov(.b) #0,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0x4000, ALIAS_ZERO)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "tst")==0) // ** tst(.b) dst = cmp(.b) #0,dst
  {
    // FIXME - isn't this better as and dst, dst??
    //if (asm_two_operand_arithmetic(asm_context, 0xf000, ALIAS_SELF)==-1) return -1;
    if (asm_two_operand_arithmetic(asm_context, 0x9000, ALIAS_ZERO)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "dec")==0) // ** dec(.b) dst = sub(.b) #1,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0x8000, ALIAS_ONE)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "decd")==0) // ** dec(.b) dst = sub(.b) #2,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0x8000, ALIAS_TWO)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "inc")==0) // ** inc(.b) dst = add(.b) #1,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0x5000, ALIAS_ONE)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "incd")==0) // ** inc(.b) dst = add(.b) #2,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0x5000, ALIAS_TWO)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "adc")==0) // ** adc(.b) dst = addc(.b) #0,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0x6000, ALIAS_ZERO)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "dadc")==0) // ** adc(.b) dst = dadd(.b) #0,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0xa000, ALIAS_ZERO)==-1) return -1;
  }
    else
  if (strcasecmp(instr, "sbc")==0) // ** sbc(.b) dst = subc(.b) #0,dst
  {
    if (asm_two_operand_arithmetic(asm_context, 0x7000, ALIAS_ZERO)==-1) return -1;
  }
    else
  {
    // Unknown instruction or maybe equ
    return -100;
  }

  asm_context->instruction_count++;

  return 0;
}


