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

#ifndef _ASSEMBLER_H
#define _ASSEMBLER_H

#define MAX_NESTED_MACROS 128
#define TOKENLEN 512
#define MAX_MACRO_LEN 1024
#define PARAM_STACK_LEN 4096
#define INCLUDE_PATH_LEN 1024

/*
  address_heap buffer looks like this:
  sturuct
  {
    char name[];
    int address;
  };

*/

struct _address_heap
{
  unsigned char *buffer;
  int len;
  int ptr;
  int locked;
};

/*
  defines_heap buffer looks like this:
  struct
  {
    char name[];
    unsigned char value[];  // params are binary 0x01 to 0x09
    int param_count;
  };
*/

struct _defines_heap
{
  unsigned char *buffer;
  int len;
  int ptr;
  int locked;
  char *stack[MAX_NESTED_MACROS];
  int stack_ptr;
};

struct _asm_context
{
  FILE *in;
  const char *filename;
  int address;
  int line;
  int pass;
  struct _address_heap address_heap;
  struct _defines_heap defines_heap;
  int instruction_count;
  int data_count;
  int code_count;
  int low_address;
  int high_address;
  int ifdef_count;
  int parsing_ifdef;
  char pushback[TOKENLEN];
  int pushback_type;
  char unget[512];
  int unget_ptr;
  int unget_stack[MAX_NESTED_MACROS+1];
  int unget_stack_ptr;
  int debug_file;
  char def_param_stack_data[PARAM_STACK_LEN];
  int def_param_stack_ptr[MAX_NESTED_MACROS+1];
  int def_param_stack_count;
  char include_path[INCLUDE_PATH_LEN];
  int memory_size;
  unsigned char bin[65536];
  int debug_line[65536];
};

void print_error(const char *s, struct _asm_context *asm_context);
void print_error_unexp(const char *s, struct _asm_context *asm_context);
int add_to_include_path(struct _asm_context *asm_context, char *paths);
void assemble_init(struct _asm_context *asm_context);
void assemble_print_info(struct _asm_context *asm_context, FILE *out);
int assemble(struct _asm_context *asm_context);

#endif

