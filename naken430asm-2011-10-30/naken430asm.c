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
#include <unistd.h>

#include "assembler.h"
#include "file_output.h"
#include "lookup_tables.h"
#include "macros.h"
#include "msp430_disasm.h"
#include "version.h"

#define FORMAT_HEX 0
#define FORMAT_BIN 1
#define FORMAT_ELF 2

static void new_extension(char *filename, char *ext, int len)
{
int i;

  i=strlen(filename)-1;
  if (i+2>len)
  {
    printf("Internal error: filename too long %s:%d\n", __FILE__, __LINE__);
    exit(1);
  }

  while(i>0)
  {
    if (filename[i]=='.')
    {
      strcpy(filename+i+1,ext);
      break;
    }

    i--;
  }

  if (i==0)
  {
    strcat(filename,ext);
  }

}

int main(int argc, char *argv[])
{
FILE *out;
FILE *dbg=NULL;
FILE *list=NULL;
int i;
int format=FORMAT_HEX;
int create_list=0;
char *infile=NULL,*outfile=NULL;
struct _asm_context asm_context;
int error_flag=0;

  printf("\nnaken430asm - by Michael Kohn\n");
  printf("    Web: http://www.mikekohn.net/\n");
  printf("  Email: mike@mikekohn.net\n\n");
  printf("Version: "VERSION"\n\n");

  if (argc<2)
  {
    printf("Usage: naken430asm [options] <infile>\n");
    printf("   -o <outfile>\n");
    printf("   -h             [output hex file]\n");
#ifndef DISABLE_ELF
    printf("   -e             [output elf file]\n");
#endif
    printf("   -b             [output binary file]\n");
    printf("   -d             [create .ndbg debug file]\n");
    printf("   -l             [create .lst listing file]\n");
    printf("   -I             [add to include path]\n");
    printf("\n");
    exit(0);
  }

  memset(&asm_context, 0, sizeof(asm_context));

  for (i=1; i<argc; i++)
  {
    if (strcmp(argv[i], "-o")==0)
    {
      outfile=argv[++i];
    }
      else
    if (strcmp(argv[i], "-h")==0)
    {
      format=FORMAT_HEX;
    }
      else
    if (strcmp(argv[i], "-b")==0)
    {
      format=FORMAT_BIN;
    }
#ifndef DISABLE_ELF
      else
    if (strcmp(argv[i], "-e")==0)
    {
      format=FORMAT_ELF;
    }
#endif
      else
    if (strcmp(argv[i], "-d")==0)
    {
      asm_context.debug_file=1;
    }
      else
    if (strcmp(argv[i], "-l")==0)
    {
      create_list=1;
    }
      else
    if (strncmp(argv[i], "-I", 2)==0)
    {
      char *s=argv[i];
      if (s[2]==0)
      {
        if (add_to_include_path(&asm_context, argv[++i])!=0)
        {
          printf("Internal Error:  Too many include paths\n");
          exit(1);
        }
      }
        else
      {
        if (add_to_include_path(&asm_context, s+2)!=0)
        {
          printf("Internal Error:  Too many include paths\n");
          exit(1);
        }
      }
    }
      else
    {
      infile=argv[i];
    }
  }

  if (infile==NULL)
  {
    printf("No input file specified.\n");
    exit(1);
  }

  if (outfile==NULL)
  {
    if (format==FORMAT_HEX)
    {
      outfile="out.hex";
    }
      else
    if (format==FORMAT_BIN)
    {
      outfile="out.bin";
    }
#ifndef DISABLE_ELF
      else
    if (format==FORMAT_ELF)
    {
      outfile="out.elf";
    }
#endif
  }

#ifdef INCLUDE_PATH
  if (add_to_include_path(&asm_context, INCLUDE_PATH)!=0)
  {
    printf("Internal Error:  Too many include paths\n");
    exit(1);
  }
#endif

  asm_context.in=fopen(infile,"rb");
  if (asm_context.in==NULL)
  {
    printf("Couldn't open %s for reading.\n",infile);
    exit(1);
  }

  asm_context.filename=infile;

  out=fopen(outfile,"wb");
  if (out==NULL)
  {
    printf("Couldn't open %s for writing.\n",outfile);
    exit(1);
  }

  printf(" Input file: %s\n",infile);
  printf("Output file: %s\n",outfile);

  if (asm_context.debug_file==1)
  {
    char filename[1024];
    strcpy(filename,outfile);

    new_extension(filename, "ndbg", 1024);

    dbg=fopen(filename,"wb");
    if (dbg==NULL)
    {
      printf("Couldn't open %s for writing.\n",filename);
      exit(1);
    }

    printf(" Debug file: %s\n",filename);

    fprintf(dbg,"%s\n",infile);
  }

  if (create_list==1)
  {
    char filename[1024];
    strcpy(filename,outfile);

    new_extension(filename, "lst", 1024);

    list=fopen(filename,"wb");
    if (list==NULL)
    {
      printf("Couldn't open %s for writing.\n",filename);
      exit(1);
    }

    printf("  List file: %s\n",filename);
  }

  printf("\n");

  address_heap_init(&asm_context.address_heap);
  defines_heap_init(&asm_context.defines_heap);

  printf("Pass 1...\n");
  asm_context.pass=1;
  assemble_init(&asm_context);
  error_flag=assemble(&asm_context);
  if (error_flag!=0)
  {
    printf("** Errors... bailing out\n");
    fclose(out);
    unlink(outfile);
    create_list=0;
  }
    else
  {
    address_heap_lock(&asm_context.address_heap);
    // defines_heap_lock(&asm_context.defines_heap);

    printf("Pass 2...\n");
    asm_context.pass=2;
    assemble_init(&asm_context);
    error_flag=assemble(&asm_context);

    if (format==FORMAT_HEX)
    {
      write_hex(&asm_context, out);
    }
      else
    if (format==FORMAT_BIN)
    {
      write_bin(&asm_context, out);
    }
#ifndef DISABLE_ELF
      else
    if (format==FORMAT_ELF)
    {
      write_elf(&asm_context, out);
    }
#endif

    if (dbg!=NULL)
    {
      for (i=0; i<65536; i++)
      {
        putc(asm_context.debug_line[i]>>8,dbg);
        putc(asm_context.debug_line[i]&0xff,dbg);
      }

      fclose(dbg);
    }

    fclose(out);
  }

#if 0
  for (i=asm_context.low_address; i<=asm_context.high_address; i++)
  {
    printf("%04x: %d\n", i, asm_context.debug_line[i]);
  }
#endif

  if (create_list==1)
  {
    int *lines=NULL;
    int line=1;
    int ch;
    fseek(asm_context.in, 0, SEEK_SET);

    /* Build a map of lines to offset in source file (really more
       of a map of lines to offset in debug_line which is an offset
       to input file */

    if (asm_context.line<65535)
    {
      lines=(int *)malloc((asm_context.line+1)*sizeof(int));
      memset(lines, -1, (asm_context.line+1)*sizeof(int));

      for (i=asm_context.high_address; i>=asm_context.low_address; i--)
      {
        if ((int)asm_context.debug_line[i]<0) continue;
        lines[asm_context.debug_line[i]]=i;
      }
    }

    /* Read input file again line by line and write back out to lst file.
       If line matches something in the index, then output the line and
       disassembled code */

    while(1)
    {
      ch=getc(asm_context.in);
      if (ch!=EOF) { putc(ch, list); }

      if (ch=='\n' || ch==EOF)
      {
        int i;
        int start;

        if (lines==NULL)
        { start=asm_context.low_address; }
          else
        {
          if (lines[line]>=0)
          {
            start=lines[line];
          }
            else
          {
            start=asm_context.high_address+1;
          }
        }

        //int loop_count=0;
        for (i=start; i<=asm_context.high_address; i++)
        {
          int num;

          if (asm_context.debug_line[i]==line)
          {
            fprintf(list, "\n");
            while(asm_context.debug_line[i]==line)
            {
              int cycles;
              char instruction[128];
              int count=msp430_disasm(asm_context.bin, i, instruction, &cycles);
              num=asm_context.bin[i]|(asm_context.bin[i+1]<<8);
              fprintf(list, "0x%04x: 0x%04x %-40s cycles: %d\n", i, num, instruction, cycles);
              count--;
              while (count>0)
              {
                i=i+2;
                num=asm_context.bin[i]|(asm_context.bin[i+1]<<8);
                fprintf(list, "0x%04x: 0x%04x\n", i, num);
                count--;
              }

              i=i+2;
            }

            fprintf(list, "\n");
            break;
          }
          //loop_count++;
        }

        //printf("loop_count=%d\n", loop_count);
        if (ch==EOF) { break; }
        line++;
      }
    }

    if (lines!=NULL) free(lines);

    assemble_print_info(&asm_context, list);

    fclose(list);
  }

  assemble_print_info(&asm_context, stdout);

  address_heap_free(&asm_context.address_heap);
  defines_heap_free(&asm_context.defines_heap);

  fclose(asm_context.in);

  if (error_flag!=0)
  {
    printf("*** Failed ***\n\n");
    unlink(outfile);
  }

  return 0;
}


