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
#include "file_output.h"

static void write_hex_line(FILE *out, int address, unsigned char *data, int len)
{
int checksum;
int n;

  fprintf(out, ":%02X%04X00", len, address);
  checksum=len+(address>>8)+(address&255);

  for (n=0; n<len; n++)
  {
    fprintf(out, "%02X", data[n]);
    checksum=checksum+data[n];
  }

  fprintf(out,"%02X\n", (((checksum&0xff)^0xff)+1)&0xff);
}

int write_hex(struct _asm_context *asm_context, FILE *out)
{
unsigned char data[16];
int len;
int n;
int address=0;

  len=-1;
  for (n=asm_context->low_address; n<=asm_context->high_address; n++)
  {
    if (((int)asm_context->debug_line[n])==-1)
    {
      if (len>0)
      {
        write_hex_line(out, address, data, len);
        len=-1;
      }

      continue;
    }

    if (len==-1)
    {
      address=n;
      len=0;
    }

    data[len++]=asm_context->bin[n];

    if (len==16)
    {
      write_hex_line(out, address, data, len);
      len=-1;
    }
  }

  if (len>0)
  {
    write_hex_line(out, address, data, len);
  }

  fputs(":00000001FF\n", out);

  return 0;
}

int write_bin(struct _asm_context *asm_context, FILE *out)
{
int n;

  for (n=0; n<65536; n++)
  {
    putc(asm_context->bin[n], out);
  }

  return 0;
}

#ifndef DISABLE_ELF
static void write_int32_l(FILE *out, unsigned int n)
{
  putc(n&0xff, out);
  putc((n>>8)&0xff, out);
  putc((n>>16)&0xff, out);
  putc((n>>24)&0xff, out);
}

static void write_int16_l(FILE *out, unsigned int n)
{
  putc(n&0xff, out);
  putc((n>>8)&0xff, out);
}

static void write_shdr(FILE *out, struct _shdr *shdr)
{
  write_int32_l(out, shdr->sh_name);
  write_int32_l(out, shdr->sh_type);
  write_int32_l(out, shdr->sh_flags);
  write_int32_l(out, shdr->sh_addr);
  write_int32_l(out, shdr->sh_offset);
  write_int32_l(out, shdr->sh_size);
  write_int32_l(out, shdr->sh_link);
  write_int32_l(out, shdr->sh_info);
  write_int32_l(out, shdr->sh_addralign);
  write_int32_l(out, shdr->sh_entsize);
}

static void write_symtab(FILE *out, struct _symtab *symtab)
{
  write_int32_l(out, symtab->st_name);
  write_int32_l(out, symtab->st_value);
  write_int32_l(out, symtab->st_size);
  putc(symtab->st_info, out);
  putc(symtab->st_other, out);
  write_int16_l(out, symtab->st_shndx);
}

static int find_section(char *sections, char *name, int len)
{
int n=0;

  while(n<len)
  {
    if (sections[n]=='.')
    {
      if (strcmp(sections+n, name)==0) return n;
      n+=strlen(sections+n);
    }

    n++;
  }

  return -1;
}

static void elf_addr_align(FILE *out)
{
  long marker=ftell(out);
  while((marker%4)!=0) { putc(0x00, out); marker++; }
}

static int get_string_table_len(char *string_table)
{
int n=0;

  while(string_table[n]!=0 || string_table[n+1]!=0) { n++; }

  return n+1;
}

static void string_table_append(char *string_table, char *name)
{
int len;

  len=get_string_table_len(string_table);
  string_table=string_table+len;
  strcpy(string_table, name);
  string_table=string_table+strlen(name)+1;
  *string_table=0;
}

int write_elf(struct _asm_context *asm_context, FILE *out)
{
unsigned char e_ident[16] = { 0x7f, 'E','L','F',  1, 1, 1, 0xff,
                              0, 0, 0, 0,  0, 0, 0, 0 };
struct _sections_offset sections_offset;
struct _sections_size sections_size;
struct _shdr shdr;
struct _symtab symtab;
int i;
int e_flags;
int e_shnum;
int e_shstrndx=0;
int text_addr[ELF_TEXT_MAX];
int data_addr[ELF_TEXT_MAX];
int text_count=0;
int data_count=0;

  memset(&sections_offset, 0, sizeof(sections_offset));
  memset(&sections_size, 0, sizeof(sections_size));

  // Fill in blank to minimize text and data sections
  i=0;
  while (i<=asm_context->high_address)
  {
    if (asm_context->debug_line[i]==-2)
    {
      // Fill gaps in data sections
      while(asm_context->debug_line[i]==-2)
      {
        //printf("Found data %02x\n", i);
        while(i<=asm_context->high_address && asm_context->debug_line[i]==-2)
        { i+=2; }
        //printf("End data %02x\n", i);
        if (i==asm_context->high_address) break;
        int marker=i;
        while(i<=asm_context->high_address && asm_context->debug_line[i]==-1)
        { i+=2; }
        if (i==asm_context->high_address) break;

        if (i<=asm_context->high_address && asm_context->debug_line[i]==-2)
        {
          while(marker!=i) { asm_context->debug_line[marker++]=-2; }
        }
        if (i==asm_context->high_address) break;
      }
    }
      else
    if (asm_context->debug_line[i]>=0)
    {
      // Fill gaps in code sections
      while(i<=asm_context->high_address && asm_context->debug_line[i]>=0)
      {
        //printf("Found code %02x %d %d\n", i, asm_context->debug_line[i], asm_context->bin[i]);
        while(i<=asm_context->high_address && (asm_context->debug_line[i]>=0 || asm_context->debug_line[i]==-3))
        { i+=2; }
        if (i==asm_context->high_address) break;
        //printf("End code %02x  %d\n", i, asm_context->debug_line[i]);
        int marker=i;
        while(i<=asm_context->high_address && asm_context->debug_line[i]==-1)
        { i+=2; }
        if (i==asm_context->high_address) break;

        if (asm_context->debug_line[i]>=0)
        {
          while(marker!=i) { asm_context->debug_line[marker++]=-3; }
        }
        if (i==asm_context->high_address) break;
      }
    }

    i+=2;
  }

  // For now we will only have .text, .shstrtab, .symtab, strtab
  // I can't see a use for .data and .bss unless the bootloaders know elf?
  // I'm also not supporting relocations.  I can do it later if requested.

  // Write string table
  char string_table[1024] = {
    "\0"
    //".text\0"
    //".rela.text\0"
    //".data\0"
    //".bss\0"
    ".shstrtab\0"
    ".symtab\0"
    ".strtab\0"
    ".comment\0"
    //".debug_line\0"
    //".rela.debug_line\0"
    //".debug_info\0"
    //".rela.debug_info\0"
    //".debug_abbrev\0"
    //".debug_aranges\0"
    //".rela.debug_aranges\0"
  };

  e_flags=11;  // mspgcc4/build/insight-6.8-1/include/elf/msp430.h

  e_shnum=4;
  //if (asm_context->debug_file==1) { e_shnum+=4; }
  e_shnum++; // Null section to start...

  // Write Ehdr;
  i=fwrite(e_ident, 1, 16, out);
  write_int16_l(out, 0);       // e_type 0=not relocatable 1=msp_32
  write_int16_l(out, 0x69);    // e_machine EM_MSP430=0x69
  write_int32_l(out, 1);       // e_version
  write_int32_l(out, 0);       // e_entry (this could be 16 bit at 0xfffe)
  write_int32_l(out, 0);       // e_phoff (program header offset)
  write_int32_l(out, 0);       // e_shoff (section header offset)
  write_int32_l(out, e_flags); // e_flags (should be set to CPU model)
  write_int16_l(out, 0x34);    // e_ehsize (size of this struct)
  write_int16_l(out, 0);       // e_phentsize (program header size)
  write_int16_l(out, 0);       // e_phnum (number of program headers)
  write_int16_l(out, 40);      // e_shentsize (section header size)
  write_int16_l(out, e_shnum); // e_shnum (number of section headers)
  write_int16_l(out, 2);       // e_shstrndx (section header string table index)

  // .text and .data sections
  i=0;
  while (i<=asm_context->high_address)
  {
    char name[32];

    if (asm_context->debug_line[i]==-2)
    {
      if (data_count>=ELF_TEXT_MAX) { printf("Too many elf .data sections (count=%d).  Internal error.\n", data_count); exit(1); }
      if (data_count==0) { strcpy(name, ".data"); }
      else { sprintf(name, ".data%d", data_count); }
      data_addr[data_count]=i;
      string_table_append(string_table, name);
      sections_offset.data[data_count]=ftell(out);
      while(asm_context->debug_line[i]==-2)
      {
        putc(asm_context->bin[i++], out);
        putc(asm_context->bin[i++], out);
      }
      //i=fwrite(asm_context->bin+asm_context->low_address, 1, asm_context->high_address-asm_context->low_address+1, out);
      sections_size.data[data_count]=ftell(out)-sections_offset.data[data_count];
      data_count++;
      e_shnum++;
    }
      else
    if (asm_context->debug_line[i]!=-1)
    {
      if (text_count>=ELF_TEXT_MAX) { printf("Too many elf .text sections(%d).  Internal error.\n", text_count); exit(1); }
      if (text_count==0) { strcpy(name, ".text"); }
      else { sprintf(name, ".text%d", text_count); }
      text_addr[text_count]=i;
printf("and i=%d  text_count=%d\n", i, text_count);
      string_table_append(string_table, name);
      sections_offset.text[text_count]=ftell(out);
      while(asm_context->debug_line[i]>=0 || asm_context->debug_line[i]==-3)
      {
        putc(asm_context->bin[i++], out);
        putc(asm_context->bin[i++], out);
      }
      //i=fwrite(asm_context->bin+asm_context->low_address, 1, asm_context->high_address-asm_context->low_address+1, out);
      sections_size.text[text_count]=ftell(out)-sections_offset.text[text_count];
      text_count++;
      e_shnum++;
    }

    i++;
  }

  e_shstrndx=data_count+text_count+1;

  // .shstrtab section
  sections_offset.shstrtab=ftell(out);
  i=fwrite(string_table, 1, get_string_table_len(string_table), out);
  putc(0x00, out); // null
  sections_size.shstrtab=ftell(out)-sections_offset.shstrtab;

  {
    struct _address_heap *address_heap=&asm_context->address_heap;
    int symbol_count=0;
    int sym_offset=0;
    int ptr,n;

    // Count symbols
    ptr=0;
    while(ptr<address_heap->ptr)
    {
      int token_len=strlen((char *)address_heap->buffer+ptr)+1;
      //fprintf(out, "%s", address_heap->buffer+ptr);
      ptr=ptr+token_len+sizeof(int);
      symbol_count++;
    }

    int symbol_address[symbol_count];

    // .strtab section
    elf_addr_align(out);
    sections_offset.strtab=ftell(out);
    putc(0x00, out); // none

    fprintf(out, "%s%c", asm_context->filename, 0);
    sym_offset=strlen(asm_context->filename)+2;

    ptr=0;
    n=0;
    while(ptr<address_heap->ptr)
    {
      symbol_address[n++]=sym_offset;
      int token_len=strlen((char *)address_heap->buffer+ptr)+1;
      fprintf(out, "%s%c", address_heap->buffer+ptr, 0);
      ptr=ptr+token_len+sizeof(int);
      sym_offset+=token_len;
    }
    putc(0x00, out); // null
    sections_size.strtab=ftell(out)-sections_offset.strtab;

    // .symtab section
    elf_addr_align(out);
    sections_offset.symtab=ftell(out);

    // symtab text
    memset(&symtab, 0, sizeof(symtab));
    symtab.st_info=3;
    symtab.st_shndx=1;
    write_symtab(out, &symtab);

    // symtab filename
    memset(&symtab, 0, sizeof(symtab));
    symtab.st_name=1;
    symtab.st_info=4;
    write_symtab(out, &symtab);

    // symbols from lookup tables

    ptr=0;
    n=0;
    while(ptr<address_heap->ptr)
    {
      int token_len=strlen((char *)address_heap->buffer+ptr)+1;
      memset(&symtab, 0, sizeof(symtab));
      unsigned char *data=address_heap->buffer+ptr+token_len;
      symtab.st_name=symbol_address[n++];
      symtab.st_value=data[0]|(data[1]<<8)|(data[2]<<16)|(data[3]<<24);
      symtab.st_shndx=1;
      write_symtab(out, &symtab);
      ptr=ptr+token_len+sizeof(int);
    }

    sections_size.symtab=ftell(out)-sections_offset.symtab;
  }

  // .comment section
  sections_offset.comment=ftell(out);
  fprintf(out, "Created with naken430asm.  http://www.mikekohn.net/");
  sections_size.comment=ftell(out)-sections_offset.comment;

  if (asm_context->debug_file==1)
  {
    // insert debug sections
  }

  // A little ex-lax to dump the SHT's
  long marker=ftell(out);
  fseek(out, 32, SEEK_SET);
  write_int32_l(out, marker);     // e_shoff (section header offset)
  fseek(out, 0x30, SEEK_SET);
  write_int16_l(out, e_shnum);    // e_shnum (section count)
  write_int16_l(out, e_shstrndx); // e_shstrndx (string_table index)
  fseek(out, marker, SEEK_SET);

  memset(&shdr, 0, sizeof(shdr));
  write_shdr(out, &shdr);

  // SHT .text
  for (i=0; i<text_count; i++)
  {
    char name[32];
    if (i==0) { strcpy(name, ".text"); }
    else { sprintf(name, ".text%d", i); }
    shdr.sh_name=find_section(string_table, name, sizeof(string_table));
printf("name=%s (%d) %s\n", name, shdr.sh_name, string_table+ shdr.sh_name);
    shdr.sh_type=1;
    shdr.sh_flags=6;
printf("text_addr=%d\n", text_addr[i]);
    shdr.sh_addr=text_addr[i]; //asm_context->low_address;
    shdr.sh_offset=sections_offset.text[i];
    shdr.sh_size=sections_size.text[i];
    shdr.sh_addralign=1;
    write_shdr(out, &shdr);
  }

  // SHT .data
  for (i=0; i<data_count; i++)
  {
    char name[32];
    if (i==0) { strcpy(name, ".data"); }
    else { sprintf(name, ".data%d", i); }
    shdr.sh_name=find_section(string_table, name, sizeof(string_table));
    shdr.sh_type=1;
    shdr.sh_flags=3;
    shdr.sh_addr=data_addr[i]; //asm_context->low_address;
    shdr.sh_offset=sections_offset.data[i];
    shdr.sh_size=sections_size.data[i];
    shdr.sh_addralign=1;
    write_shdr(out, &shdr);
  }

  // SHT .shstrtab
  memset(&shdr, 0, sizeof(shdr));
  shdr.sh_name=find_section(string_table, ".shstrtab", sizeof(string_table));
  shdr.sh_type=3;
  shdr.sh_offset=sections_offset.shstrtab;
  shdr.sh_size=sections_size.shstrtab;
  shdr.sh_addralign=1;
  write_shdr(out, &shdr);

  // SHT .symtab
  memset(&shdr, 0, sizeof(shdr));
  shdr.sh_name=find_section(string_table, ".symtab", sizeof(string_table));
  shdr.sh_type=2;
  shdr.sh_offset=sections_offset.symtab;
  shdr.sh_size=sections_size.symtab;
  shdr.sh_addralign=4;
  shdr.sh_entsize=16;
  write_shdr(out, &shdr);

  // SHT .strtab
  memset(&shdr, 0, sizeof(shdr));
  shdr.sh_name=find_section(string_table, ".strtab", sizeof(string_table));
  shdr.sh_type=3;
  shdr.sh_offset=sections_offset.strtab;
  shdr.sh_size=sections_size.strtab;
  shdr.sh_addralign=1;
  write_shdr(out, &shdr);

  // SHT .comment
  memset(&shdr, 0, sizeof(shdr));
  shdr.sh_name=find_section(string_table, ".comment", sizeof(string_table));
  shdr.sh_type=1;
  shdr.sh_offset=sections_offset.comment;
  shdr.sh_size=sections_size.comment;
  shdr.sh_addralign=1;
  write_shdr(out, &shdr);

  if (asm_context->debug_file==1)
  {
    // insert debug SHT's
  }
 
  return 0;
}
#endif



