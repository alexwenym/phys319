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
#include "ifdef_expression.h"
#include "macros.h"

enum
{
  OPER_NONE=-1,
  OPER_EQUAL,
  OPER_GT_EQUAL,
  OPER_LT_EQUAL,
  OPER_GT,
  OPER_LT,
  OPER_OR,
  OPER_AND
};

enum
{
  PREC_OR=0,
  PREC_AND,
  PREC_EQUAL
};

struct _operator
{
  int operation;
  int precedence;
};

static int get_operator(char *token, struct _operator *operator)
{
  if (IS_TOKEN(token,'>'))
  {
    operator->operation=OPER_GT;
    operator->precedence=PREC_EQUAL;
  }
    else
  if (IS_TOKEN(token,'<'))
  {
    operator->operation=OPER_LT;
    operator->precedence=PREC_EQUAL;
  }
    else
  if (strcmp(token,"==")==0)
  {
    operator->operation=OPER_EQUAL;
    operator->precedence=PREC_EQUAL;
  }
    else
  if (strcmp(token,">=")==0)
  {
    operator->operation=OPER_GT_EQUAL;
    operator->precedence=PREC_EQUAL;
  }
    else
  if (strcmp(token,"<=")==0)
  {
    operator->operation=OPER_LT_EQUAL;
    operator->precedence=PREC_EQUAL;
  }
    else
  if (strcmp(token,"||")==0)
  {
    operator->operation=OPER_OR;
    operator->precedence=PREC_OR;
  }
    else
  if (strcmp(token,"&&")==0)
  {
    operator->operation=OPER_AND;
    operator->precedence=PREC_AND;
  }
    else
  {
    printf("Internal Error: Unknown equals_type %s:%d\n", __FILE__, __LINE__);
    return -1;
  }

  return 0;
}

static int parse_defined(struct _asm_context *asm_context)
{
char token[TOKENLEN];
int token_type;
int param_count; // throw away
int ret;

  token_type=get_token(asm_context, token, TOKENLEN);
  if (IS_NOT_TOKEN(token,'('))
  {
    print_error_unexp(token, asm_context);
    return -1;
  }

  token_type=get_token(asm_context, token, TOKENLEN);
  if (defines_heap_lookup(&asm_context->defines_heap, token, &param_count)!=NULL)
  { ret=1; }
    else
  { ret=0; }

  token_type=get_token(asm_context, token, TOKENLEN);
  if (IS_NOT_TOKEN(token,')'))
  {
    print_error_unexp(token, asm_context);
    return -1;
  }

  return ret;
}

static int eval_operation(int operator, int num1, int num2)
{
#ifdef DEBUG
printf("debug> #if eval_operation()  operator=%d  num1=%d  num2=%d\n", operator, num1, num2);
#endif

  switch(operator)
  {
    case OPER_EQUAL:
      return num1==num2?1:0;
    case OPER_GT_EQUAL:
      return num1>=num2?1:0;
    case OPER_LT_EQUAL:
      return num1<=num2?1:0;
    case OPER_GT:
      return num1>num2?1:0;
    case OPER_LT:
      return num1<num2?1:0;
    case OPER_OR:
      return ((num1|num2)!=0)?1:0;
    case OPER_AND:
      if (num1!=0 && num2!=0) return 1;
      return 0;
  }

  printf("Internal Error: Error in eval_operation() %s:%d\n", __FILE__, __LINE__);
  return -1;
}

static int is_num(char *s)
{
  while(*s!=0) { if (*s<'0' || *s>'9') return 0; s++; }
  return 1;
}

static int parse_ifdef_expression(struct _asm_context *asm_context, int *num, int paren_count, int precedence, int state)
{
char token[TOKENLEN];
struct _operator operator;
int token_type;
//int state=0;  // 0 = get next num to process
int not=0;
int n1=0;
int n;

  operator.operation=OPER_NONE;
  operator.precedence=precedence;
  n=*num;

  while(1)
  {
    token_type=get_token(asm_context, token, TOKENLEN);

#ifdef DEBUG
printf("debug> #if: %d) %s   n=%d paren_count=%d precedence=%d state=%d\n", token_type, token, n, paren_count, precedence, state);
#endif

    if (token_type==TOKEN_EOL || token_type==TOKEN_EOF)
    {
      pushback(asm_context, token, token_type);

      if (paren_count!=0)
      {
        print_error("Unbalanced parentheses.", asm_context);
        return -1;
      }

      if (state!=1)
      {
        print_error("Unexpected end of expression.", asm_context);
        return -1;
      }

      if (operator.operation!=OPER_NONE)
      {
        n=eval_operation(operator.operation,n1,n);
#ifdef DEBUG
printf("debug> #if eval_operation() @EOL  n=%d precedence=%d state=%d\n", n, precedence, state);
#endif
        if (n==-1) { return -1; }
      }

      *num=n;
      return 0;
    }

    if (state!=1)
    {
      if (state==2)
      {
        n1=n;
      }

      if (token_type==TOKEN_SYMBOL)
      {
        if (IS_TOKEN(token,'!'))
        {
          not^=1;
          continue;
        }
          else
        if (IS_TOKEN(token,'('))
        {
          if (parse_ifdef_expression(asm_context, &n, paren_count+1, PREC_OR, 0)==-1) return -1;
        }
          else
        if (IS_TOKEN(token,')'))
        {
          if (paren_count==0)
          {
            print_error_unexp(token, asm_context);
            return -1;
          }

          if (state!=1)
          {
            print_error("Unexpected end of expression.", asm_context);
            return -1;
          }

          if (operator.operation!=OPER_NONE)
          {
            n=eval_operation(operator.operation,n1,n);
#ifdef DEBUG
printf("debug> #if eval_operation() @paren  n=%d\n", n);
#endif
            if (n==-1) { return -1; }
          }

          *num=n;
          return 0;
        }
      }
        else
      if (token_type==TOKEN_STRING)
      {
        int param_count;
        char *value=defines_heap_lookup(&asm_context->defines_heap, token, &param_count);

        if (strcasecmp(token, "defined")==0)
        {
          n=parse_defined(asm_context);
#ifdef DEBUG
printf("debug> #if: parse_defined()=%d\n", n);
#endif
          if (n==-1) return -1;
        }
          else
        if (value!=NULL && param_count==0 && is_num(value))
        {
          n=atoi(value);
        }
          else
        {
          print_error_unexp(token, asm_context);
          return -1;
        }
      }
        else
      if (token_type==TOKEN_NUMBER)
      {
        n=atoi(token);
      }

      if (not==1)
      {
        if (n==0)
        { n=1; }
          else
        { n=0; }

        not=0;
      }

#if 0
      if (state==2)
      {
#ifdef DEBUG
printf("debug> #if eval_operation() n1=%d n=%d\n", n1, n);
#endif
        n=eval_operation(operator.operation,n1,n);
        state=0;
#ifdef DEBUG
printf("debug> #if eval_operation() n=%d operation=%d\n", n, operator.operation);
#endif
      }
#endif

      state=1;
      continue;
    }

    if (token_type==TOKEN_SYMBOL || token_type==TOKEN_EQUALITY)
    {
      struct _operator next_operator;

      if (get_operator(token, &next_operator)==-1)
      {
        print_error_unexp(token, asm_context);
        return -1;
      }

#ifdef DEBUG
printf("debug> #if get_operator() token=%s operation=%d precedence=%d\n", token, next_operator.operation, next_operator.precedence);
#endif

      if (next_operator.precedence>precedence)
      {
        pushback(asm_context, token, token_type);
        if (parse_ifdef_expression(asm_context, &n, paren_count, next_operator.precedence, 1)==-1) return -1;
      }
        else
      if (next_operator.precedence<precedence)
      {
        pushback(asm_context, token, token_type);
        return 0;
      }
        else
      {
        state=2;

        if (operator.operation!=OPER_NONE)
        {
          n=eval_operation(operator.operation,n1,n);
#ifdef DEBUG
printf("debug> #if eval_operation() @ state 2  n=%d\n", n);
#endif
          if (n==-1) { return -1; }
        }

        operator=next_operator;
      }

      continue;
    }

    print_error_unexp(token, asm_context);
    return -1;
  }

  return -1;
}

int eval_ifdef_expression(struct _asm_context *asm_context)
{
char token[TOKENLEN];
int token_type;
int num=0;

  if (parse_ifdef_expression(asm_context, &num, 0, PREC_OR, 0)==-1) return -1;

#ifdef DEBUG
printf("debug> parse_ifdef_expression() result is %d\n", num);
#endif
  token_type=get_token(asm_context, token, TOKENLEN);

  if (token_type!=TOKEN_EOL && token_type!=TOKEN_EOF)
  {
    print_error_unexp(token, asm_context);
  }
    else
  { asm_context->line++; }

  return num;
}


