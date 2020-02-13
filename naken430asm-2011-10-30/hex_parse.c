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

#include "hex_parse.h"
#include "naken430util.h"

static int get_hex(FILE *in, int len)
{
int ch;
int n=0;

  while(len>0)
  {
    ch=getc(in);
    if (ch==EOF) return -1;
    if (ch>='0' && ch<='9') ch-='0';
      else
    if (ch>='A' && ch<='F') ch=(ch-'A')+10;
      else
    if (ch>='a' && ch<='f') ch=(ch-'A')+10;
      else
    { return -2; }

    n=(n<<4)+ch;
    len--;
  }

  return n;
}

int hex_parse(char *filename, unsigned char *memory, int memory_len, int *start, int *end, unsigned char *dirty)
{
FILE *in;
int ch;
int byte_count;
int address;
int record_type;
int checksum;
int checksum_calc;
int n;
int start_address=0;
int line=0;

  memset(memory, 0, memory_len);
  memset(dirty, 0, memory_len);

  *start=-1;
  *end=-1;

  in=fopen(filename, "rb");
  if (in==0)
  {
    return -1;
  }

  /* It's a state machine.. it's a state machine... */
  while(1)
  {
    line++;
    ch=getc(in);
    if (ch==EOF) break;

    if (ch!=':')
    {
      /* Line is a junkie piece of shit because ch says so */
      while(1) { if (ch=='\n') break; ch=getc(in); }
      continue;
    }

    byte_count=get_hex(in,2);
    address=get_hex(in,4);
    record_type=get_hex(in,2);
    checksum_calc=byte_count+(address&0xff)+(address>>8)+record_type;

#ifdef DEBUG1
    printf(" byte_count: %02x (%d)\n",byte_count,byte_count);
    printf("    address: %04x (%d)\n",address,address);
    printf("record_type: %02x (%d)\n",record_type,record_type);
#endif

    switch(record_type) 
    {
      /* Data Record */
      case 0x00:
        if (*start==-1)
        {
          *start=address;
          *end=address+byte_count;
        }
          else
        {
          if (address<*start) *start=address;
          if (address+byte_count>*end) *end=address+byte_count;
        }

        for (n=0; n<byte_count; n++)
        {
          ch=get_hex(in,2);
          dirty[address]=1;
          memory[address++]=ch;
          checksum_calc+=ch;
#ifdef DEBUG1
          printf(" %02x",ch);
#endif
        }
        break;

      /* End Of File */
      case 0x01:
        start_address=get_hex(in,byte_count<<1);
        break;

      /* Extended Segment Address Record */
      case 0x02:
        ch=get_hex(in,4);
        start_address=(address<<4)+ch;
        checksum_calc+=(start_address&0xff)+(start_address>>8);
        #ifdef DEBUG1
        printf("Address %d\n",start_address);
        #endif
        break;

      /* Start Segment Address Record */
      case 0x03:

      /* Extended Linear Address Record */
      case 0x04:

      /* Start Linear Address Record */
      case 0x05:

      default:
        for (n=0; n<byte_count; n++)
        {
          ch=get_hex(in,2);
          checksum_calc+=ch;
#ifdef DEBUG1
          printf(" %02x",ch);
#endif
        }
        //printf("Unsupported or unknown code: %d\n",record_type);
        break;
    }

    #ifdef DEBUG1
    printf("\n");
    #endif

    checksum=get_hex(in,2);
    checksum_calc=(((checksum_calc&0xff)^0xff)+1)&0xff;

    #ifdef DEBUG1
    printf("   checksum: %02x [%02x]\n\n",checksum,checksum_calc);
    #endif

    if (checksum!=checksum_calc)
    {
      printf("hex_parse: Checksum failure on line %d!\n", line);
      fclose(in);
      in=NULL;
      start_address=-4;
      break;
    }

    /* All tied up to a state machine */
    while(1)
    {
      ch=getc(in);
      if (ch=='\n' || ch==EOF) break;
    }
  }
  /* We're all slaves to a state machine */

  if (in!=NULL) fclose(in);

  return start_address;
}


