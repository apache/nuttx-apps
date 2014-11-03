/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_GETTEXT
#  include <libintl.h>
#  define _(String) gettext(String)
#else
#  define _(String) String
#endif
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "getopt.h"

#include "auto.h"
#include "bas.h"
#include "error.h"
#include "fs.h"
#include "global.h"
#include "program.h"
#include "value.h"
#include "var.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DIRECTMODE (pc.line== -1)
#ifndef __GNUC__
#  define inline
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum LabelType
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

struct LabelStack
  {
    enum LabelType type;
    struct Pc patch;
  };

/****************************************************************************
 * Private Data
 ****************************************************************************/

static unsigned int labelStackPointer, labelStackCapacity;
static struct LabelStack *labelStack;
static struct Pc *lastdata;
static struct Pc curdata;
static struct Pc nextdata;
static enum
  { DECLARE, COMPILE, INTERPRET } pass;
static int stopped;
static int optionbase;
static struct Pc pc;
static struct Auto stack;
static struct Program program;
static struct Global globals;
static int run_restricted;

int bas_argc;
char *bas_argv0;
char **bas_argv;
int bas_end;

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
  int err;

  if ((fd = open(filename, O_RDONLY)) == -1)
    return -1;
  while ((l = read(fd, buf, sizeof(buf))) > 0)
    {
      ssize_t off, w;

      off = 0;
      while (off < l)
        {
          if ((w = write(1, buf + off, l - off)) == -1)
            {
              err = errno;
              close(fd);
              errno = err;
              return -1;
            }
          off += w;
        }
    }
  if (l == -1)
    {
      err = errno;
      close(fd);
      errno = err;
      return -1;
    }
  close(fd);
  return 0;
}

static struct Value *lvalue(struct Value *value)
{
  struct Symbol *sym;
  struct Pc lvpc = pc;

  sym = pc.token->u.identifier->sym;
  assert(pass == DECLARE || sym->type == GLOBALVAR || sym->type == GLOBALARRAY
         || sym->type == LOCALVAR);
  if ((pc.token + 1)->type == T_OP)
    {
      struct Pc idxpc;
      unsigned int dim, capacity;
      int *idx;

      pc.token += 2;
      dim = 0;
      capacity = 0;
      idx = (int *)0;
      while (1)
        {
          if (dim == capacity && pass == INTERPRET)     /* enlarge idx */
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

          idxpc = pc;
          if (eval(value, _("index"))->type == V_ERROR ||
              VALUE_RETYPE(value, V_INTEGER)->type == V_ERROR)
            {
              if (capacity)
                free(idx);
              pc = idxpc;
              return value;
            }
          if (pass == INTERPRET)
            {
              idx[dim] = value->u.integer;
              ++dim;
            }
          Value_destroy(value);
          if (pc.token->type == T_COMMA)
            ++pc.token;
          else
            break;
        }
      if (pc.token->type != T_CP)
        {
          assert(pass != INTERPRET);
          return Value_new_ERROR(value, MISSINGCP);
        }
      else
        ++pc.token;
      switch (pass)
        {
        case INTERPRET:
          {
            if ((value =
                 Var_value(&(sym->u.var), dim, idx, value))->type == V_ERROR)
              pc = lvpc;
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
                                   GLOBALARRAY ? sym->u.var.
                                   type : Auto_varType(&stack, sym));
          }
        default:
          assert(0);
        }
      return (struct Value *)0;
    }
  else
    {
      ++pc.token;
      switch (pass)
        {
        case INTERPRET:
          return VAR_SCALAR_VALUE(sym->type ==
                                  GLOBALVAR ? &(sym->u.var) : Auto_local(&stack,
                                                                         sym->u.
                                                                         local.
                                                                         offset));
        case DECLARE:
          return Value_nullValue(V_INTEGER);
        case COMPILE:
          return Value_nullValue(sym->type ==
                                 GLOBALVAR ? sym->u.var.
                                 type : Auto_varType(&stack, sym));
        default:
          assert(0);
        }
      return (struct Value *)0;
    }
}

static struct Value *func(struct Value *value)
{
  struct Identifier *ident;
  struct Pc funcpc = pc;
  int firstslot = -99;
  int args = 0;
  struct Symbol *sym;

  assert(pc.token->type == T_IDENTIFIER);
  /*
   * Evaluating a function in direct mode may start a program, so it needs to
   * be compiled.  If in direct mode, programs will be compiled after the
   * direct mode pass DECLARE, but errors are ignored at that point, because
   * the program may not be needed.  If the program is fine, its symbols will
   * be available during the compile phase already.  If not and we need it at
   * this point, compile it again to get the error and abort. */
  if (DIRECTMODE && !program.runnable && pass != DECLARE)
    {
      if (compileProgram(value, 0)->type == V_ERROR)
        return value;
      Value_destroy(value);
    }
  ident = pc.token->u.identifier;
  assert(pass == DECLARE || ident->sym->type == BUILTINFUNCTION ||
         ident->sym->type == USERFUNCTION);
  ++pc.token;
  if (pass != DECLARE)
    {
      firstslot = stack.stackPointer;
      if (ident->sym->type == USERFUNCTION &&
          ident->sym->u.sub.retType != V_VOID)
        {
          struct Var *v = Auto_pushArg(&stack);
          Var_new(v, ident->sym->u.sub.retType, 0, (const unsigned int *)0, 0);
        }
    }
  if (pc.token->type == T_OP)   /* push arguments to stack */
    {
      ++pc.token;
      if (pc.token->type != T_CP)
        while (1)
          {
            if (pass == DECLARE)
              {
                if (eval(value, _("actual parameter"))->type == V_ERROR)
                  return value;
                Value_destroy(value);
              }
            else
              {
                struct Var *v = Auto_pushArg(&stack);

                Var_new_scalar(v);
                if (eval(v->value, (const char *)0)->type == V_ERROR)
                  {
                    Value_clone(value, v->value);
                    while (stack.stackPointer > firstslot)
                      Var_destroy(&stack.slot[--stack.stackPointer].var);
                    return value;
                  }
                v->type = v->value->type;
              }
            ++args;
            if (pc.token->type == T_COMMA)
              ++pc.token;
            else
              break;
          }
      if (pc.token->type != T_CP)
        {
          if (pass != DECLARE)
            {
              while (stack.stackPointer > firstslot)
                Var_destroy(&stack.slot[--stack.stackPointer].var);
            }
          return Value_new_ERROR(value, MISSINGCP);
        }
      ++pc.token;
    }

  if (pass == DECLARE)
    Value_new_null(value, ident->defaultType);
  else
    {
      int i;
      int nomore;
      int argerr;
      int overloaded;

      if (pass == INTERPRET && ident->sym->type == USERFUNCTION)
        {
          for (i = 0; i < ident->sym->u.sub.u.def.localLength; ++i)
            {
              struct Var *v = Auto_pushArg(&stack);
              Var_new(v, ident->sym->u.sub.u.def.localTypes[i], 0,
                      (const unsigned int *)0, 0);
            }
        }
      Auto_pushFuncRet(&stack, firstslot, &pc);

      sym = ident->sym;
      overloaded = (pass == COMPILE && sym->type == BUILTINFUNCTION &&
                    sym->u.sub.u.bltin.next);
      do
        {
          nomore = (pass == COMPILE &&
                    !(sym->type == BUILTINFUNCTION && sym->u.sub.u.bltin.next));
          argerr = 0;
          if (args < sym->u.sub.argLength)
            {
              if (nomore)
                Value_new_ERROR(value, TOOFEW);
              argerr = 1;
            }

          else if (args > sym->u.sub.argLength)
            {
              if (nomore)
                Value_new_ERROR(value, TOOMANY);
              argerr = 1;
            }

          else
            {
              for (i = 0; i < args; ++i)
                {
                  struct Value *arg =
                    Var_value(Auto_local(&stack, i), 0, (int *)0, value);

                  assert(arg->type != V_ERROR);
                  if (overloaded)
                    {
                      if (arg->type != sym->u.sub.argTypes[i])
                        {
                          if (nomore)
                            Value_new_ERROR(value, TYPEMISMATCH2, i + 1);
                          argerr = 1;
                          break;
                        }
                    }
                  else if (Value_retype(arg, sym->u.sub.argTypes[i])->type ==
                           V_ERROR)
                    {
                      if (nomore)
                        Value_new_ERROR(value, TYPEMISMATCH3, arg->u.error.msg,
                                        i + 1);
                      argerr = 1;
                      break;
                    }
                }
            }

          if (argerr)
            {
              if (nomore)
                {
                  Auto_funcReturn(&stack, (struct Pc *)0);
                  pc = funcpc;
                  return value;
                }
              else
                sym = sym->u.sub.u.bltin.next;
            }
        }
      while (argerr);
      ident->sym = sym;
      if (sym->type == BUILTINFUNCTION)
        {
          if (pass == INTERPRET)
            {
              if (sym->u.sub.u.bltin.call(value, &stack)->type == V_ERROR)
                pc = funcpc;
            }
          else
            Value_new_null(value, sym->u.sub.retType);
        }
      else if (sym->type == USERFUNCTION)
        {
          if (pass == INTERPRET)
            {
              int r = 1;

              pc = sym->u.sub.u.def.scope.start;
              if (pc.token->type == T_COLON)
                ++pc.token;
              else
                Program_skipEOL(&program, &pc, STDCHANNEL, 1);
              do
                {
                  if (statements(value)->type == V_ERROR)
                    {
                      if (strchr(value->u.error.msg, '\n') == (char *)0)
                        {
                          Auto_setError(&stack,
                                        Program_lineNumber(&program, &pc), &pc,
                                        value);
                          Program_PCtoError(&program, &pc, value);
                        }
                      if (stack.onerror.line != -1)
                        {
                          stack.resumeable = 1;
                          pc = stack.onerror;
                        }
                      else
                        {
                          Auto_frameToError(&stack, &program, value);
                          break;
                        }
                    }
                  else if (value->type != V_NIL)
                    break;
                  Value_destroy(value);
                }
              while ((r = Program_skipEOL(&program, &pc, STDCHANNEL, 1)));
              if (!r)
                Value_new_VOID(value);
            }
          else
            Value_new_null(value, sym->u.sub.retType);
        }
      Auto_funcReturn(&stack, pass == INTERPRET &&
                      value->type != V_ERROR ? &pc : (struct Pc *)0);
    }
  return value;
}

#ifdef USE_LR0

/* Grammar with LR(0) sets */
/*
Grammar:

1 EV -> E
2 E  -> E op E
3 E  -> op E
4 E  -> ( E )
5 E  -> value

i0:
EV -> . E                goto(0,E)=5
E  -> . E op E           goto(0,E)=5
E  -> . op E      +,-    shift 2
E  -> . ( E )     (      shift 3
E  -> . value     value  shift 4

i5:
EV -> E .         else   accept
E  -> E . op E    op     shift 1

i2:
E  -> op . E             goto(2,E)=6
E  -> . E op E           goto(2,E)=6
E  -> . op E      +,-    shift 2
E  -> . ( E )     (      shift 3
E  -> . value     value  shift 4

i3:
E  -> ( . E )            goto(3,E)=7
E  -> . E op E           goto(3,E)=7
E  -> . op E      +,-    shift 2
E  -> . ( E )     (      shift 3
E  -> . value     value  shift 4

i4:
E  -> value .            reduce 5

i1:
E  -> E op . E           goto(1,E)=8
E  -> . E op E           goto(1,E)=8
E  -> . op E      +,-    shift 2
E  -> . ( E )     (      shift 3
E  -> . value     value  shift 4

i6:
E  -> op E .             reduce 3
E  -> E . op E    op*    shift 1 *=if stack[-2] contains op of unary lower priority

i7:
E  -> ( E . )     )      shift 9
E  -> E . op E    op     shift 1

i8:
E  -> E op E .           reduce 2
E  -> E . op E    op*    shift 1 *=if stack[-2] contains op of lower priority or if
                                   if it is of equal priority and right associative

i9:
E  -> ( E ) .            reduce 4

*/

static struct Value *eval(struct Value *value, const char *desc)
{
  /* variables */
  static const int gotoState[10] = { 5, 8, 6, 7, -1, -1, -1, -1, -1, -1 };
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
      ip = pc.token->type;
      switch (sp->state)
        {
        case 0:
        case 1:
        case 2:
        case 3: /* including 4 */
          {
            if (ip == T_IDENTIFIER)
              {
                /* printf("state %d: shift 4\n",sp->state); */
                /* printf("state 4: reduce E -> value\n"); */
                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                if (pass == COMPILE)
                  {
                    if (((pc.token + 1)->type == T_OP ||
                         Auto_find(&stack, pc.token->u.identifier) == 0) &&
                        Global_find(&globals, pc.token->u.identifier,
                                    (pc.token + 1)->type == T_OP) == 0)
                      {
                        Value_new_ERROR(value, UNDECLARED);
                        goto error;
                      }
                  }
                if (pass != DECLARE &&
                    (pc.token->u.identifier->sym->type == GLOBALVAR ||
                     pc.token->u.identifier->sym->type == GLOBALARRAY ||
                     pc.token->u.identifier->sym->type == LOCALVAR))
                  {
                    struct Value *l;

                    if ((l = lvalue(value))->type == V_ERROR)
                      goto error;
                    Value_clone(&sp->u.value, l);
                  }
                else
                  {
                    struct Pc var = pc;

                    func(&sp->u.value);
                    if (sp->u.value.type == V_VOID)
                      {
                        pc = var;
                        Value_new_ERROR(value, VOIDVALUE);
                        goto error;
                      }
                  }
              }
            else if (ip == T_INTEGER)
              {
                /* printf("state %d: shift 4\n",sp->state); */
                /* printf("state 4: reduce E -> value\n"); */
                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                VALUE_NEW_INTEGER(&sp->u.value, pc.token->u.integer);
                ++pc.token;
              }
            else if (ip == T_REAL)
              {
                /* printf("state %d: shift 4\n",sp->state); */
                /* printf("state 4: reduce E -> value\n"); */
                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                VALUE_NEW_REAL(&sp->u.value, pc.token->u.real);
                ++pc.token;
              }
            else if (TOKEN_ISUNARYOPERATOR(ip))
              {
                /* printf("state %d: shift 2\n",sp->state); */
                ++sp;
                sp->state = 2;
                sp->u.token = ip;
                ++pc.token;
              }
            else if (ip == T_HEXINTEGER)
              {
                /* printf("state %d: shift 4\n",sp->state); */
                /* printf("state 4: reduce E -> value\n"); */
                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                VALUE_NEW_INTEGER(&sp->u.value, pc.token->u.hexinteger);
                ++pc.token;
              }
            else if (ip == T_OCTINTEGER)
              {
                /* printf("state %d: shift 4\n",sp->state); */
                /* printf("state 4: reduce E -> value\n"); */
                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                VALUE_NEW_INTEGER(&sp->u.value, pc.token->u.octinteger);
                ++pc.token;
              }
            else if (ip == T_OP)
              {
                /* printf("state %d: shift 3\n",sp->state); */
                ++sp;
                sp->state = 3;
                sp->u.token = T_OP;
                ++pc.token;
              }
            else if (ip == T_STRING)
              {
                /* printf("state %d: shift 4\n",sp->state); */
                /* printf("state 4: reduce E -> value\n"); */
                ++sp;
                sp->state = gotoState[(sp - 1)->state];
                Value_new_STRING(&sp->u.value);
                String_destroy(&sp->u.value.u.string);
                String_clone(&sp->u.value.u.string, pc.token->u.string);
                ++pc.token;
              }
            else
              {
                char state = sp->state;

                if (state == 0)
                  {
                    if (desc)
                      Value_new_ERROR(value, MISSINGEXPR, desc);
                    else
                      value = (struct Value *)0;
                  }
                else
                  Value_new_ERROR(value, MISSINGEXPR, _("operand"));
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
                ++pc.token;
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
                /* printf("state %d: shift 1 (not reducing E -> op
                 * E)\n",sp->state); */
                ++sp;
                sp->state = 1;
                sp->u.token = ip;
                ++pc.token;
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
                    Value_uneg(&sp->u.value, pass == INTERPRET);
                    break;
                  case T_NOT:
                    Value_unot(&sp->u.value, pass == INTERPRET);
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

        case 7: /* including 9 */
          {
            if (TOKEN_ISBINARYOPERATOR(ip))
              {
                /* printf("state %d: shift 1\n"sp->state); */
                ++sp;
                sp->state = 1;
                sp->u.token = ip;
                ++pc.token;
              }
            else if (ip == T_CP)
              {
                /* printf("state %d: shift 9\n",sp->state); */
                /* printf("state 9: reduce E -> ( E )\n"); */
                --sp;
                sp->state = gotoState[(sp - 1)->state];
                sp->u.value = (sp + 1)->u.value;
                ++pc.token;
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

            if (TOKEN_ISBINARYOPERATOR(ip)
                &&
                (((p1 = TOKEN_BINARYPRIORITY((sp - 1)->u.token)) < (p2 =
                                                                    TOKEN_BINARYPRIORITY
                                                                    (ip))) ||
                 (p1 == p2 && TOKEN_ISRIGHTASSOCIATIVE((sp - 1)->u.token))))
              {
                /* printf("state %d: shift 1\n",sp->state); */
                ++sp;
                sp->state = 1;
                sp->u.token = ip;
                ++pc.token;
              }
            else
              {
                /* printf("state %d: reduce E -> E op E\n",sp->state); */
                if (Value_commonType[(sp - 2)->u.value.type][sp->u.value.type]
                    == V_ERROR)
                  {
                    Value_destroy(&sp->u.value);
                    sp -= 2;
                    Value_destroy(&sp->u.value);
                    Value_new_ERROR(value, INVALIDOPERAND);
                    --sp;
                    goto error;
                  }
                else
                  switch ((sp - 1)->u.token)
                    {
                    case T_LT:
                      Value_lt(&(sp - 2)->u.value, &sp->u.value,
                               pass == INTERPRET);
                      break;
                    case T_LE:
                      Value_le(&(sp - 2)->u.value, &sp->u.value,
                               pass == INTERPRET);
                      break;
                    case T_EQ:
                      Value_eq(&(sp - 2)->u.value, &sp->u.value,
                               pass == INTERPRET);
                      break;
                    case T_GE:
                      Value_ge(&(sp - 2)->u.value, &sp->u.value,
                               pass == INTERPRET);
                      break;
                    case T_GT:
                      Value_gt(&(sp - 2)->u.value, &sp->u.value,
                               pass == INTERPRET);
                      break;
                    case T_NE:
                      Value_ne(&(sp - 2)->u.value, &sp->u.value,
                               pass == INTERPRET);
                      break;
                    case T_PLUS:
                      Value_add(&(sp - 2)->u.value, &sp->u.value,
                                pass == INTERPRET);
                      break;
                    case T_MINUS:
                      Value_sub(&(sp - 2)->u.value, &sp->u.value,
                                pass == INTERPRET);
                      break;
                    case T_MULT:
                      Value_mult(&(sp - 2)->u.value, &sp->u.value,
                                 pass == INTERPRET);
                      break;
                    case T_DIV:
                      Value_div(&(sp - 2)->u.value, &sp->u.value,
                                pass == INTERPRET);
                      break;
                    case T_IDIV:
                      Value_idiv(&(sp - 2)->u.value, &sp->u.value,
                                 pass == INTERPRET);
                      break;
                    case T_MOD:
                      Value_mod(&(sp - 2)->u.value, &sp->u.value,
                                pass == INTERPRET);
                      break;
                    case T_POW:
                      Value_pow(&(sp - 2)->u.value, &sp->u.value,
                                pass == INTERPRET);
                      break;
                    case T_AND:
                      Value_and(&(sp - 2)->u.value, &sp->u.value,
                                pass == INTERPRET);
                      break;
                    case T_OR:
                      Value_or(&(sp - 2)->u.value, &sp->u.value,
                               pass == INTERPRET);
                      break;
                    case T_XOR:
                      Value_xor(&(sp - 2)->u.value, &sp->u.value,
                                pass == INTERPRET);
                      break;
                    case T_EQV:
                      Value_eqv(&(sp - 2)->u.value, &sp->u.value,
                                pass == INTERPRET);
                      break;
                    case T_IMP:
                      Value_imp(&(sp - 2)->u.value, &sp->u.value,
                                pass == INTERPRET);
                      break;
                    default:
                      assert(0);
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
static inline struct Value *binarydown(struct Value *value, struct Value *(level) (struct Value * value), const int prio)
{
  enum TokenType op;
  struct Pc oppc;

  if (level(value) == (struct Value *)0)
    return (struct Value *)0;
  if (value->type == V_ERROR)
    return value;
  do
    {
      struct Value x;

      op = pc.token->type;
      if (!TOKEN_ISBINARYOPERATOR(op) || TOKEN_BINARYPRIORITY(op) != prio)
        return value;
      oppc = pc;
      ++pc.token;
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
        switch (op)
          {
          case T_LT:
            Value_lt(value, &x, pass == INTERPRET);
            break;
          case T_LE:
            Value_le(value, &x, pass == INTERPRET);
            break;
          case T_EQ:
            Value_eq(value, &x, pass == INTERPRET);
            break;
          case T_GE:
            Value_ge(value, &x, pass == INTERPRET);
            break;
          case T_GT:
            Value_gt(value, &x, pass == INTERPRET);
            break;
          case T_NE:
            Value_ne(value, &x, pass == INTERPRET);
            break;
          case T_PLUS:
            Value_add(value, &x, pass == INTERPRET);
            break;
          case T_MINUS:
            Value_sub(value, &x, pass == INTERPRET);
            break;
          case T_MULT:
            Value_mult(value, &x, pass == INTERPRET);
            break;
          case T_DIV:
            Value_div(value, &x, pass == INTERPRET);
            break;
          case T_IDIV:
            Value_idiv(value, &x, pass == INTERPRET);
            break;
          case T_MOD:
            Value_mod(value, &x, pass == INTERPRET);
            break;
          case T_POW:
            Value_pow(value, &x, pass == INTERPRET);
            break;
          case T_AND:
            Value_and(value, &x, pass == INTERPRET);
            break;
          case T_OR:
            Value_or(value, &x, pass == INTERPRET);
            break;
          case T_XOR:
            Value_xor(value, &x, pass == INTERPRET);
            break;
          case T_EQV:
            Value_eqv(value, &x, pass == INTERPRET);
            break;
          case T_IMP:
            Value_imp(value, &x, pass == INTERPRET);
            break;
          default:
            assert(0);
          }

      Value_destroy(&x);
    }
  while (value->type != V_ERROR);

  if (value->type == V_ERROR)
    pc = oppc;

  return value;
}


static inline struct Value *unarydown(struct Value *value, struct Value *(level) (struct Value * value), const int prio)
{
  enum TokenType op;
  struct Pc oppc;

  op = pc.token->type;
  if (!TOKEN_ISUNARYOPERATOR(op) || TOKEN_UNARYPRIORITY(op) != prio)
    return level(value);
  oppc = pc;
  ++pc.token;
  if (unarydown(value, level, prio) == (struct Value *)0)
    return Value_new_ERROR(value, MISSINGEXPR, _("unary operand"));

  if (value->type == V_ERROR)
    return value;

  switch (op)
    {
    case T_PLUS:
      Value_uplus(value, pass == INTERPRET);
      break;
    case T_MINUS:
      Value_uneg(value, pass == INTERPRET);
      break;
    case T_NOT:
      Value_unot(value, pass == INTERPRET);
      break;
    default:
      assert(0);
    }

  if (value->type == V_ERROR)
    pc = oppc;

  return value;
}


static struct Value *eval8(struct Value *value)
{
  switch (pc.token->type)
    {
    case T_IDENTIFIER:
      {
        struct Pc var;
        struct Value *l;

        var = pc;
        if (pass == COMPILE)
          {
            if (((pc.token + 1)->type == T_OP ||
                 Auto_find(&stack, pc.token->u.identifier) == 0) &&
                Global_find(&globals, pc.token->u.identifier,
                            (pc.token + 1)->type == T_OP) == 0)
              return Value_new_ERROR(value, UNDECLARED);
          }
        assert(pass == DECLARE || pc.token->u.identifier->sym);
        if (pass != DECLARE &&
            (pc.token->u.identifier->sym->type == GLOBALVAR ||
             pc.token->u.identifier->sym->type == GLOBALARRAY ||
             pc.token->u.identifier->sym->type == LOCALVAR))
          {
            if ((l = lvalue(value))->type == V_ERROR)
              return value;
            Value_clone(value, l);
          }
        else
          {
            func(value);
            if (value->type == V_VOID)
              {
                Value_destroy(value);
                pc = var;
                return Value_new_ERROR(value, VOIDVALUE);
              }
          }
        break;
      }

    case T_INTEGER:
      {
        VALUE_NEW_INTEGER(value, pc.token->u.integer);
        ++pc.token;
        break;
      }

    case T_REAL:
      {
        VALUE_NEW_REAL(value, pc.token->u.real);
        ++pc.token;
        break;
      }

    case T_STRING:
      {
        Value_new_STRING(value);
        String_destroy(&value->u.string);
        String_clone(&value->u.string, pc.token->u.string);
        ++pc.token;
        break;
      }

    case T_HEXINTEGER:
      {
        VALUE_NEW_INTEGER(value, pc.token->u.hexinteger);
        ++pc.token;
        break;
      }

    case T_OCTINTEGER:
      {
        VALUE_NEW_INTEGER(value, pc.token->u.octinteger);
        ++pc.token;
        break;
      }

    case T_OP:
      {
        ++pc.token;
        if (eval(value, _("parenthetic"))->type == V_ERROR)
          return value;
        if (pc.token->type != T_CP)
          {
            Value_destroy(value);
            return Value_new_ERROR(value, MISSINGCP);
          }
        ++pc.token;
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
  /* avoid function calls for atomic expression */
  switch (pc.token->type)
    {
    case T_STRING:
    case T_REAL:
    case T_INTEGER:
    case T_HEXINTEGER:
    case T_OCTINTEGER:
    case T_IDENTIFIER:
      if (!TOKEN_ISBINARYOPERATOR((pc.token + 1)->type) &&
          (pc.token + 1)->type != T_OP)
        return eval7(value);
    default:
      break;
    }
  if (binarydown(value, eval1, 0) == (struct Value *)0)
    {
      if (desc)
        return Value_new_ERROR(value, MISSINGEXPR, desc);
      else
        return (struct Value *)0;
    }
  else
    return value;
}
#endif

static void new(void)
{
  Global_destroy(&globals);
  Global_new(&globals);
  Auto_destroy(&stack);
  Auto_new(&stack);
  Program_destroy(&program);
  Program_new(&program);
  FS_closefiles();
  optionbase = 0;
}

static void pushLabel(enum LabelType type, struct Pc *patch)
{
  if (labelStackPointer == labelStackCapacity)
    {
      struct LabelStack *more;

      more =
        realloc(labelStack,
                sizeof(struct LabelStack) *
                (labelStackCapacity ? (labelStackCapacity *= 2) : (32)));
      labelStack = more;
    }

  labelStack[labelStackPointer].type = type;
  labelStack[labelStackPointer].patch = *patch;
  ++labelStackPointer;
}

static struct Pc *popLabel(enum LabelType type)
{
  if (labelStackPointer == 0 || labelStack[labelStackPointer - 1].type != type)
    return (struct Pc *)0;
  else
    return &labelStack[--labelStackPointer].patch;
}

static struct Pc *findLabel(enum LabelType type)
{
  int i;

  for (i = labelStackPointer - 1; i >= 0; --i)
    if (labelStack[i].type == type)
      return &labelStack[i].patch;
  return (struct Pc *)0;
}

static void labelStackError(struct Value *v)
{
  assert(labelStackPointer);
  pc = labelStack[labelStackPointer - 1].patch;
  switch (labelStack[labelStackPointer - 1].type)
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
        pc = *findLabel(L_FOR);
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
  if (labelStackPointer == 0)
    {
      return _("program");
    }
  switch (labelStack[labelStackPointer - 1].type)
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

  if (strcasecmp(pc.token->u.identifier->name, "mid$") == 0)    /* mid$(a$,n,m)=b$
     */
    {
      long int n, m;
      struct Value *l;

      ++pc.token;
      if (pc.token->type != T_OP)
        return Value_new_ERROR(value, MISSINGOP);
      ++pc.token;
      if (pc.token->type != T_IDENTIFIER)
        return Value_new_ERROR(value, MISSINGSTRIDENT);
      if (pass == DECLARE)
        {
          if (((pc.token + 1)->type == T_OP ||
               Auto_find(&stack, pc.token->u.identifier) == 0) &&
              Global_variable(&globals, pc.token->u.identifier,
                              pc.token->u.identifier->defaultType,
                              (pc.token + 1)->type ==
                              T_OP ? GLOBALARRAY : GLOBALVAR, 0) == 0)
            {
              return Value_new_ERROR(value, REDECLARATION);
            }
        }
      if ((l = lvalue(value))->type == V_ERROR)
        return value;
      if (pass == COMPILE && l->type != V_STRING)
        return Value_new_ERROR(value, TYPEMISMATCH4);
      if (pc.token->type != T_COMMA)
        return Value_new_ERROR(value, MISSINGCOMMA);
      ++pc.token;
      if (eval(value, _("position"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        return value;
      n = value->u.integer;
      Value_destroy(value);
      if (pass == INTERPRET && n < 1)
        return Value_new_ERROR(value, OUTOFRANGE, "position");
      if (pc.token->type == T_COMMA)
        {
          ++pc.token;
          if (eval(value, _("length"))->type == V_ERROR ||
              Value_retype(value, V_INTEGER)->type == V_ERROR)
            return value;
          m = value->u.integer;
          if (pass == INTERPRET && m < 0)
            return Value_new_ERROR(value, OUTOFRANGE, _("length"));
          Value_destroy(value);
        }
      else
        m = -1;
      if (pc.token->type != T_CP)
        return Value_new_ERROR(value, MISSINGCP);
      ++pc.token;
      if (pc.token->type != T_EQ)
        return Value_new_ERROR(value, MISSINGEQ);
      ++pc.token;
      if (eval(value, _("rhs"))->type == V_ERROR ||
          Value_retype(value, V_STRING)->type == V_ERROR)
        return value;
      if (pass == INTERPRET)
        {
          if (m == -1)
            m = value->u.string.length;
          String_set(&l->u.string, n - 1, &value->u.string, m);
        }
    }
  else
    {
      struct Value **l = (struct Value **)0;
      int i, used = 0, capacity = 0;
      struct Value retyped_value;

      for (;;)
        {
          if (used == capacity)
            {
              struct Value **more;

              capacity = capacity ? 2 * capacity : 2;
              more = realloc(l, capacity * sizeof(*l));
              l = more;
            }

          if (pass == DECLARE)
            {
              if (((pc.token + 1)->type == T_OP ||
                   Auto_find(&stack, pc.token->u.identifier) == 0) &&
                  Global_variable(&globals, pc.token->u.identifier,
                                  pc.token->u.identifier->defaultType,
                                  (pc.token + 1)->type ==
                                  T_OP ? GLOBALARRAY : GLOBALVAR, 0) == 0)
                {
                  if (capacity)
                    free(l);
                  return Value_new_ERROR(value, REDECLARATION);
                }
            }
          if ((l[used] = lvalue(value))->type == V_ERROR)
            return value;
          ++used;
          if (pc.token->type == T_COMMA)
            ++pc.token;
          else
            break;
        }

      if (pc.token->type != T_EQ)
        return Value_new_ERROR(value, MISSINGEQ);
      ++pc.token;
      expr = pc;
      if (eval(value, _("rhs"))->type == V_ERROR)
        return value;

      for (i = 0; i < used; ++i)
        {
          Value_clone(&retyped_value, value);
          if (pass != DECLARE &&
              VALUE_RETYPE(&retyped_value, (l[i])->type)->type == V_ERROR)
            {
              pc = expr;
              free(l);
              Value_destroy(value);
              *value = retyped_value;
              return value;
            }
          if (pass == INTERPRET)
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

  stack.resumeable = 0;
  if (clearGlobals)
    {
      Global_destroy(&globals);
      Global_new(&globals);
    }
  else
    Global_clearFunctions(&globals);

  if (Program_beginning(&program, &begin))
    {
      struct Pc savepc;
      int savepass;

      savepc = pc;
      savepass = pass;
      Program_norun(&program);
      for (pass = DECLARE; pass != INTERPRET; ++pass)
        {
          if (pass == DECLARE)
            {
              stack.begindata.line = -1;
              lastdata = &stack.begindata;
            }
          optionbase = 0;
          stopped = 0;
          program.runnable = 1;
          pc = begin;
          while (1)
            {
              statements(v);
              if (v->type == V_ERROR)
                break;
              Value_destroy(v);
              if (!Program_skipEOL(&program, &pc, 0, 0))
                {
                  Value_new_NIL(v);
                  break;
                }
            }
          if (v->type != V_ERROR && labelStackPointer > 0)
            {
              Value_destroy(v);
              labelStackError(v);
            }
          if (v->type == V_ERROR)
            {
              labelStackPointer = 0;
              Program_norun(&program);
              if (stack.cur)
                Auto_funcEnd(&stack);   /* Always correct? */
              pass = savepass;
              return v;
            }
        }
      pc = begin;
      if (Program_analyse(&program, &pc, v))
        {
          labelStackPointer = 0;
          Program_norun(&program);
          if (stack.cur)
            Auto_funcEnd(&stack);       /* Always correct? */
          pass = savepass;
          return v;
        }

      curdata = stack.begindata;
      pc = savepc;
      pass = savepass;
    }

  return Value_new_NIL(v);
}

static void runline(struct Token *line)
{
  struct Value value;

  FS_flush(STDCHANNEL);
  for (pass = DECLARE; pass != INTERPRET; ++pass)
    {
      curdata.line = -1;
      pc.line = -1;
      pc.token = line;
      optionbase = 0;
      stopped = 0;
      statements(&value);
      if (value.type != V_ERROR && pc.token->type != T_EOL)
        {
          Value_destroy(&value);
          Value_new_ERROR(&value, SYNTAX);
        }
      if (value.type != V_ERROR && labelStackPointer > 0)
        {
          Value_destroy(&value);
          labelStackError(&value);
        }
      if (value.type == V_ERROR)
        {
          struct String s;

          Auto_setError(&stack, Program_lineNumber(&program, &pc), &pc, &value);
          Program_PCtoError(&program, &pc, &value);
          labelStackPointer = 0;
          FS_putChars(STDCHANNEL, _("Error: "));
          String_new(&s);
          Value_toString(&value, &s, ' ', -1, 0, 0, 0, 0, -1, 0, 0);
          Value_destroy(&value);
          FS_putString(STDCHANNEL, &s);
          String_destroy(&s);
          return;
        }
      if (!program.runnable && pass == COMPILE)
        {
          Value_destroy(&value);
          (void)compileProgram(&value, 0);
        }
    }

  pc.line = -1;
  pc.token = line;
  optionbase = 0;
  curdata = stack.begindata;
  nextdata.line = -1;
  Value_destroy(&value);
  pass = INTERPRET;

  do
    {
      assert(pass == INTERPRET);
      statements(&value);
      assert(pass == INTERPRET);
      if (value.type == V_ERROR)
        {
          if (strchr(value.u.error.msg, '\n') == (char *)0)
            {
              Auto_setError(&stack, Program_lineNumber(&program, &pc), &pc,
                            &value);
              Program_PCtoError(&program, &pc, &value);
            }
          if (stack.onerror.line != -1)
            {
              stack.resumeable = 1;
              pc = stack.onerror;
            }
          else
            {
              struct String s;

              String_new(&s);
              if (!stopped)
                {
                  stopped = 0;
                  FS_putChars(STDCHANNEL, _("Error: "));
                }
              Auto_frameToError(&stack, &program, &value);
              Value_toString(&value, &s, ' ', -1, 0, 0, 0, 0, -1, 0, 0);
              while (Auto_gosubReturn(&stack, (struct Pc *)0));
              FS_putString(STDCHANNEL, &s);
              String_destroy(&s);
              Value_destroy(&value);
              break;
            }
        }
      Value_destroy(&value);
    }
  while (pc.token->type != T_EOL ||
         Program_skipEOL(&program, &pc, STDCHANNEL, 1));
}

static struct Value *evalGeometry(struct Value *value, unsigned int *dim, unsigned int geometry[])
{
  struct Pc exprpc = pc;

  if (eval(value, _("dimension"))->type == V_ERROR ||
      (pass != DECLARE && Value_retype(value, V_INTEGER)->type == V_ERROR))
    return value;
  if (pass == INTERPRET && value->u.integer < optionbase)
    {
      Value_destroy(value);
      pc = exprpc;
      return Value_new_ERROR(value, OUTOFRANGE, _("dimension"));
    }
  geometry[0] = value->u.integer - optionbase + 1;
  Value_destroy(value);
  if (pc.token->type == T_COMMA)
    {
      ++pc.token;
      exprpc = pc;
      if (eval(value, _("dimension"))->type == V_ERROR ||
          (pass != DECLARE && Value_retype(value, V_INTEGER)->type == V_ERROR))
        return value;
      if (pass == INTERPRET && value->u.integer < optionbase)
        {
          Value_destroy(value);
          pc = exprpc;
          return Value_new_ERROR(value, OUTOFRANGE, _("dimension"));
        }
      geometry[1] = value->u.integer - optionbase + 1;
      Value_destroy(value);
      *dim = 2;
    }
  else
    *dim = 1;
  if (pc.token->type == T_CP)
    ++pc.token;
  else
    return Value_new_ERROR(value, MISSINGCP);
  return (struct Value *)0;
}

static struct Value *convert(struct Value *value, struct Value *l, struct Token *t)
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
          return Value_new_ERROR(value, BADCONVERSION, _("integer"));
        datainput = t->u.datainput;
        v = Value_vali(datainput, &end, &overflow);
        if (end == datainput || (*end != '\0' && *end != ' ' && *end != '\t'))
          return Value_new_ERROR(value, BADCONVERSION, _("integer"));
        if (overflow)
          return Value_new_ERROR(value, OUTOFRANGE, _("converted value"));
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
          return Value_new_ERROR(value, BADCONVERSION, _("real"));
        datainput = t->u.datainput;
        v = Value_vald(datainput, &end, &overflow);
        if (end == datainput || (*end != '\0' && *end != ' ' && *end != '\t'))
          return Value_new_ERROR(value, BADCONVERSION, _("real"));
        if (overflow)
          return Value_new_ERROR(value, OUTOFRANGE, _("converted value"));
        Value_destroy(l);
        VALUE_NEW_REAL(l, v);
        break;
      }
    case V_STRING:
      {
        Value_destroy(l);
        Value_new_STRING(l);
        if (t->type == T_STRING)
          String_appendString(&l->u.string, t->u.string);
        else
          String_appendChars(&l->u.string, t->u.datainput);
        break;
      }
    default:
      assert(0);
    }
  return (struct Value *)0;
}

static struct Value *dataread(struct Value *value, struct Value *l)
{
  if (curdata.line == -1)
    {
      return Value_new_ERROR(value, ENDOFDATA);
    }
  if (curdata.token->type == T_DATA)
    {
      nextdata = curdata.token->u.nextdata;
      ++curdata.token;
    }
  if (convert(value, l, curdata.token))
    {
      return value;
    }
  ++curdata.token;
  if (curdata.token->type == T_COMMA)
    ++curdata.token;
  else
    curdata = nextdata;
  return (struct Value *)0;
}

static struct Value more_statements;
#include "statement.c"
static struct Value *statements(struct Value *value)
{
more:
  if (pc.token->statement)
    {
      struct Value *v;

      if ((v = pc.token->statement(value)))
        {
          if (v == &more_statements)
            goto more;
          else
            return value;
        }
    }
  else
    return Value_new_ERROR(value, MISSINGSTATEMENT);

  if (pc.token->type == T_COLON && (pc.token + 1)->type == T_ELSE)
    ++pc.token;
  else if ((pc.token->type == T_COLON && (pc.token + 1)->type != T_ELSE) ||
           pc.token->type == T_QUOTE)
    {
      ++pc.token;
      goto more;
    }
  else if ((pass == DECLARE || pass == COMPILE) && pc.token->type != T_EOL &&
           pc.token->type != T_ELSE)
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
#ifdef HAVE_GETTEXT
  bindtextdomain("bas", LOCALEDIR);
  textdomain("bas");
#endif
  stack.begindata.line = -1;
  Token_init(backslash_colon, uppercase);
  Global_new(&globals);
  Auto_new(&stack);
  Program_new(&program);
  FS_opendev(STDCHANNEL, 0, 1);
  FS_opendev(LPCHANNEL, -1, lpfd);
  run_restricted = restricted;
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
  else if (Program_merge(&program, dev, &value))
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

      Program_setname(&program, runFile);
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
      // FS_putChars(STDCHANNEL,"bas " VERSION "\n"); //acassis: fix it
      FS_putChars(STDCHANNEL, "Copyright 1999-2014 Michael Haardt.\n");
      FS_putChars(STDCHANNEL,
                  _("This is free software with ABSOLUTELY NO WARRANTY.\n"));
    }
  new();
  while (1)
    {
      struct Token *line;
      struct String s;

      stopped = 0;
      FS_nextline(STDCHANNEL);
      if (FS_istty(STDCHANNEL))
        FS_putChars(STDCHANNEL, "> ");
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
              if (program.numbered)
                {
                  if ((line + 1)->type == T_EOL)
                    {
                      struct Pc where;

                      if (Program_goLine(&program, line->u.integer, &where) ==
                          (struct Pc *)0)
                        FS_putChars(STDCHANNEL, (NOSUCHLINE));
                      else
                        Program_delete(&program, &where, &where);
                      Token_destroy(line);
                    }
                  else
                    Program_store(&program, line, line->u.integer);
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
              if (FS_istty(STDCHANNEL) && bas_end > 0)
                {
                  FS_putChars(STDCHANNEL, _("END program\n"));
                  bas_end = 0;
                }
            }
          else
            {
              FS_putChars(STDCHANNEL, _("Invalid line\n"));
              Token_destroy(line);
            }
        }
      else
        Token_destroy(line);
    }
}

void bas_exit(void)
{
  Auto_destroy(&stack);
  Global_destroy(&globals);
  Program_destroy(&program);
  if (labelStack)
    free(labelStack);
  FS_closefiles();
  FS_close(LPCHANNEL);
  FS_close(STDCHANNEL);
}
