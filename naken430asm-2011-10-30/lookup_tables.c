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
#include "lookup_tables.h"
#include "macros.h"

int address_heap_init(struct _address_heap *address_heap)
{
  address_heap->buffer=(unsigned char *)malloc(ADDRESS_HEAP_SIZE);
  address_heap->len=ADDRESS_HEAP_SIZE;
  address_heap->ptr=0;
  address_heap->locked=0;

  return 0;
}

void address_heap_free(struct _address_heap *address_heap)
{
  free(address_heap->buffer);
}

int address_heap_append(struct _asm_context *asm_context, char *name, int address)
{
int token_len;
struct _address_heap *address_heap=&asm_context->address_heap;

  if (address_heap->locked==1) return 0;

  int param_count_temp;
  if (address_heap_lookup(address_heap, name)!=-1 || defines_heap_lookup(&asm_context->defines_heap, name, &param_count_temp)!=NULL)
  {
    printf("Error: Label '%s' already defined.\n", name);
    return -1;
  }

  token_len=strlen(name)+1;

  if (address_heap->ptr+token_len+sizeof(int)>=address_heap->len)
  {
    if (token_len<65536)
    { address_heap->len+=65536; }
      else
    { address_heap->len+=65536+token_len; }

    address_heap->buffer=(unsigned char *)realloc(address_heap->buffer, address_heap->len);
#ifdef DEBUG
    printf("Uh oh: address_heap exhaused.. growing to %d\n",address_heap->len);
#endif
    //printf("Internal error: address_heap exhaused.\n");
    //exit(1);
    //return -1;
  }

  memcpy(address_heap->buffer+address_heap->ptr, name, token_len);
  memcpy(address_heap->buffer+address_heap->ptr+token_len, &address, sizeof(int));
  address_heap->ptr+=token_len+sizeof(int);

  return 0;
} 

void address_heap_lock(struct _address_heap *address_heap)
{
  address_heap->locked=1;
}

int address_heap_lookup(struct _address_heap *address_heap, char *name)
{
int token_len;
int address;
int ptr=0;

  while(ptr<address_heap->ptr)
  {
    token_len=strlen((char *)address_heap->buffer+ptr)+1;
    if (strcmp((char *)address_heap->buffer+ptr, name)==0)
    {
      memcpy(&address, address_heap->buffer+ptr+token_len, sizeof(int));
      return address;
    }
    ptr=ptr+token_len+sizeof(int);
  }

  return -1;
}

int address_heap_print(struct _address_heap *address_heap)
{
int token_len;
int address;
int ptr=0;
int count=0;

  printf("address_heap.len=%d  address_heap.ptr=%d\n\n", address_heap->len, address_heap->ptr);

  printf("%30s ADDRESS\n", "LABEL");

  while(ptr<address_heap->ptr)
  {
    token_len=strlen((char *)address_heap->buffer+ptr)+1;
    memcpy(&address, address_heap->buffer+ptr+token_len, sizeof(int));
    printf("%30s %08x (%d)\n", address_heap->buffer+ptr, address, address);
    ptr=ptr+token_len+sizeof(int);
    count++;
  }

  printf("Total %d.\n\n", count);

  return 0;
}

