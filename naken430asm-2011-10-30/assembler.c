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
#include "ifdef_expression.h"
#include "lookup_tables.h"
#include "macros.h"
#include "msp430_asm.h"

void print_error(const char *s, struct _asm_context *asm_context)
{
  printf("Error: %s at %s:%d\n", s, asm_context->filename, asm_context->line);
}

void print_error_unexp(const char *s, struct _asm_context *asm_context)
{
  printf("Error: Unexpected token '%s' at %s:%d\n", s, asm_context->filename, asm_context->line);
}

int add_to_include_path(struct _asm_context *asm_context, char *paths)
{
int ptr=0;
int n=0;
char *s;

  s=asm_context->include_path;
  while(!(s[ptr]==0 && s[ptr+1]==0)) { ptr++; }
  if (ptr!=0) ptr++;

  while(paths[n]!=0)
  {
    if (paths[n]==':')
    {
      n++;
      s[ptr++]=0;
    }
      else
    {
      s[ptr++]=paths[n++];
    }

    if (ptr>=INCLUDE_PATH_LEN-1) return -1;
  }

  return 0;
}

static int parse_include(struct _asm_context *asm_context)
{
char token[TOKENLEN];
int token_type;
const char *oldname;
int oldline;
FILE *oldfp;
int ret;

  token_type=get_token(asm_context, token, TOKENLEN);
#ifdef DEBUG
printf("including file %s.\n", token);
#endif

  oldfp=asm_context->in;

  asm_context->in=fopen(token, "rb");

  if (asm_context->in==NULL)
  {
    int ptr=0;
    char *s=asm_context->include_path;
    char filename[1024];

    while(1)
    {
      if (s[ptr]==0) break;

      if (strlen(token)+strlen(s+ptr)<1022)
      {
        sprintf(filename, "%s/%s", s+ptr, token);
        asm_context->in=fopen(filename, "rb");
        if (asm_context->in!=NULL) break;
      }

      while (s[ptr]!=0) ptr++;
      ptr++;
    }
  }

  if (asm_context->in==NULL)
  {
    printf("Cannot open include file '%s' at %s:%d\n", token, asm_context->filename, asm_context->line);
    ret=-1;
  }
    else
  {
    oldname=asm_context->filename;
    oldline=asm_context->line;

    asm_context->filename=token;
    asm_context->line=1;

    ret=assemble(asm_context);

    asm_context->filename=oldname;
    asm_context->line=oldline;
  }

  asm_context->in=oldfp;

  return ret;
}

static int parse_org(struct _asm_context *asm_context)
{
char token[TOKENLEN];
int token_type;

  token_type=get_token(asm_context, token, TOKENLEN);

  if (token_type==TOKEN_NUMBER)
  {
    asm_context->address=atoi(token);

    if (asm_context->instruction_count==0 && asm_context->data_count==0)
    {
      asm_context->low_address=asm_context->address;
      asm_context->high_address=asm_context->address;
    }
  }
    else
  {
    printf("Parse error on line %d. ORG expects an address.\n", asm_context->line);
    return -1;
  }

  return 0;
}

static int parse_name(struct _asm_context *asm_context)
{
char token[TOKENLEN];
int token_type;

  token_type=get_token(asm_context, token, TOKENLEN);

  printf("Program name: %s (ignored)\n", token);

  return 0;
}

static int parse_public(struct _asm_context *asm_context)
{
char token[TOKENLEN];
int token_type;

  token_type=get_token(asm_context, token, TOKENLEN);

  printf("Public symbol: %s (ignored)\n", token);

  return 0;
}

static int parse_db(struct _asm_context *asm_context, int null_term_flag)
{
char token[TOKENLEN];
int token_type;
//int sign=1;
int data32;

  if (asm_context->low_address>asm_context->address)
  {
    asm_context->low_address=asm_context->address;
  }

  while(1)
  {
    token_type=get_token(asm_context, token, TOKENLEN);
    if (token_type==TOKEN_EOL || token_type==TOKEN_EOF) break;

    if (token_type==TOKEN_QUOTED)
    {
      unsigned char *s=(unsigned char *)token;
      while(*s!=0)
      {
        if (*s=='\\')
        {
          int e=escape_char(asm_context, s);
          if (e==0)
          {
            return -1;
          }
          s=s+e;
        }

        asm_context->debug_line[asm_context->address]=-2;
        asm_context->bin[asm_context->address++]=*s;
        if (asm_context->address>=65336)
        {
           printf("Warning: db overran 64k boundary at %s:%d", asm_context->filename, asm_context->line);
           asm_context->address=0; 
           asm_context->high_address=65536;
        }
        asm_context->data_count++;
        s++;
      }

      if (null_term_flag==1)
      {
        asm_context->debug_line[asm_context->address]=-2;
        asm_context->bin[asm_context->address++]=0;
        asm_context->data_count++;
      }
    }
      else
    {
#if 0
      if (token_type!=TOKEN_NUMBER)
      {
        if (IS_TOKEN(token,'-'))
        {
          sign=sign*-1;
          continue;
        }
          else
        {
          printf("Parse error: db data must be numbers on line %d.\n", asm_context->line);
          return -1;
        }
      }
#endif
      pushback(asm_context, token, token_type);
      eval_expression(asm_context, &data32);

      asm_context->debug_line[asm_context->address]=-2;
      //asm_context->bin[asm_context->address++]=(unsigned char)(atoi(token)*sign);
      asm_context->bin[asm_context->address++]=(unsigned char )data32;
      if (asm_context->address>=asm_context->memory_size)
      {
        asm_context->address=0; asm_context->high_address=asm_context->memory_size-1;
      }
      asm_context->data_count++;
    }

    token_type=get_token(asm_context, token, TOKENLEN);
    if (token_type==TOKEN_EOL || token_type==TOKEN_EOF) break;

    if (IS_NOT_TOKEN(token,','))
    {
      printf("Parse error: expecting a ',' on line %d.\n", asm_context->line);
      return -1;
    }

    //sign=1;
  }

  asm_context->line++;

  if (asm_context->high_address<asm_context->address-1)
  { asm_context->high_address=asm_context->address-1; }

  return 0;
}

static int parse_dw(struct _asm_context *asm_context)
{
char token[TOKENLEN];
int token_type;
int sign=1;
int data32;
unsigned short int data16;

  if (asm_context->low_address>asm_context->address)
  {
    asm_context->low_address=asm_context->address;
  }

  if ((asm_context->address&0x01)!=0)
  {
    if (asm_context->pass==2)
    {
      printf("Warning: dw doesn't start on 16 bit boundary at %s:%d.  Padding with a 0.\n", asm_context->filename, asm_context->line);
    }
    asm_context->debug_line[asm_context->address]=-2;
    asm_context->bin[asm_context->address++]=0;
    if (asm_context->address>=asm_context->memory_size)
    { asm_context->address=0; asm_context->high_address=asm_context->memory_size-1; }
    asm_context->data_count++;
  }

  while(1)
  {
    eval_expression(asm_context, &data32);
    data16=(unsigned short)data32;
#if 0
    token_type=get_token(asm_context, token, TOKENLEN);
    if (token_type==TOKEN_EOL || token_type==TOKEN_EOF) break;

    if (token_type!=TOKEN_NUMBER)
    {
      if (IS_TOKEN(token,'-'))
      {
        sign=sign*-1;
        continue;
      }
        else
      {
        printf("Parse error: db data must be numbers on line %d.\n", asm_context->line);
        return -1;
      }
    }

    data16=(unsigned short int)(atoi(token)*sign);
#endif
    asm_context->debug_line[asm_context->address]=-2;
    asm_context->debug_line[asm_context->address+1]=-2;
    asm_context->bin[asm_context->address++]=data16&255;
    asm_context->bin[asm_context->address++]=data16>>8;
    asm_context->data_count+=2;
    token_type=get_token(asm_context, token, TOKENLEN);
    if (token_type==TOKEN_EOL || token_type==TOKEN_EOF) break;

    if (IS_NOT_TOKEN(token,','))
    {
      printf("Parse error: expecting a ',' on line %d.\n", asm_context->line);
      return -1;
    }

    sign=1;
  }

  asm_context->line++;

  if (asm_context->high_address<asm_context->address-1)
  { asm_context->high_address=asm_context->address-1; }

  return 0;
}

static int parse_ds(struct _asm_context *asm_context, int n)
{
char token[TOKENLEN];
int token_type;
int num;

  token_type=get_token(asm_context, token, TOKENLEN);
  if (token_type!=TOKEN_NUMBER)
  {
    printf("Parse error: memory length on line %d.\n", asm_context->line);
    return -1;
  }

  num=atoi(token)*n;
#if 0
  if ((num&1)==1)
  {
    printf("Warning: databytes not aligned to 2 bytes at %s:%d.  Adding 1 byte.\n", asm_context->filename, asm_context->line);
    num++;
  }
#endif

  for (n=0; n<num; n++)
  {
    asm_context->debug_line[asm_context->address]=-2;
    asm_context->bin[asm_context->address++]=0;
    if (asm_context->address>=65336)
    {
       printf("Error: ds overran 64k boundary at %s:%d", asm_context->filename, asm_context->line);
       return -1;
    }
  }

  asm_context->data_count+=num;
  if (asm_context->high_address<asm_context->address-1)
  { asm_context->high_address=asm_context->address-1; }

  return 0;
}

static int ifdef_ignore(struct _asm_context *asm_context)
{
char token[TOKENLEN];
int token_type;

  while(1)
  {
    token_type=get_token(asm_context, token, TOKENLEN);

    if (token_type==TOKEN_EOF)
    {
      print_error("Missing endif", asm_context);
      return -1;
    }

    if (token_type==TOKEN_EOL)
    {
      asm_context->line++;
    }
      else
    if (token_type==TOKEN_POUND || IS_TOKEN(token,'.'))
    {
      token_type=get_token(asm_context, token, TOKENLEN);
      if (strcasecmp(token, "endif")==0) return 0;
        else
      if (strcasecmp(token, "else")==0) return 2;
    }
  }
}

static int parse_ifdef_ignore(struct _asm_context *asm_context, int ignore_section)
{
//char token[TOKENLEN];
//int token_type;
//int ignore_section=0;
//int param_count;  // throw away
//int ret;

  //asm_context->parsing_ifdef=1;
  //ret=parse_ifdef_expression(asm_context);
  //token_type=get_token(asm_context, token, TOKENLEN);
  //asm_context->parsing_ifdef=0;

  if (ignore_section==1)
  {
    if (ifdef_ignore(asm_context)==2)
    {
      assemble(asm_context);
    }
  }
    else
  {
    if (assemble(asm_context)==2)
    {
      ifdef_ignore(asm_context);
    }
  }

  return 0;
}

static int parse_ifdef(struct _asm_context *asm_context, int ifndef)
{
char token[TOKENLEN];
int token_type;
int ignore_section=0;
int param_count; // throw away

  asm_context->ifdef_count++;

  asm_context->parsing_ifdef=1;
  token_type=get_token(asm_context, token, TOKENLEN);
  asm_context->parsing_ifdef=0;

  if (token_type!=TOKEN_STRING)
  {
    print_error("#ifdef has no label", asm_context);
    return -1;
  }

  if (defines_heap_lookup(&asm_context->defines_heap, token, &param_count)!=NULL)
  {
    if (ifndef==1) ignore_section=1;
  }
    else
  {
    if (ifndef==0) ignore_section=1;
  }

  parse_ifdef_ignore(asm_context, ignore_section);

  asm_context->ifdef_count--;

  return 0;
}

static int parse_if(struct _asm_context *asm_context)
{
int num;

  asm_context->ifdef_count++;

  asm_context->parsing_ifdef=1;
  num=eval_ifdef_expression(asm_context);
  asm_context->parsing_ifdef=0;

  if (num==-1) return -1;

  if (num!=0)
  {
    parse_ifdef_ignore(asm_context, 0);
  }
    else
  {
    parse_ifdef_ignore(asm_context, 1);
  }

  asm_context->ifdef_count--;

  return 0;
}

void assemble_init(struct _asm_context *asm_context)
{
  fseek(asm_context->in, 0, SEEK_SET);
  asm_context->address=0;
  asm_context->line=1;
  asm_context->instruction_count=0;
  asm_context->code_count=0;
  asm_context->data_count=0;
  asm_context->ifdef_count=0;
  asm_context->parsing_ifdef=0;
  asm_context->pushback[0]=0;
  asm_context->unget[0]=0;
  asm_context->unget_ptr=0;
  asm_context->unget_stack_ptr=0;
  asm_context->unget_stack[0]=0;

  // This could change when we switch to 20 bit extensions
  asm_context->memory_size=65536;

  asm_context->defines_heap.ptr=0;
  asm_context->defines_heap.stack_ptr=0;
  asm_context->def_param_stack_count=0;
  if (asm_context->pass==1)
  {
    memset(asm_context->debug_line, 0xff, sizeof(int)*asm_context->memory_size);
  }
}

void assemble_print_info(struct _asm_context *asm_context, FILE *out)
{
  fprintf(out, "\nProgram Info:\n");
#ifdef DEBUG
  address_heap_print(&asm_context->address_heap);
  defines_heap_print(&asm_context->defines_heap);
#endif

  fprintf(out, "Include Paths: .\n");
  int ptr=0;
  if (asm_context->include_path[ptr]!=0)
  {
    fprintf(out, "               ");
    while(1)
    {
      if (asm_context->include_path[ptr]==0 &&
          asm_context->include_path[ptr+1]==0)
      { fprintf(out, "\n"); break; }

      if (asm_context->include_path[ptr]==0)
      {
        fprintf(out, "\n               ");
        ptr++;
        continue;
      }
      putc(asm_context->include_path[ptr++], out); 
    }
  }

  fprintf(out, " Instructions: %d\n", asm_context->instruction_count);
  fprintf(out, "   Code Bytes: %d\n", asm_context->code_count);
  fprintf(out, "   Data Bytes: %d\n", asm_context->data_count);
  fprintf(out, "  Low Address: %04x (%d)\n", asm_context->low_address, asm_context->low_address);
  fprintf(out, " High Address: %04x (%d)\n", asm_context->high_address, asm_context->high_address);
  fprintf(out, "\n");
}

int check_for_directive(struct _asm_context *asm_context, char *token)
{
  if (strcasecmp(token, "org")==0)
  {
    if (parse_org(asm_context)!=0) return -1;
    return 1;
  }
    else
  if (strcasecmp(token, "name")==0)
  {
    if (parse_name(asm_context)!=0) return -1;
    return 1;
  }
    else
  if (strcasecmp(token, "public")==0)
  {
    if (parse_public(asm_context)!=0) return -1;
    return 1;
  }
    else
  if (strcasecmp(token, "db")==0 || strcasecmp(token, "dc8")==0 || strcasecmp(token, "ascii")==0)
  {
    if (parse_db(asm_context, 0)!=0) return -1;
    return 1;
  }
    else
  if (strcasecmp(token, "asciiz")==0)
  {
    if (parse_db(asm_context, 1)!=0) return -1;
    return 1;
  }
    else
  if (strcasecmp(token, "dw")==0 || strcasecmp(token, "dc16")==0)
  {
    if (parse_dw(asm_context)!=0) return -1;
    return 1;
  }
    else
  if (strcasecmp(token, "ds")==0 || strcasecmp(token, "ds8")==0)
  {
    if (parse_ds(asm_context,1)!=0) return -1;
    return 1;
  }
    else
  if (strcasecmp(token, "ds16")==0)
  {
    if (parse_ds(asm_context,2)!=0) return -1;
    return 1;
  }
    else
  if (strcasecmp(token, "end")==0)
  {
    return 2;
  }

  return 0;
}

int assemble(struct _asm_context *asm_context)
{
char token[TOKENLEN];
int token_type;

  while(1)
  {
    token_type=get_token(asm_context, token, TOKENLEN);
#ifdef DEBUG
    printf("%d: <%d> %s\n", asm_context->line, token_type, token);
#endif
    if (token_type==TOKEN_EOF) break;

    if (token_type==TOKEN_EOL)
    {
      if (asm_context->defines_heap.stack_ptr==0) { asm_context->line++; }
    }
      else
    if (token_type==TOKEN_LABEL)
    {
      if (address_heap_append(asm_context, token, asm_context->address)==-1)
      { return -1; }
    }
      else
    if (token_type==TOKEN_POUND || IS_TOKEN(token,'.'))
    {
      token_type=get_token(asm_context, token, TOKENLEN);
#ifdef DEBUG
    printf("%d: <%d> %s\n", asm_context->line, token_type, token);
#endif
      if (token_type==TOKEN_EOF) break;

      if (strcasecmp(token, "define")==0)
      {
        if (parse_macro(asm_context, IS_DEFINE)!=0) return -1;
      }
        else
      if (strcasecmp(token, "ifdef")==0)
      {
        parse_ifdef(asm_context, 0);
      }
        else
      if (strcasecmp(token, "ifndef")==0)
      {
        parse_ifdef(asm_context, 1);
      }
        else
      if (strcasecmp(token, "if")==0)
      {
        parse_if(asm_context);
      }
        else
      if (strcasecmp(token, "endif")==0)
      {
        if (asm_context->ifdef_count<1)
        {
          printf("Error: unmatched #endif at %s:%d\n", asm_context->filename, asm_context->ifdef_count);
          return -1;
        }
        return 0;
      }
        else
      if (strcasecmp(token, "else")==0)
      {
        if (asm_context->ifdef_count<1)
        {
          printf("Error: unmatched #else at %s:%d\n", asm_context->filename, asm_context->ifdef_count);
          return -1;
        }
        return 2;
      }
        else
      if (strcasecmp(token, "include")==0)
      {
        if (parse_include(asm_context)!=0) return -1;
      }
        else
      if (strcasecmp(token, "macro")==0)
      {
        if (parse_macro(asm_context, IS_MACRO)!=0) return -1;
      }
        else
      {
        int ret=check_for_directive(asm_context, token);
        if (ret==2) break;
        if (ret==-1) return -1;
        if (ret!=1)
        {
          printf("Error: Unknown directive '%s' at %s:%d.\n", token, asm_context->filename, asm_context->line);
          return -1;
        }
      }
    }
      else
    if (token_type==TOKEN_STRING)
    {
      int ret=check_for_directive(asm_context, token);
      if (ret==2) break;
      if (ret==-1) return -1;
      if (ret!=1) 
      {
        int start_address=asm_context->address;
        ret=parse_instruction(asm_context, token);
        if (ret==-100)
        {
          char token2[TOKENLEN];
          int token_type2;

          token_type2=get_token(asm_context, token2, TOKENLEN);
          //if (strcasecmp(token2, "equ")!=0 && strcasecmp(token2, ".equ")!=0)
          if (strcasecmp(token2, "equ")!=0)
          {
            printf("Error: Unknown instruction '%s' at %s:%d.\n", token, asm_context->filename, asm_context->line);
            return -1;
          }

          //token_type2=get_token(asm_context, token2, TOKENLEN);
          int ptr=0;
          int ch='\n';
          while(1)
          {
            ch=get_next_char(asm_context);
            if (ch==EOF || ch=='\n') break;
            if (ch=='*' && ptr>0 && token2[ptr-1]=='/')
            {
              eatout_star_comment(asm_context);
              ptr--;
              continue;
            }

            token2[ptr++]=ch;
            if (ptr==TOKENLEN-1)
            {
              printf("Internal Error: token overflow at %s:%d.\n", __FILE__, __LINE__);
              return -1;
            }
          }
          token2[ptr]=0;
          unget_next_char(asm_context, ch);
          strip_macro(token2);
          defines_heap_append(asm_context, token, token2, 0);
        }
          else
        if (ret!=0)
        {
          return -1;
        }
          else
        {
          if (asm_context->address>start_address)
          {
            asm_context->code_count+=(asm_context->address-start_address);
          }
        }
      }
    }
      else
    {
      print_error_unexp(token, asm_context);
      return -1;
    }
  }

  return 0;
}

