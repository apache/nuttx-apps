/****************************************************************************
 * apps/interpreters/bas/bas.c
 *
 *   Copyright (c) 1999-2014 Michael Haardt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Adapted to NuttX and re-released under a 3-clause BSD license:
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Authors: Alan Carvalho de Assis <Alan Carvalho de Assis>
 *            Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include "bas_auto.h"
#include "bas.h"
#include "bas_error.h"
#include "bas_fs.h"
#include "bas_global.h"
#include "bas_program.h"
#include "bas_value.h"
#include "bas_var.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DIRECTMODE (g_pc.line== -1)
#define _(String) String

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum labeltype_e
  {
    L_IF = 1,
    L_ELSE,
    L_DO,
    L_DOcondition,
    L_FOR,
    L_FOR_VAR,
    L_FOR_LIMIT,
    L_FOR_BODY,
    L_REPEAT,
    L_SELECTCASE,
    L_WHILE,
    L_FUNC
  };

struct labelstack_s
  {
    enum labeltype_e type;
    struct Pc patch;
  };

/****************************************************************************
 * Private Data
 ****************************************************************************/

static unsigned int g_labelstack_index;
static unsigned int g_labelstack_capacity;
static struct labelstack_s *g_labelstack;
static struct Pc *g_lastdata;
static struct Pc g_curdata;
static struct Pc g_nextdata;

static enum
  {
    DECLARE,
    COMPILE,
    INTERPRET
  } g_pass;

static int g_stopped;
static int g_optionbase;
static struct Pc g_pc;
static struct Auto g_stack;
static struct Program g_program;
static struct Global g_globals;
static int g_run_restricted;

/****************************************************************************
 * Public Data
 ****************************************************************************/

int g_bas_argc;
char *g_bas_argv0;
char **g_bas_argv;
bool g_bas_end;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static struct Value *statements(struct Value *value);
static struct Value *compileProgram(struct Value *v, int clearGlobals);
static struct Value *eval(struct Value *value, const char *desc);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int cat(const char *filename)
{
  int fd;
  char buf[4096];
  ssize_t l;
  int errcode;

  if ((fd = open(filename, O_RDONLY)) == -1)
    {
      return -1;
    }

  while ((l = read(fd, buf, sizeof(buf))) > 0)
    {
      ssize_t off;
      ssize_t w;

      off = 0;
      while (off < l)
        {
          if ((w = write(1, buf + off, l - off)) == -1)
            {
              errcode = errno;
              close(fd);
              errno = errcode;
              return -1;
            }

          off += w;
        }
    }

  if (l == -1)
    {
      errcode = errno;
      close(fd);
      errno = errcode;
      return -1;
    }

  close(fd);
  return 0;
}

static struct Value *lvalue(struct Value *value)
{
  struct Symbol *sym;
  struct Pc lvpc = g_pc;

  sym = g_pc.token->u.identifier->sym;
  assert(g_pass == DECLARE || sym->type == GLOBALVAR
         || sym->type == GLOBALARRAY || sym->type == LOCALVAR);

  if ((g_pc.token + 1)->type == T_OP)
    {
      struct Pc idxpc;
      unsigned int dim;
      unsigned int capacity;
      int *idx;

      g_pc.token += 2;
      dim = 0;
      capacity = 0;
      idx = (int *)0;
      while (1)
        {
          if (dim == capacity && g_pass == INTERPRET)     /* enlarge idx */
            {
              int *more;

              more =
                realloc(idx,
                        sizeof(unsigned int) *
                        (capacity ? (capacity *= 2) : (capacity = 3)));
              if (!more)
                {
                  if (capacity)
                    free(idx);
                  return Value_new_ERROR(value, OUTOFMEMORY);
                }

              idx = more;
            }

          idxpc = g_pc;
          if (eval(value, _("index"))->type == V_ERROR ||
              VALUE_RETYPE(value, V_INTEGER)->type == V_ERROR)
            {
              if (capacity)
                {
                  free(idx);
                }

              g_pc = idxpc;
              return value;
            }

          if (g_pass == INTERPRET)
            {
              idx[dim] = value->u.integer;
              ++dim;
            }

          Value_destroy(value);
          if (g_pc.token->type == T_COMMA)
            {
              ++g_pc.token;
            }
          else
            {
              break;
            }
        }

      if (g_pc.token->type != T_CP)
        {
          assert(g_pass != INTERPRET);
          return Value_new_ERROR(value, MISSINGCP);
        }
      else
        {
          ++g_pc.token;
        }

      switch (g_pass)
        {
        case INTERPRET:
          {
            if ((value =
                 Var_value(&(sym->u.var), dim, idx, value))->type == V_ERROR)
              {
                g_pc = lvpc;
              }

            free(idx);
            return value;
          }

        case DECLARE:
          {
            return Value_nullValue(V_INTEGER);
          }

        case COMPILE:
          {
            return Value_nullValue(sym->type ==
                                   GLOBALARRAY ? sym->u.
                                   var.type : Auto_varType(&g_stack, sym));
          }

        default:
          assert(0);
        }

      return (struct Value *)0;
    }
  else
    {
      ++g_pc.token;
      switch (g_pass)
        {
        case INTERPRET:
          return VAR_SCALAR_VALUE(sym->type == GLOBALVAR ? &(sym->u.var) :
                                  Auto_local(&g_stack, sym->u.local.offset));

        case DECLARE:
          return Value_nullValue(V_INTEGER);

        case COMPILE:
          return Value_nullValue(sym->type ==
                                 GLOBALVAR ? sym->u.
                                 var.type : Auto_varType(&g_stack, sym));

        default:
          assert(0);
        }

      return (struct Value *)0;
    }
}

static struct Value *func(struct Value *value)
{
  struct Identifier *ident;
  struct Pc funcpc = g_pc;
  long int firstslot = -99;
  int args = 0;
  struct Symbol *sym;

  assert(g_pc.token->type == T_IDENTIFIER);

  /* Evaluating a function in direct mode may start a program, so it needs to
   * be compiled.  If in direct mode, programs will be compiled after the
   * direct mode pass DECLARE, but errors are ignored at that point, because
   * the program may not be needed.  If the program is fine, its symbols will
   * be available during the compile phase already.  If not and we need it at
   * this point, compile it again to get the error and abort.
   */

  if (DIRECTMODE && !g_program.runnable && g_pass != DECLARE)
    {
      if (compileProgram(value, 0)->type == V_ERROR)
        {
          return value;
        }

      Value_destroy(value);
    }

  ident = g_pc.token->u.identifier;
  assert(g_pass == DECLARE || ident->sym->type == BUILTINFUNCTION ||
         ident->sym->type == USERFUNCTION);
  ++g_pc.token;
  if (g_pass != DECLARE)
    {
      firstslot = g_stack.stackPointer;
      if (ident->sym->type == USERFUNCTION &&
          ident->sym->u.sub.retType != V_VOID)
        {
          struct Var *v = Auto_pushArg(&g_stack);
          Var_new(v, ident->sym->u.sub.retType, 0, NULL, 0);
        }
    }

  if (g_pc.token->type == T_OP)   /* push arguments to stack */
    {
      ++g_pc.token;
      if (g_pc.token->type != T_CP)
        {
          while (1)
            {
              if (g_pass == DECLARE)
                {
                  if (eval(value, _("actual parameter"))->type == V_ERROR)
                    {
                      return value;
                    }

                  Value_destroy(value);
                }
              else
                {
                  struct Var *v = Auto_pushArg(&g_stack);

                  Var_new_scalar(v);
                  if (eval(v->value, (const char *)0)->type == V_ERROR)
                    {
                      Value_clone(value, v->value);
                      while (g_stack.stackPointer > firstslot)
                        {
                          long int stackPointer;

                          stackPointer = --g_stack.stackPointer;
                          Var_destroy(&g_stack.slot[stackPointer].var);
                        }

                      return value;
                    }

                  v->type = v->value->type;
                }

              ++args;
              if (g_pc.token->type == T_COMMA)
                {
                  ++g_pc.token;
                }
              else
                {
                  break;
                }
            }

          if (g_pc.token->type != T_CP)
            {
              if (g_pass != DECLARE)
                {
                  while (g_stack.stackPointer > firstslot)
                    {
                      long int stackPointer;

                      stackPointer = --g_stack.stackPointer;
                      Var_destroy(&g_stack.slot[stackPointer].var);
                    }
                }

              return Value_new_ERROR(value, MISSINGCP);
            }

          ++g_pc.token;
        }
    }

  if (g_pass == DECLARE)
    {
      Value_new_null(value, ident->defaultType);
    }
  else
    {
      int i;
      int nomore;
      int argerr;
      int overloaded;

      if (g_pass == INTERPRET && ident->sym->type == USERFUNCTION)
        {
          for (i = 0; i < ident->sym->u.sub.u.def.localLength; ++i)
            {
              struct Var *v = Auto_pushArg(&g_stack);
              Var_new(v, ident->sym->u.sub.u.def.localTypes[i], 0,
                      (const unsigned int *)0, 0);
            }
        }

      Auto_pushFuncRet(&g_stack, firstslot, &g_pc);

      sym = ident->sym;
      overloaded = (g_pass == COMPILE && sym->type == BUILTINFUNCTION &&
                    sym->u.sub.u.bltin.next);
      do
        {
          nomore = (g_pass == COMPILE &&
                    !(sym->type == BUILTINFUNCTION &&
                      sym->u.sub.u.bltin.next));
          argerr = 0;
          if (args < sym->u.sub.argLength)
            {
              if (nomore)
                {
                  Value_new_ERROR(value, TOOFEW);
                }

              argerr = 1;
            }

          else if (args > sym->u.sub.argLength)
            {
              if (nomore)
                {
                  Value_new_ERROR(value, TOOMANY);
                }

              argerr = 1;
            }
          else
            {
              for (i = 0; i < args; ++i)
                {
                  struct Value *arg =
                    Var_value(Auto_local(&g_stack, i), 0, (int *)0, value);

                  assert(arg->type != V_ERROR);
                  if (overloaded)
                    {
                      if (arg->type != sym->u.sub.argTypes[i])
                        {
                          if (nomore)
                            {
                              Value_new_ERROR(value, TYPEMISMATCH2, i + 1);
                            }

                          argerr = 1;
                          break;
                        }
                    }
                  else if (Value_retype(arg, sym->u.sub.argTypes[i])->type ==
                           V_ERROR)
                    {
                      if (nomore)
                        {
                          Value_new_ERROR(value, TYPEMISMATCH3,
                                          arg->u.error.msg, i + 1);
                        }

                      argerr = 1;
                      break;
                    }
                }
            }

          if (argerr)
            {
              if (nomore)
                {
                  Auto_funcReturn(&g_stack, (struct Pc *)0);
                  g_pc = funcpc;
                  return value;
                }
              else
                {
                  sym = sym->u.sub.u.bltin.next;
                }
            }
        }
      while (argerr);

      ident->sym = sym;
      if (sym->type == BUILTINFUNCTION)
        {
          if (g_pass == INTERPRET)
            {
              if (sym->u.sub.u.bltin.call(value, &g_stack)->type == V_ERROR)
                {
                  g_pc = funcpc;
                }
            }
          else
            {
              Value_new_null(value, sym->u.sub.retType);
            }
        }
      else if (sym->type == USERFUNCTION)
        {
          if (g_pass == INTERPRET)
            {
              int r = 1;

              g_pc = sym->u.sub.u.def.scope.start;
              if (g_pc.token->type == T_COLON)
                {
                  ++g_pc.token;
                }
              else
                {
                  Program_skipEOL(&g_program, &g_pc, STDCHANNEL, 1);
                }

              do
                {
                  if (statements(value)->type == V_ERROR)
                    {
                      if (strchr(value->u.error.msg, '\n') == (char *)0)
                        {
                          Auto_setError(&g_stack,
                            Program_lineNumber(&g_program, &g_pc),
                            &g_pc, value);
                          Program_PCtoError(&g_program, &g_pc, value);
                        }

                      if (g_stack.onerror.line != -1)
                        {
                          g_stack.resumeable = 1;
                          g_pc = g_stack.onerror;
                        }
                      else
                        {
                          Auto_frameToError(&g_stack, &g_program, value);
                          break;
                        }
                    }
                  else if (value->type != V_NIL)
                    {
                      break;
                    }

                  Value_destroy(value);
                }
              while ((r = Program_skipEOL(&g_program, &g_pc,
                                          STDCHANNEL, 1)));

              if (!r)
                {
                  Value_new_VOID(value);
                }
            }
          else
            {
              Value_new_null(value, sym->u.sub.retType);
            }
        }

      Auto_funcReturn(&g_stack, g_pass == INTERPRET &&
                      value->type != V_ERROR ? &g_pc : (struct Pc *)0);
    }

  return value;
}

#ifdef CONFIG_INTERPRETER_BAS_USE_LR0

/* Grammar with LR(0) sets */

/* Grammar:
 *
 *   1 EV -> E
 *   2 E  -> E op E
 *   3 E  -> op E
 *   4 E  -> ( E )
 *   5 E  -> value
 *
 *   i0:
 *   EV -> . E                goto(0,E)=5
 *   E  -> . E op E           goto(0,E)=5
 *   E  -> . op E      +,-    shift 2
 *   E  -> . ( E )     (      shift 3
 *   E  -> . value     value  shift 4
 *
 *   i5:
 *   EV -> E .         else   accept
 *   E  -> E . op E    op     shift 1
 *
 *   i2:
 *   E  -> op . E             goto(2,E)=6
 *   E  -> . E op E           goto(2,E)=6
 *   E  -> . op E      +,-    shift 2
 *   E  -> . ( E )     (      shift 3
 *   E  -> . value     value  shift 4
 *
 *   i3:
 *   E  -> ( . E )            goto(3,E)=7
 *   E  -> . E op E           goto(3,E)=7
 *   E  -> . op E      +,-    shift 2
 *   E  -> . ( E )     (      shift 3
 *   E  -> . value     value  shift 4
 *
 *   i4:
 *   E  -> value .            reduce 5
 *
 *   i1:
 *   E  -> E op . E           goto(1,E)=8
 *   E  -> . E op E           goto(1,E)=8
 *   E  -> . op E      +,-    shift 2
 *   E  -> . ( E )     (      shift 3
 *   E  -> . value     value  shift 4
 *
 *   i6:
 *   E  -> op E .             reduce 3
 *   E  -> E . op E    op*    shift 1 *=if stack[-2] contains op of unary
 *                                      lower priority
 *
 *   i7:
 *   E  -> ( E . )     )      shift 9
 *   E  -> E . op E    op     shift 1
 *
 *   i8:
 *   E  -> E op E .           reduce 2
 *   E  -> E . op E    op*    shift 1 *=if stack[-2] contains op of lower
 *                                      priority or if it is of equal
 *                                      priority and right associative
 *   i9:
 *   E  -> ( E ) .            reduce 4
 */

static struct Value *eval(struct Value *value, const char *desc)
{
  /* Variables */

  static const int gotoState[10] =
  {
    5, 8, 6, 7, -1, -1, -1, -1, -1, -1
  };

  int capacity = 10;
  struct Pdastack
    {
      union
        {
          enum TokenType token;
          struct Value value;
        } u;
      char state;
    };

  struct Pdastack *pdastack = malloc(capacity * sizeof(struct Pdastack));
  struct Pdastack *sp = pdastack;
  struct Pdastack *stackEnd = pdastack + capacity - 1;
  enum TokenType ip;

  sp->state = 0;
  while (1)
    {
      if (sp == stackEnd)
        {
          pdastack =
            realloc(pdastack, (capacity + 10) * sizeof(struct Pdastack));
          sp = pdastack + capacity - 1;
          capacity += 10;
          stackEnd = pdastack + capacity - 1;
        }

      ip = g_pc.token->type;
      switch (sp->state)
        {
        case 0:
        case 1:
        case 2:
        case 3:                /* including 4 */
          {
            if (ip == T_IDENTIFIER)
              {
                /* printf("state %d: shift 4\n",sp->state);
                 * printf("state 4: reduce E -> value\n");
                 */

                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                if (g_pass == COMPILE)
                  {
                    if (((g_pc.token + 1)->type == T_OP ||
                         Auto_find(&g_stack, g_pc.token->u.identifier) == 0)
                        && Global_find(&g_globals, g_pc.token->u.identifier,
                                       (g_pc.token + 1)->type == T_OP) == 0)
                      {
                        Value_new_ERROR(value, UNDECLARED);
                        goto error;
                      }
                  }

                if (g_pass != DECLARE &&
                    (g_pc.token->u.identifier->sym->type == GLOBALVAR ||
                     g_pc.token->u.identifier->sym->type == GLOBALARRAY ||
                     g_pc.token->u.identifier->sym->type == LOCALVAR))
                  {
                    struct Value *l;

                    if ((l = lvalue(value))->type == V_ERROR)
                      goto error;
                    Value_clone(&sp->u.value, l);
                  }
                else
                  {
                    struct Pc var = g_pc;

                    func(&sp->u.value);
                    if (sp->u.value.type == V_VOID)
                      {
                        g_pc = var;
                        Value_new_ERROR(value, VOIDVALUE);
                        goto error;
                      }
                  }
              }
            else if (ip == T_INTEGER)
              {
                /* printf("state %d: shift 4\n",sp->state);
                 * printf("state 4: reduce E -> value\n");
                 */

                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                VALUE_NEW_INTEGER(&sp->u.value, g_pc.token->u.integer);
                ++g_pc.token;
              }
            else if (ip == T_REAL)
              {
                /* printf("state %d: shift 4\n",sp->state);
                 * printf("state 4: reduce E -> value\n");
                 */

                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                VALUE_NEW_REAL(&sp->u.value, g_pc.token->u.real);
                ++g_pc.token;
              }
            else if (TOKEN_ISUNARYOPERATOR(ip))
              {
                /* printf("state %d: shift 2\n",sp->state); */

                ++sp;
                sp->state = 2;
                sp->u.token = ip;
                ++g_pc.token;
              }
            else if (ip == T_HEXINTEGER)
              {
                /* printf("state %d: shift 4\n",sp->state);
                 * printf("state 4: reduce E -> value\n");
                 */

                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                VALUE_NEW_INTEGER(&sp->u.value, g_pc.token->u.hexinteger);
                ++g_pc.token;
              }
            else if (ip == T_OCTINTEGER)
              {
                /* printf("state %d: shift 4\n",sp->state);
                 * printf("state 4: reduce E -> value\n");
                 */

                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                VALUE_NEW_INTEGER(&sp->u.value, g_pc.token->u.octinteger);
                ++g_pc.token;
              }
            else if (ip == T_OP)
              {
                /* printf("state %d: shift 3\n",sp->state); */

                ++sp;
                sp->state = 3;
                sp->u.token = T_OP;
                ++g_pc.token;
              }
            else if (ip == T_STRING)
              {
                /* printf("state %d: shift 4\n",sp->state);
                 * printf("state 4: reduce E -> value\n");
                 */

                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                Value_new_STRING(&sp->u.value);
                String_destroy(&sp->u.value.u.string);
                String_clone(&sp->u.value.u.string, g_pc.token->u.string);
                ++g_pc.token;
              }
            else
              {
                char state = sp->state;

                if (state == 0)
                  {
                    if (desc)
                      {
                        Value_new_ERROR(value, MISSINGEXPR, desc);
                      }
                    else
                      {
                        value = (struct Value *)0;
                      }
                  }
                else
                  {
                    Value_new_ERROR(value, MISSINGEXPR, _("operand"));
                  }

                goto error;
              }

            break;
          }

        case 5:
          {
            if (TOKEN_ISBINARYOPERATOR(ip))
              {
                /* printf("state %d: shift 1\n",sp->state); */

                ++sp;
                sp->state = 1;
                sp->u.token = ip;
                ++g_pc.token;
                break;
              }
            else
              {
                assert(sp == pdastack + 1);
                *value = sp->u.value;
                free(pdastack);
                return value;
              }

            break;
          }

        case 6:
          {
            if (TOKEN_ISBINARYOPERATOR(ip) &&
                TOKEN_UNARYPRIORITY((sp - 1)->u.token) <
                TOKEN_BINARYPRIORITY(ip))
              {
                assert(TOKEN_ISUNARYOPERATOR((sp - 1)->u.token));

                /* printf("state %d: shift 1 (not reducing E -> op E)\n",
                 *        sp->state);
                 */

                ++sp;
                sp->state = 1;
                sp->u.token = ip;
                ++g_pc.token;
              }
            else
              {
                enum TokenType op;

                /* printf("reduce E -> op E\n"); */

                --sp;
                op = sp->u.token;
                sp->u.value = (sp + 1)->u.value;
                switch (op)
                  {
                  case T_PLUS:
                    break;

                  case T_MINUS:
                    Value_uneg(&sp->u.value, g_pass == INTERPRET);
                    break;

                  case T_NOT:
                    Value_unot(&sp->u.value, g_pass == INTERPRET);
                    break;

                  default:
                    assert(0);
                  }

                sp->state = gotoState[(sp - 1)->state];
                if (sp->u.value.type == V_ERROR)
                  {
                    *value = sp->u.value;
                    --sp;
                    goto error;
                  }
              }

            break;
          }

        case 7:                /* including 9 */
          {
            if (TOKEN_ISBINARYOPERATOR(ip))
              {
                /* printf("state %d: shift 1\n"sp->state); */

                ++sp;
                sp->state = 1;
                sp->u.token = ip;
                ++g_pc.token;
              }
            else if (ip == T_CP)
              {
                /* printf("state %d: shift 9\n",sp->state);
                 * printf("state 9: reduce E -> ( E )\n");
                 */

                --sp;
                sp->state = gotoState[(sp - 1)->state];
                sp->u.value = (sp + 1)->u.value;
                ++g_pc.token;
              }
            else
              {
                Value_new_ERROR(value, MISSINGCP);
                goto error;
              }

            break;
          }

        case 8:
          {
            int p1, p2;

            if (TOKEN_ISBINARYOPERATOR(ip) &&
                (((p1 = TOKEN_BINARYPRIORITY((sp - 1)->u.token)) <
                  (p2 = TOKEN_BINARYPRIORITY(ip))) ||
                 (p1 == p2 && TOKEN_ISRIGHTASSOCIATIVE((sp - 1)->u.token))))
              {
                /* printf("state %d: shift 1\n",sp->state); */

                ++sp;
                sp->state = 1;
                sp->u.token = ip;
                ++g_pc.token;
              }
            else
              {
                /* printf("state %d: reduce E -> E op E\n",sp->state); */

                if (Value_commonType[(sp - 2)->u.value.type]
                    [sp->u.value.type] == V_ERROR)
                  {
                    Value_destroy(&sp->u.value);
                    sp -= 2;
                    Value_destroy(&sp->u.value);
                    Value_new_ERROR(value, INVALIDOPERAND);
                    --sp;
                    goto error;
                  }
                else
                  {
                    switch ((sp - 1)->u.token)
                      {
                      case T_LT:
                        Value_lt(&(sp - 2)->u.value, &sp->u.value,
                                 g_pass == INTERPRET);
                        break;

                      case T_LE:
                        Value_le(&(sp - 2)->u.value, &sp->u.value,
                                 g_pass == INTERPRET);
                        break;

                      case T_EQ:
                        Value_eq(&(sp - 2)->u.value, &sp->u.value,
                                 g_pass == INTERPRET);
                        break;

                      case T_GE:
                        Value_ge(&(sp - 2)->u.value, &sp->u.value,
                                 g_pass == INTERPRET);
                        break;

                      case T_GT:
                        Value_gt(&(sp - 2)->u.value, &sp->u.value,
                                 g_pass == INTERPRET);
                        break;

                      case T_NE:
                        Value_ne(&(sp - 2)->u.value, &sp->u.value,
                                 g_pass == INTERPRET);
                        break;

                      case T_PLUS:
                        Value_add(&(sp - 2)->u.value, &sp->u.value,
                                  g_pass == INTERPRET);
                        break;
                      case T_MINUS:
                        Value_sub(&(sp - 2)->u.value, &sp->u.value,
                                  g_pass == INTERPRET);
                        break;

                      case T_MULT:
                        Value_mult(&(sp - 2)->u.value, &sp->u.value,
                                   g_pass == INTERPRET);
                        break;

                      case T_DIV:
                        Value_div(&(sp - 2)->u.value, &sp->u.value,
                                  g_pass == INTERPRET);
                        break;

                      case T_IDIV:
                        Value_idiv(&(sp - 2)->u.value, &sp->u.value,
                                   g_pass == INTERPRET);
                        break;

                      case T_MOD:
                        Value_mod(&(sp - 2)->u.value, &sp->u.value,
                                  g_pass == INTERPRET);
                        break;

                      case T_POW:
                        Value_pow(&(sp - 2)->u.value, &sp->u.value,
                                  g_pass == INTERPRET);
                        break;

                      case T_AND:
                        Value_and(&(sp - 2)->u.value, &sp->u.value,
                                  g_pass == INTERPRET);
                        break;

                      case T_OR:
                        Value_or(&(sp - 2)->u.value, &sp->u.value,
                                 g_pass == INTERPRET);
                        break;

                      case T_XOR:
                        Value_xor(&(sp - 2)->u.value, &sp->u.value,
                                  g_pass == INTERPRET);
                        break;

                      case T_EQV:
                        Value_eqv(&(sp - 2)->u.value, &sp->u.value,
                                  g_pass == INTERPRET);
                        break;

                      case T_IMP:
                        Value_imp(&(sp - 2)->u.value, &sp->u.value,
                                  g_pass == INTERPRET);
                        break;

                      default:
                        assert(0);
                      }
                  }

                Value_destroy(&sp->u.value);
                sp -= 2;
                sp->state = gotoState[(sp - 1)->state];
                if (sp->u.value.type == V_ERROR)
                  {
                    *value = sp->u.value;
                    --sp;
                    goto error;
                  }
              }

            break;
          }
        }
    }

error:
  while (sp > pdastack)
    {
      switch (sp->state)
        {
        case 5:
        case 6:
        case 7:
        case 8:
          Value_destroy(&sp->u.value);
        }

      --sp;
    }

  free(pdastack);
  return value;
}

#else
static inline struct Value *binarydown(struct Value *value,
                                       struct Value *(level) (struct Value *
                                                              value),
                                       const int prio)
{
  enum TokenType op;
  struct Pc oppc;

  if (level(value) == (struct Value *)0)
    {
      return (struct Value *)0;
    }

  if (value->type == V_ERROR)
    {
      return value;
    }

  do
    {
      struct Value x;

      op = g_pc.token->type;
      if (!TOKEN_ISBINARYOPERATOR(op) || TOKEN_BINARYPRIORITY(op) != prio)
        {
          return value;
        }

      oppc = g_pc;
      ++g_pc.token;
      if (level(&x) == (struct Value *)0)
        {
          Value_destroy(value);
          return Value_new_ERROR(value, MISSINGEXPR, _("binary operand"));
        }

      if (x.type == V_ERROR)
        {
          Value_destroy(value);
          *value = x;
          return value;
        }

      if (Value_commonType[value->type][x.type] == V_ERROR)
        {
          Value_destroy(value);
          Value_destroy(&x);
          return Value_new_ERROR(value, INVALIDOPERAND);
        }
      else
        {
          switch (op)
            {
            case T_LT:
              Value_lt(value, &x, g_pass == INTERPRET);
              break;

            case T_LE:
              Value_le(value, &x, g_pass == INTERPRET);
              break;

            case T_EQ:
              Value_eq(value, &x, g_pass == INTERPRET);
              break;

            case T_GE:
              Value_ge(value, &x, g_pass == INTERPRET);
              break;

            case T_GT:
              Value_gt(value, &x, g_pass == INTERPRET);
              break;

            case T_NE:
              Value_ne(value, &x, g_pass == INTERPRET);
              break;

            case T_PLUS:
              Value_add(value, &x, g_pass == INTERPRET);
              break;

            case T_MINUS:
              Value_sub(value, &x, g_pass == INTERPRET);
              break;

            case T_MULT:
              Value_mult(value, &x, g_pass == INTERPRET);
              break;

            case T_DIV:
              Value_div(value, &x, g_pass == INTERPRET);
              break;

            case T_IDIV:
              Value_idiv(value, &x, g_pass == INTERPRET);
              break;

            case T_MOD:
              Value_mod(value, &x, g_pass == INTERPRET);
              break;

            case T_POW:
              Value_pow(value, &x, g_pass == INTERPRET);
              break;

            case T_AND:
              Value_and(value, &x, g_pass == INTERPRET);
              break;

            case T_OR:
              Value_or(value, &x, g_pass == INTERPRET);
              break;

            case T_XOR:
              Value_xor(value, &x, g_pass == INTERPRET);
              break;

            case T_EQV:
              Value_eqv(value, &x, g_pass == INTERPRET);
              break;

            case T_IMP:
              Value_imp(value, &x, g_pass == INTERPRET);
              break;

            default:
              assert(0);
            }
        }

      Value_destroy(&x);
    }
  while (value->type != V_ERROR);

  if (value->type == V_ERROR)
    {
      g_pc = oppc;
    }

  return value;
}

static inline struct Value *unarydown(struct Value *value,
                                      struct Value *(level) (struct Value *
                                                             value),
                                      const int prio)
{
  enum TokenType op;
  struct Pc oppc;

  op = g_pc.token->type;
  if (!TOKEN_ISUNARYOPERATOR(op) || TOKEN_UNARYPRIORITY(op) != prio)
    {
      return level(value);
    }

  oppc = g_pc;
  ++g_pc.token;
  if (unarydown(value, level, prio) == (struct Value *)0)
    {
      return Value_new_ERROR(value, MISSINGEXPR, _("unary operand"));
    }

  if (value->type == V_ERROR)
    {
      return value;
    }

  switch (op)
    {
    case T_PLUS:
      Value_uplus(value, g_pass == INTERPRET);
      break;

    case T_MINUS:
      Value_uneg(value, g_pass == INTERPRET);
      break;

    case T_NOT:
      Value_unot(value, g_pass == INTERPRET);
      break;

    default:
      assert(0);
    }

  if (value->type == V_ERROR)
    {
      g_pc = oppc;
    }

  return value;
}

static struct Value *eval8(struct Value *value)
{
  switch (g_pc.token->type)
    {
    case T_IDENTIFIER:
      {
        struct Pc var;
        struct Value *l;

        var = g_pc;
        if (g_pass == COMPILE)
          {
            if (((g_pc.token + 1)->type == T_OP ||
                 Auto_find(&g_stack, g_pc.token->u.identifier) == 0) &&
                Global_find(&g_globals, g_pc.token->u.identifier,
                            (g_pc.token + 1)->type == T_OP) == 0)
              return Value_new_ERROR(value, UNDECLARED);
          }

        assert(g_pass == DECLARE || g_pc.token->u.identifier->sym);
        if (g_pass != DECLARE &&
            (g_pc.token->u.identifier->sym->type == GLOBALVAR ||
             g_pc.token->u.identifier->sym->type == GLOBALARRAY ||
             g_pc.token->u.identifier->sym->type == LOCALVAR))
          {
            if ((l = lvalue(value))->type == V_ERROR)
              {
                return value;
              }

            Value_clone(value, l);
          }
        else
          {
            func(value);
            if (value->type == V_VOID)
              {
                Value_destroy(value);
                g_pc = var;
                return Value_new_ERROR(value, VOIDVALUE);
              }
          }

        break;
      }

    case T_INTEGER:
      {
        VALUE_NEW_INTEGER(value, g_pc.token->u.integer);
        ++g_pc.token;
        break;
      }

    case T_REAL:
      {
        VALUE_NEW_REAL(value, g_pc.token->u.real);
        ++g_pc.token;
        break;
      }

    case T_STRING:
      {
        Value_new_STRING(value);
        String_destroy(&value->u.string);
        String_clone(&value->u.string, g_pc.token->u.string);
        ++g_pc.token;
        break;
      }

    case T_HEXINTEGER:
      {
        VALUE_NEW_INTEGER(value, g_pc.token->u.hexinteger);
        ++g_pc.token;
        break;
      }

    case T_OCTINTEGER:
      {
        VALUE_NEW_INTEGER(value, g_pc.token->u.octinteger);
        ++g_pc.token;
        break;
      }

    case T_OP:
      {
        ++g_pc.token;
        if (eval(value, _("parenthetic"))->type == V_ERROR)
          {
            return value;
          }

        if (g_pc.token->type != T_CP)
          {
            Value_destroy(value);
            return Value_new_ERROR(value, MISSINGCP);
          }

        ++g_pc.token;
        break;
      }

    default:
      {
        return (struct Value *)0;
      }
    }

  return value;
}

static struct Value *eval7(struct Value *value)
{
  return binarydown(value, eval8, 7);
}

static struct Value *eval6(struct Value *value)
{
  return unarydown(value, eval7, 6);
}

static struct Value *eval5(struct Value *value)
{
  return binarydown(value, eval6, 5);
}

static struct Value *eval4(struct Value *value)
{
  return binarydown(value, eval5, 4);
}

static struct Value *eval3(struct Value *value)
{
  return binarydown(value, eval4, 3);
}

static struct Value *eval2(struct Value *value)
{
  return unarydown(value, eval3, 2);
}

static struct Value *eval1(struct Value *value)
{
  return binarydown(value, eval2, 1);
}

static struct Value *eval(struct Value *value, const char *desc)
{
  /* Avoid function calls for atomic expression */

  switch (g_pc.token->type)
    {
    case T_STRING:
    case T_REAL:
    case T_INTEGER:
    case T_HEXINTEGER:
    case T_OCTINTEGER:
    case T_IDENTIFIER:
      if (!TOKEN_ISBINARYOPERATOR((g_pc.token + 1)->type) &&
          (g_pc.token + 1)->type != T_OP)
        {
          return eval7(value);
        }

    default:
      break;
    }

  if (binarydown(value, eval1, 0) == (struct Value *)0)
    {
      if (desc)
        {
          return Value_new_ERROR(value, MISSINGEXPR, desc);
        }
      else
        {
          return (struct Value *)0;
        }
    }
  else
    {
      return value;
    }
}
#endif

static void new(void)
{
  Global_destroy(&g_globals);
  Global_new(&g_globals);
  Auto_destroy(&g_stack);
  Auto_new(&g_stack);
  Program_destroy(&g_program);
  Program_new(&g_program);
  FS_closefiles();
  g_optionbase = 0;
}

static void pushLabel(enum labeltype_e type, struct Pc *patch)
{
  if (g_labelstack_index == g_labelstack_capacity)
    {
      struct labelstack_s *more;

      more =
        realloc(g_labelstack,
                sizeof(struct labelstack_s) *
                (g_labelstack_capacity ? g_labelstack_capacity *= 2 : 32));
      g_labelstack = more;
    }

  g_labelstack[g_labelstack_index].type = type;
  g_labelstack[g_labelstack_index].patch = *patch;
  ++g_labelstack_index;
}

static struct Pc *popLabel(enum labeltype_e type)
{
  if (g_labelstack_index == 0 ||
      g_labelstack[g_labelstack_index - 1].type != type)
    {
      return (struct Pc *)0;
    }
  else
    {
      return &g_labelstack[--g_labelstack_index].patch;
    }
}

static struct Pc *findLabel(enum labeltype_e type)
{
  int i;

  for (i = g_labelstack_index - 1; i >= 0; --i)
    {
      if (g_labelstack[i].type == type)
        {
          return &g_labelstack[i].patch;
        }
    }

  return (struct Pc *)0;
}

static void labelStackError(struct Value *v)
{
  assert(g_labelstack_index);
  g_pc = g_labelstack[g_labelstack_index - 1].patch;
  switch (g_labelstack[g_labelstack_index - 1].type)
    {
    case L_IF:
      Value_new_ERROR(v, STRAYIF);
      break;

    case L_DO:
      Value_new_ERROR(v, STRAYDO);
      break;

    case L_DOcondition:
      Value_new_ERROR(v, STRAYDOcondition);
      break;

    case L_ELSE:
      Value_new_ERROR(v, STRAYELSE2);
      break;

    case L_FOR_BODY:
      {
        Value_new_ERROR(v, STRAYFOR);
        g_pc = *findLabel(L_FOR);
        break;
      }

    case L_WHILE:
      Value_new_ERROR(v, STRAYWHILE);
      break;

    case L_REPEAT:
      Value_new_ERROR(v, STRAYREPEAT);
      break;

    case L_SELECTCASE:
      Value_new_ERROR(v, STRAYSELECTCASE);
      break;

    case L_FUNC:
      Value_new_ERROR(v, STRAYFUNC);
      break;

    default:
      assert(0);
    }
}

static const char *topLabelDescription(void)
{
  if (g_labelstack_index == 0)
    {
      return _("program");
    }

  switch (g_labelstack[g_labelstack_index - 1].type)
    {
    case L_IF:
      return _("`if' branch");

    case L_DO:
      return _("`do' loop");

    case L_DOcondition:
      return _("`do while' or `do until' loop");

    case L_ELSE:
      return _("`else' branch");

    case L_FOR_BODY:
      return _("`for' loop");

    case L_WHILE:
      return _("`while' loop");

    case L_REPEAT:
      return _("`repeat' loop");

    case L_SELECTCASE:
      return _("`select case' control structure");

    case L_FUNC:
      return _("function or procedure");

    default:
      assert(0);
    }

  /* NOTREACHED */

  return (const char *)0;
}

static struct Value *assign(struct Value *value)
{
  struct Pc expr;

  if (strcasecmp(g_pc.token->u.identifier->name, "mid$") == 0)
    {
      long int n, m;
      struct Value *l;

      ++g_pc.token;
      if (g_pc.token->type != T_OP)
        {
          return Value_new_ERROR(value, MISSINGOP);
        }

      ++g_pc.token;
      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGSTRIDENT);
        }

      if (g_pass == DECLARE)
        {
          if (((g_pc.token + 1)->type == T_OP ||
               Auto_find(&g_stack, g_pc.token->u.identifier) == 0) &&
              Global_variable(&g_globals, g_pc.token->u.identifier,
                              g_pc.token->u.identifier->defaultType,
                              (g_pc.token + 1)->type ==
                              T_OP ? GLOBALARRAY : GLOBALVAR, 0) == 0)
            {
              return Value_new_ERROR(value, REDECLARATION);
            }
        }

      if ((l = lvalue(value))->type == V_ERROR)
        {
          return value;
        }

      if (g_pass == COMPILE && l->type != V_STRING)
        {
          return Value_new_ERROR(value, TYPEMISMATCH4);
        }

      if (g_pc.token->type != T_COMMA)
        {
          return Value_new_ERROR(value, MISSINGCOMMA);
        }

      ++g_pc.token;
      if (eval(value, _("position"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      n = value->u.integer;
      Value_destroy(value);
      if (g_pass == INTERPRET && n < 1)
        {
          return Value_new_ERROR(value, OUTOFRANGE, "position");
        }

      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
          if (eval(value, _("length"))->type == V_ERROR ||
              Value_retype(value, V_INTEGER)->type == V_ERROR)
            {
              return value;
            }

          m = value->u.integer;
          if (g_pass == INTERPRET && m < 0)
            {
              return Value_new_ERROR(value, OUTOFRANGE, _("length"));
            }

          Value_destroy(value);
        }
      else
        {
          m = -1;
        }

      if (g_pc.token->type != T_CP)
        {
          return Value_new_ERROR(value, MISSINGCP);
        }

      ++g_pc.token;
      if (g_pc.token->type != T_EQ)
        {
          return Value_new_ERROR(value, MISSINGEQ);
        }

      ++g_pc.token;
      if (eval(value, _("rhs"))->type == V_ERROR ||
          Value_retype(value, V_STRING)->type == V_ERROR)
        {
          return value;
        }

      if (g_pass == INTERPRET)
        {
          if (m == -1)
            {
              m = value->u.string.length;
            }

          String_set(&l->u.string, n - 1, &value->u.string, m);
        }
    }
  else
    {
      struct Value **l = (struct Value **)0;
      int i, used = 0, capacity = 0;
      struct Value retyped_value;

      for (; ; )
        {
          if (used == capacity)
            {
              struct Value **more;

              capacity = capacity ? 2 * capacity : 2;
              more = realloc(l, capacity * sizeof(*l));
              l = more;
            }

          if (g_pass == DECLARE)
            {
              if (((g_pc.token + 1)->type == T_OP ||
                   Auto_find(&g_stack, g_pc.token->u.identifier) == 0) &&
                  Global_variable(&g_globals, g_pc.token->u.identifier,
                                  g_pc.token->u.identifier->defaultType,
                                  (g_pc.token + 1)->type ==
                                  T_OP ? GLOBALARRAY : GLOBALVAR, 0) == 0)
                {
                  if (capacity)
                    {
                      free(l);
                    }

                  return Value_new_ERROR(value, REDECLARATION);
                }
            }

          if ((l[used] = lvalue(value))->type == V_ERROR)
            {
              return value;
            }

          ++used;
          if (g_pc.token->type == T_COMMA)
            {
              ++g_pc.token;
            }
          else
            {
              break;
            }
        }

      if (g_pc.token->type != T_EQ)
        {
          return Value_new_ERROR(value, MISSINGEQ);
        }

      ++g_pc.token;
      expr = g_pc;
      if (eval(value, _("rhs"))->type == V_ERROR)
        {
          return value;
        }

      for (i = 0; i < used; ++i)
        {
          Value_clone(&retyped_value, value);
          if (g_pass != DECLARE &&
              VALUE_RETYPE(&retyped_value, (l[i])->type)->type == V_ERROR)
            {
              g_pc = expr;
              free(l);
              Value_destroy(value);
              *value = retyped_value;
              return value;
            }

          if (g_pass == INTERPRET)
            {
              Value_destroy(l[i]);
              *(l[i]) = retyped_value;
            }
        }

      free(l);
      Value_destroy(value);
      *value = retyped_value;   /* for status only */
    }

  return value;
}

static struct Value *compileProgram(struct Value *v, int clearGlobals)
{
  struct Pc begin;

  g_stack.resumeable = 0;
  if (clearGlobals)
    {
      Global_destroy(&g_globals);
      Global_new(&g_globals);
    }
  else
    {
      Global_clearFunctions(&g_globals);
    }

  if (Program_beginning(&g_program, &begin))
    {
      struct Pc savepc;
      int savepass;

      savepc = g_pc;
      savepass = g_pass;
      Program_norun(&g_program);
      for (g_pass = DECLARE; g_pass != INTERPRET; ++g_pass)
        {
          if (g_pass == DECLARE)
            {
              g_stack.begindata.line = -1;
              g_lastdata = &g_stack.begindata;
            }

          g_optionbase = 0;
          g_stopped = 0;
          g_program.runnable = 1;
          g_pc = begin;
          while (1)
            {
              statements(v);
              if (v->type == V_ERROR)
                {
                  break;
                }

              Value_destroy(v);
              if (!Program_skipEOL(&g_program, &g_pc, 0, 0))
                {
                  Value_new_NIL(v);
                  break;
                }
            }

          if (v->type != V_ERROR && g_labelstack_index > 0)
            {
              Value_destroy(v);
              labelStackError(v);
            }

          if (v->type == V_ERROR)
            {
              g_labelstack_index = 0;
              Program_norun(&g_program);
              if (g_stack.cur)
                {
                  Auto_funcEnd(&g_stack); /* Always correct? */
                }

              g_pass = savepass;
              return v;
            }
        }

      g_pc = begin;
      if (Program_analyse(&g_program, &g_pc, v))
        {
          g_labelstack_index = 0;
          Program_norun(&g_program);
          if (g_stack.cur)
            {
              Auto_funcEnd(&g_stack);     /* Always correct? */
            }

          g_pass = savepass;
          return v;
        }

      g_curdata = g_stack.begindata;
      g_pc = savepc;
      g_pass = savepass;
    }

  return Value_new_NIL(v);
}

static void runline(struct Token *line)
{
  struct Value value;

  FS_flush(STDCHANNEL);
  for (g_pass = DECLARE; g_pass != INTERPRET; ++g_pass)
    {
      g_curdata.line = -1;
      g_pc.line = -1;
      g_pc.token = line;
      g_optionbase = 0;
      g_stopped = 0;
      statements(&value);
      if (value.type != V_ERROR && g_pc.token->type != T_EOL)
        {
          Value_destroy(&value);
          Value_new_ERROR(&value, SYNTAX);
        }

      if (value.type != V_ERROR && g_labelstack_index > 0)
        {
          Value_destroy(&value);
          labelStackError(&value);
        }

      if (value.type == V_ERROR)
        {
          struct String s;

          Auto_setError(&g_stack, Program_lineNumber(&g_program, &g_pc),
                        &g_pc, &value);
          Program_PCtoError(&g_program, &g_pc, &value);
          g_labelstack_index = 0;
          FS_putChars(STDCHANNEL, _("Error: "));
          String_new(&s);
          Value_toString(&value, &s, ' ', -1, 0, 0, 0, 0, -1, 0, 0);
          Value_destroy(&value);
          FS_putString(STDCHANNEL, &s);
          String_destroy(&s);
          return;
        }

      if (!g_program.runnable && g_pass == COMPILE)
        {
          Value_destroy(&value);
          compileProgram(&value, 0);
        }
    }

  g_pc.line = -1;
  g_pc.token = line;
  g_optionbase = 0;
  g_curdata = g_stack.begindata;
  g_nextdata.line = -1;
  Value_destroy(&value);
  g_pass = INTERPRET;

  do
    {
      assert(g_pass == INTERPRET);
      statements(&value);
      assert(g_pass == INTERPRET);
      if (value.type == V_ERROR)
        {
          if (strchr(value.u.error.msg, '\n') == (char *)0)
            {
              Auto_setError(&g_stack, Program_lineNumber(&g_program, &g_pc),
                            &g_pc, &value);
              Program_PCtoError(&g_program, &g_pc, &value);
            }

          if (g_stack.onerror.line != -1)
            {
              g_stack.resumeable = 1;
              g_pc = g_stack.onerror;
            }
          else
            {
              struct String s;

              String_new(&s);
              if (!g_stopped)
                {
                  g_stopped = 0;
                  FS_putChars(STDCHANNEL, _("Error: "));
                }

              Auto_frameToError(&g_stack, &g_program, &value);
              Value_toString(&value, &s, ' ', -1, 0, 0, 0, 0, -1, 0, 0);
              while (Auto_gosubReturn(&g_stack, (struct Pc *)0));
              FS_putString(STDCHANNEL, &s);
              String_destroy(&s);
              Value_destroy(&value);
              break;
            }
        }

      Value_destroy(&value);
    }
  while (g_pc.token->type != T_EOL ||
         Program_skipEOL(&g_program, &g_pc, STDCHANNEL, 1));
}

static struct Value *evalGeometry(struct Value *value, unsigned int *dim,
                                  unsigned int geometry[])
{
  struct Pc exprpc = g_pc;

  if (eval(value, _("dimension"))->type == V_ERROR ||
      (g_pass != DECLARE && Value_retype(value, V_INTEGER)->type == V_ERROR))
    {
      return value;
    }

  if (g_pass == INTERPRET && value->u.integer < g_optionbase)
    {
      Value_destroy(value);
      g_pc = exprpc;
      return Value_new_ERROR(value, OUTOFRANGE, _("dimension"));
    }

  geometry[0] = value->u.integer - g_optionbase + 1;
  Value_destroy(value);
  if (g_pc.token->type == T_COMMA)
    {
      ++g_pc.token;
      exprpc = g_pc;
      if (eval(value, _("dimension"))->type == V_ERROR || (g_pass != DECLARE
          && Value_retype(value, V_INTEGER)->type == V_ERROR))
        {
          return value;
        }

      if (g_pass == INTERPRET && value->u.integer < g_optionbase)
        {
          Value_destroy(value);
          g_pc = exprpc;
          return Value_new_ERROR(value, OUTOFRANGE, _("dimension"));
        }

      geometry[1] = value->u.integer - g_optionbase + 1;
      Value_destroy(value);
      *dim = 2;
    }
  else
    {
      *dim = 1;
    }

  if (g_pc.token->type == T_CP)
    {
      ++g_pc.token;
    }
  else
    {
      return Value_new_ERROR(value, MISSINGCP);
    }

  return (struct Value *)0;
}

static struct Value *convert(struct Value *value, struct Value *l,
                             struct Token *t)
{
  switch (l->type)
    {
    case V_INTEGER:
      {
        char *datainput;
        char *end;
        long int v;
        int overflow;

        if (t->type != T_DATAINPUT)
          {
            return Value_new_ERROR(value, BADCONVERSION, _("integer"));
          }

        datainput = t->u.datainput;
        v = Value_vali(datainput, &end, &overflow);
        if (end == datainput ||
            (*end != '\0' && *end != ' ' && *end != '\t'))
          {
            return Value_new_ERROR(value, BADCONVERSION, _("integer"));
          }

        if (overflow)
          {
            return Value_new_ERROR(value, OUTOFRANGE, _("converted value"));
          }

        Value_destroy(l);
        VALUE_NEW_INTEGER(l, v);
        break;
      }

    case V_REAL:
      {
        char *datainput;
        char *end;
        double v;
        int overflow;

        if (t->type != T_DATAINPUT)
          {
            return Value_new_ERROR(value, BADCONVERSION, _("real"));
          }

        datainput = t->u.datainput;
        v = Value_vald(datainput, &end, &overflow);
        if (end == datainput ||
            (*end != '\0' && *end != ' ' && *end != '\t'))
          {
            return Value_new_ERROR(value, BADCONVERSION, _("real"));
          }

        if (overflow)
          {
            return Value_new_ERROR(value, OUTOFRANGE, _("converted value"));
          }

        Value_destroy(l);
        VALUE_NEW_REAL(l, v);
        break;
      }

    case V_STRING:
      {
        Value_destroy(l);
        Value_new_STRING(l);
        if (t->type == T_STRING)
          {
            String_appendString(&l->u.string, t->u.string);
          }
        else
          {
            String_appendChars(&l->u.string, t->u.datainput);
          }

        break;
      }

    default:
      assert(0);
    }

  return (struct Value *)0;
}

static struct Value *dataread(struct Value *value, struct Value *l)
{
  if (g_curdata.line == -1)
    {
      return Value_new_ERROR(value, ENDOFDATA);
    }

  if (g_curdata.token->type == T_DATA)
    {
      g_nextdata = g_curdata.token->u.nextdata;
      ++g_curdata.token;
    }

  if (convert(value, l, g_curdata.token))
    {
      return value;
    }

  ++g_curdata.token;
  if (g_curdata.token->type == T_COMMA)
    {
      ++g_curdata.token;
    }
  else
    {
      g_curdata = g_nextdata;
    }

  return (struct Value *)0;
}

static struct Value more_statements;
#include "bas_statement.c"
static struct Value *statements(struct Value *value)
{
more:
  if (g_pc.token->statement)
    {
      struct Value *v;

      if ((v = g_pc.token->statement(value)))
        {
          if (v == &more_statements)
            {
              goto more;
            }
          else
            {
              return value;
            }
        }
    }
  else
    {
      return Value_new_ERROR(value, MISSINGSTATEMENT);
    }

  if (g_pc.token->type == T_COLON && (g_pc.token + 1)->type == T_ELSE)
    {
      ++g_pc.token;
    }
  else if ((g_pc.token->type == T_COLON && (g_pc.token + 1)->type != T_ELSE)
           || g_pc.token->type == T_QUOTE)
    {
      ++g_pc.token;
      goto more;
    }
  else if ((g_pass == DECLARE || g_pass == COMPILE) &&
           g_pc.token->type != T_EOL && g_pc.token->type != T_ELSE)
    {
      return Value_new_ERROR(value, MISSINGCOLON);
    }

  return Value_new_NIL(value);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bas_init(int backslash_colon, int restricted, int uppercase, int lpfd)
{
  g_stack.begindata.line = -1;
  Token_init(backslash_colon, uppercase);
  Global_new(&g_globals);
  Auto_new(&g_stack);
  Program_new(&g_program);
  FS_opendev(STDCHANNEL, 0, 1);
  FS_opendev(LPCHANNEL, -1, lpfd);
  g_run_restricted = restricted;
}

void bas_runFile(const char *runFile)
{
  struct Value value;
  int dev;

  new();
  if ((dev = FS_openin(runFile)) == -1)
    {
      const char *errmsg = FS_errmsg;

      FS_putChars(0, _("bas: Executing `"));
      FS_putChars(0, runFile);
      FS_putChars(0, _("' failed ("));
      FS_putChars(0, errmsg);
      FS_putChars(0, _(").\n"));
    }
  else if (Program_merge(&g_program, dev, &value))
    {
      struct String s;

      FS_putChars(0, "bas: ");
      String_new(&s);
      Value_toString(&value, &s, ' ', -1, 0, 0, 0, 0, -1, 0, 0);
      FS_putString(0, &s);
      String_destroy(&s);
      FS_putChar(0, '\n');
      Value_destroy(&value);
    }
  else
    {
      struct Token line[2];

      Program_setname(&g_program, runFile);
      line[0].type = T_RUN;
      line[0].statement = stmt_RUN;
      line[1].type = T_EOL;
      line[1].statement = stmt_COLON_EOL;

      FS_close(dev);
      runline(line);
    }
}

void bas_runLine(const char *runLine)
{
  struct Token *line;

  line = Token_newCode(runLine);
  runline(line + 1);
  Token_destroy(line);
}

void bas_interpreter(void)
{
  if (FS_istty(STDCHANNEL))
    {
      FS_putChars(STDCHANNEL, "bas " CONFIG_INTERPRETER_BAS_VERSION "\n");
      FS_putChars(STDCHANNEL, "Copyright 1999-2014 Michael Haardt.\n");
      FS_putChars(STDCHANNEL,
                  "This is free software with ABSOLUTELY NO WARRANTY.\n");
    }

  new();
  while (1)
    {
      struct Token *line;
      struct String s;

      g_stopped = 0;
      FS_nextline(STDCHANNEL);
      if (FS_istty(STDCHANNEL))
        {
          FS_putChars(STDCHANNEL, "> ");
        }

      FS_flush(STDCHANNEL);
      String_new(&s);
      if (FS_appendToString(STDCHANNEL, &s, 1) == -1)
        {
          FS_putChars(STDCHANNEL, FS_errmsg);
          FS_flush(STDCHANNEL);
          String_destroy(&s);
          break;
        }

      if (s.length == 0)
        {
          String_destroy(&s);
          break;
        }

      line = Token_newCode(s.character);
      String_destroy(&s);
      if (line->type != T_EOL)
        {
          if (line->type == T_INTEGER && line->u.integer > 0)
            {
              if (g_program.numbered)
                {
                  if ((line + 1)->type == T_EOL)
                    {
                      struct Pc where;

                      if (Program_goLine(&g_program, line->u.integer, &where)
                          == (struct Pc *)0)
                        {
                          FS_putChars(STDCHANNEL, _("No such line\n"));
                        }
                      else
                        {
                          Program_delete(&g_program, &where, &where);
                        }

                      Token_destroy(line);
                    }
                  else
                    {
                      Program_store(&g_program, line, line->u.integer);
                    }
                }
              else
                {
                  FS_putChars(STDCHANNEL,
                              _("Use `renum' to number program first"));
                  Token_destroy(line);
                }
            }
          else if (line->type == T_UNNUMBERED)
            {
              runline(line + 1);
              Token_destroy(line);
              if (FS_istty(STDCHANNEL) && g_bas_end)
                {
                  FS_putChars(STDCHANNEL, _("END program\n"));
                  g_bas_end = false;
                }
            }
          else
            {
              FS_putChars(STDCHANNEL, _("Invalid line\n"));
              Token_destroy(line);
            }
        }
      else
        {
          Token_destroy(line);
        }
    }
}

void bas_exit(void)
{
  /* Release resources */

  Auto_destroy(&g_stack);
  Global_destroy(&g_globals);
  Program_destroy(&g_program);
  if (g_labelstack)
    {
      free(g_labelstack);
      g_labelstack = (struct labelstack_s *)0;
    }

  /* Close files and devices.  NOTE that STDCHANNEL is also close here and
   * can no longer be use
   */

  FS_closefiles();
  FS_close(LPCHANNEL);
  FS_close(STDCHANNEL);
}
