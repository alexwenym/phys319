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

#include "get_tokens.h"
#include "lookup_tables.h"
#include "macros.h"

static int get_param_index(char *params, char *name)
{
int count=1;
int ptr=0;

  while(params[ptr]!=0)
  {
    if (strcmp(params+ptr,name)==0) return count;
    count++;
    ptr=ptr+strlen(params+ptr)+1;
  }

  return 0;
}

static int parse_macro_token(struct _asm_context *asm_context, char *token, int len)
{
int ptr=0;
char ch;

  while(1)
  {
    ch=get_next_char(asm_context);
    if (ch==' ')
    {
      if (ptr==0) continue;
      break;
    }

    if ((ch>='a' && ch <='z') || (ch>='A' && ch <='Z') || ch=='_')
    {
      token[ptr++]=ch;
    }
      else
    if (ch>='0' && ch <='9')
    {
      if (ptr==0)
      {
        print_error("Bad macro name", asm_context);
        return -1;
      }

      token[ptr++]=ch;
    }
      else
    if (ch=='(')
    {
      token[ptr]=0;
      return 1;
    }
      else
    {
      unget_next_char(asm_context,ch);
      break;
    }

    if (ptr==len-1) break;
  }

  token[ptr]=0;

  return 0;
}

void strip_macro(char *macro)
{
  char *s = macro;
  // Remove ; and // comments
  while (*s!=0)
  {
    if (*s==';') { *s=0; break; }
    if (*s=='/' && *(s+1)=='/') { *s=0; break; }
    s++;
  }

  if (s!=macro) s--;

  // Trim spaces from end of macro
  while (s!=macro)
  {
    if (*s!=' ') break;
    *s=0;
    s--;
  }
}

void eatout_star_comment(struct _asm_context *asm_context)
{
int it, cl=0;

#ifdef DEBUG
printf("debug> eatout_star_comment()\n");
#endif

  while(1)
  {
    it=get_next_char(asm_context);
    if (it==EOF) break;
    if (it=='\n') asm_context->line++;
    if (it=='/' && cl=='*') break;
    cl=it;
  }
}

static int check_endm(char *macro, int ptr)
{
  ptr--;

  while(ptr>0 && macro[ptr]!='\n')
  {
    ptr--;
  }

  ptr++;
  if (strncmp(macro+ptr, ".endm", 5)==0 || strncmp(macro+ptr, "endm", 4)==0)
  {
    macro[ptr]=0;
    return 1;
  }

  return 0;
}

int parse_macro(struct _asm_context *asm_context, int macro_type)
{
char name[128];
char token[TOKENLEN];
char params[1024];
char *name_test;
char macro[MAX_MACRO_LEN];
int ptr=0;
int token_type;
int ch;
int cont=0;
int parens=0;
int param_count=0;

  // First pull the name out
  parens=parse_macro_token(asm_context, name, 128);
  if (parens==-1) return -1;

#ifdef DEBUG
printf("#define %s   parens_flag=%d\n", name, parens);
#endif

  // Now pull any params out
  ptr=0;
  if (parens!=0)
  {
    while(1)
    {
      token_type=get_token(asm_context, token, TOKENLEN);
#ifdef DEBUG
printf("debug> #ifdef param %s\n", token);
#endif
      if (token_type!=TOKEN_STRING)
      {
        printf("Error: Expected a param name but got '%s' at %s:%d.\n",token,asm_context->filename,asm_context->line);
        return -1;
      }

      param_count++;

      int len=strlen(token);
      if (ptr+len+2>1024)
      {
        print_error("Macro with too long parameter list\n",asm_context);
        return -1;
      }

      strcpy(params+ptr,token);
      ptr=ptr+len+1;

      token_type=get_token(asm_context, token, TOKENLEN);
      if (IS_TOKEN(token,','))
      {
      }
        else
      if (IS_TOKEN(token,')'))
      {
        break;
      }
        else
      {
        print_error("Expected ',' or ')'", asm_context);
        return -1;
      }
    }
  }

  params[ptr]=0;

#ifdef DEBUG
printf("debug> #define param count=%d\n", param_count);
#endif

  // Now macro time
  ptr=0;
  name_test=NULL;
  while(1)
  {
    ch=get_next_char(asm_context);
    if (name_test==NULL)
    {
      if ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z'))
      {
        name_test=macro+ptr;
      }
    }
      else
    if (!((ch>='a' && ch<='z') || (ch>='A' && ch<='Z') ||
          (ch>='0' && ch<='9')))
    {
      if (name_test!=NULL)
      {
        macro[ptr]=0;
        int index=get_param_index(params,name_test);

#ifdef DEBUG
printf("debug> #ifdef param found %s %d\n",name_test,index);
#endif

        if (index!=0)
        {
          ptr=name_test-macro;
          macro[ptr++]=index;
        }
        name_test=NULL;
      }
    }

    if (ch==';' || (ptr>0 && macro[ptr-1]=='/'))
    {
      if (macro[ptr-1]=='/') ptr--;
      while(1)
      {
        ch=get_next_char(asm_context);
        if (ch=='\n' || ch==EOF) break;
      }

      while(ptr>0)
      {
        if (macro[ptr-1]!=' ') break;
        ptr--;
      }
    }

    if (ch=='\r') continue;
    if (ch==' ' && (ptr==0 || cont!=0)) continue;
    if (ch=='\\' && cont==0)
    {
      if (macro_type==IS_DEFINE)
      {
        cont=1;
        continue;
      }
    }

    if (ch=='\n' || ch==EOF)
    {
      asm_context->line++;

      if (macro_type==IS_DEFINE)
      {
        if (cont==1)
        {
          macro[ptr++]=ch;
          cont=2;
          continue;
        }

        break;
      }
        else
      {
        if (check_endm(macro,ptr)==1) break;
      }
    }

    if (cont==1)
    {
      printf("Parse error: Expecting end-of-line on line %d\n", asm_context->line);
      return -1;
    }

    cont=0;

    if (ch=='*' && ptr>0 && macro[ptr-1]=='/')
    {
      eatout_star_comment(asm_context);
      ptr--;
      continue;
    }

    macro[ptr++]=ch;
    if (ptr>=MAX_MACRO_LEN-1)
    {
      printf("Internal error: macro longer than %d bytes on line %d\n",MAX_MACRO_LEN,asm_context->line);
      return -1;
    }
  }

  macro[ptr++]=0;

#ifdef DEBUG
printf("Debug: adding macro '%s'\n", macro);
#endif

  strip_macro(macro);

  // defines_heap_append(&asm_context->defines_heap, name, macro, param_count);
  defines_heap_append(asm_context, name, macro, param_count);

  return 0;
}

char *expand_params(struct _asm_context *asm_context, char *define, int param_count)
{
int ch;
char params[1024];
int params_ptr[256];
int count,ptr;

  ch=get_next_char(asm_context);

  if (ch!='(')
  {
    print_error("Macro expects params", asm_context);
    return 0;
  }

  count=0;
  ptr=0;
  params_ptr[count]=ptr;

  while(1)
  {
    ch=get_next_char(asm_context);
    if (ch=='\r') continue;
    if (ch==')') break;
    if (ch=='\n' || ch==EOF)
    {
      print_error("Macro expects ')'", asm_context);
      return NULL;
    }
    if (ch==',')
    {
      params[ptr++]=0;
      params_ptr[++count]=ptr;
      continue;
    }

    params[ptr++]=ch;
  }

  params[ptr++]=0;
  count++;
  if (count!=param_count)
  {
    printf("Error: Macro expects %d params, but got only %d at %s:%d.\n", param_count, count, asm_context->filename, asm_context->line);
    return NULL;
  }

#ifdef DEBUG
printf("debug> Expanding macro with params: pass=%d\n", asm_context->pass);
int n;
for (n=0; n<count; n++)
{
  printf("debug>   %s\n", params+params_ptr[n]);
}
#endif

  ptr=asm_context->def_param_stack_ptr[asm_context->def_param_stack_count];

  while(*define!=0)
  {
    if (*define<10)
    {
      strcpy(asm_context->def_param_stack_data+ptr, params+params_ptr[((int)*define)-1]);
      while(*(asm_context->def_param_stack_data+ptr)!=0) ptr++;
    }
    else
    {
      asm_context->def_param_stack_data[ptr++]=*define;
    }

    if (ptr>=PARAM_STACK_LEN)
    {
      printf("Internal error: %s:%d\n", __FILE__, __LINE__);
      exit(1);
    }

    define++;
  }

#ifdef DEBUG
printf("debug>   ptr=%d\n", ptr);
#endif
  asm_context->def_param_stack_data[ptr++]=0;

#ifdef DEBUG
printf("debug> Expanded macro becomes: %s\n", asm_context->def_param_stack_data+asm_context->def_param_stack_ptr[asm_context->def_param_stack_count]);
#endif

  asm_context->def_param_stack_ptr[++asm_context->def_param_stack_count]=ptr;

  return asm_context->def_param_stack_data+asm_context->def_param_stack_ptr[asm_context->def_param_stack_count-1];
}

int defines_heap_init(struct _defines_heap *defines_heap)
{
  defines_heap->buffer=(unsigned char *)malloc(DEFINES_HEAP_SIZE);
  defines_heap->len=DEFINES_HEAP_SIZE;
  defines_heap->ptr=0;
  defines_heap->locked=0;

  return 0;
}

void defines_heap_free(struct _defines_heap *defines_heap)
{
  free(defines_heap->buffer);
}

int defines_heap_append(struct _asm_context *asm_context, char *name, char *value, int param_count)
{
int name_len;
int value_len;
int param_count_temp;
struct _defines_heap *defines_heap=&asm_context->defines_heap;

  if (defines_heap->locked==1) return 0;

  if (defines_heap_lookup(defines_heap, name, &param_count_temp)!=NULL || address_heap_lookup(&asm_context->address_heap, name)!=-1)
  {
    printf("Error: #define '%s' already defined.\n", name);
    return -1;
  }

  name_len=strlen(name)+1;
  value_len=strlen(value)+1;

  if (defines_heap->ptr+name_len+value_len>=defines_heap->len)
  {
    if (name_len+value_len<65536)
    { defines_heap->len+=65536; }
      else
    { defines_heap->len+=65536+name_len+value_len; }

    defines_heap->buffer=(unsigned char *)realloc(defines_heap->buffer, defines_heap->len);
#ifdef DEBUG
    printf("Uh oh: defines_heap exhaused.. growing to %d\n",defines_heap->len);
#endif
    //printf("Internal error: defines_heap exhaused.\n");
    //exit(1);
    //return -1;
  }

  memcpy(defines_heap->buffer+defines_heap->ptr, name, name_len);
  memcpy(defines_heap->buffer+defines_heap->ptr+name_len, value, value_len);
  defines_heap->ptr+=name_len+value_len+1;
  defines_heap->buffer[defines_heap->ptr-1]=(unsigned char)param_count;

  return 0;
} 

void defines_heap_lock(struct _defines_heap *defines_heap)
{
  defines_heap->locked=1;
}

char *defines_heap_lookup(struct _defines_heap *defines_heap, char *name, int *param_count)
{
int name_len;
int value_len;
char *value;
int ptr=0;

  while(ptr<defines_heap->ptr)
  {
    name_len=strlen((char *)defines_heap->buffer+ptr)+1;
    value_len=strlen((char *)defines_heap->buffer+ptr+name_len)+1;

    if (strcmp((char *)defines_heap->buffer+ptr, name)==0)
    {
      value=(char *)defines_heap->buffer+ptr+strlen((char *)defines_heap->buffer+ptr)+1;
      *param_count=*(value+strlen(value)+1);
      return value;
    }
    ptr=ptr+name_len+value_len+1;
  }

  return NULL;
}

int defines_heap_print(struct _defines_heap *defines_heap)
{
int name_len;
int value_len;
int ptr=0;
int count=0;
int param_count=0;

  printf("defines_heap.len=%d  defines_heap.ptr=%d\n\n", defines_heap->len, defines_heap->ptr);

  printf("%18s %s\n", "NAME", "VALUE");

  while(ptr<defines_heap->ptr)
  {
    name_len=strlen((char *)defines_heap->buffer+ptr)+1;
    value_len=strlen((char *)defines_heap->buffer+ptr+name_len)+1;
    param_count=*((char *)defines_heap->buffer+ptr+name_len+value_len);
    if (param_count==0)
    {
      printf("%30s=", defines_heap->buffer+ptr);
    }
      else
    {
      int n;

      printf("%30s(", defines_heap->buffer+ptr);
      for (n=0; n<param_count; n++)
      {
        if (n!=0) printf(",");
        printf("{%d}",n+1);
      }
      printf(")=");
    }

    unsigned char *name=defines_heap->buffer+ptr+name_len;
    while (*name!=0)
    {
      if (*name<10)
      { printf("{%d}",*name); }
        else
      { printf("%c",*name); }
      name++;
    }

    printf("\n");
    ptr=ptr+name_len+value_len+1;
    count++;
  }

  printf("Total %d.\n\n", count);

  return 0;
}

int defines_heap_push_define(struct _defines_heap *defines_heap, char *define)
{
#ifdef DEBUG
printf("debug> defines_heap_push_define, define=%s defines_heap->stack_ptr=%d\n", define, defines_heap->stack_ptr);
#endif
  if (defines_heap->stack_ptr>=MAX_NESTED_MACROS)
  {
    printf("Internal Error: defines heap stack exhausted.\n");
    return -1;
  }

  defines_heap->stack[defines_heap->stack_ptr++]=define;

  return 0;
}

int defines_heap_get_char(struct _asm_context *asm_context)
{
int stack_ptr;
int ch;

  struct _defines_heap *defines_heap=&asm_context->defines_heap;

  while(1)
  {
    // Is there even a character waiting on the #define stack?
    stack_ptr=defines_heap->stack_ptr-1;
    if (stack_ptr<0) return CHAR_EOF;

    // Pull the next char off the stack
    ch=*defines_heap->stack[stack_ptr];
    defines_heap->stack[stack_ptr]++;

    // If we have a char then break this loop and return (all is good)
    if (ch!=0) break;

    // drop the #define stack by 1 level
    if (defines_heap->stack[stack_ptr]>=asm_context->def_param_stack_data && defines_heap->stack[stack_ptr]<asm_context->def_param_stack_data+PARAM_STACK_LEN)
    {
      asm_context->def_param_stack_count--;
      if (asm_context->def_param_stack_count<0)
      {
        printf("Internal error: %s:%d\n", __FILE__, __LINE__);
        exit(1);
      }
#ifdef DEBUG
printf("debug> asm_context->def_param_stack_count=%d\n",asm_context->def_param_stack_count);
#endif
    }
    defines_heap->stack_ptr--;
    asm_context->unget_stack_ptr--;

    // Check if something need to be ungetted
    if (asm_context->unget_ptr>asm_context->unget_stack[asm_context->unget_stack_ptr])
    {
#ifdef DEBUG
printf("debug> get_next_char(?) ungetc %d %d '%c'\n", asm_context->unget_stack_ptr, asm_context->unget_stack[asm_context->unget_stack_ptr], asm_context->unget[asm_context->unget_ptr-1]);
#endif
      return asm_context->unget[--asm_context->unget_ptr];
    }
  }

  return ch;
}


