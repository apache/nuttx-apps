/****************************************************************************
 * apps/interpreters/bas/bas_statement.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdlib.h>
#include <strings.h>
#include <assert.h>

#include "bas_statement.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define _(String) String

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

struct Value *stmt_CALL(struct Value *value)
{
  ++g_pc.token;
  if (g_pc.token->type != T_IDENTIFIER)
    {
      return Value_new_ERROR(value, MISSINGPROCIDENT);
    }

  if (g_pass == DECLARE)
    {
      if (func(value)->type == V_ERROR)
        {
          return value;
        }
      else
        {
          Value_destroy(value);
        }
    }
  else
    {
      if (g_pass == COMPILE)
        {
          if (Global_find
              (&g_globals, g_pc.token->u.identifier,
               (g_pc.token + 1)->type == T_OP) == 0)
            {
              return Value_new_ERROR(value, UNDECLARED);
            }
        }

      if (g_pc.token->u.identifier->sym->type != USERFUNCTION &&
          g_pc.token->u.identifier->sym->type != BUILTINFUNCTION)
        {
          return Value_new_ERROR(value, TYPEMISMATCH1, "variable",
                                 "function");
        }

      func(value);
      if (Value_retype(value, V_VOID)->type == V_ERROR)
        {
          return value;
        }

      Value_destroy(value);
    }

  return (struct Value *)0;
}

struct Value *stmt_CASE(struct Value *value)
{
  struct Pc statementpc = g_pc;

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      struct Pc *selectcase;
      struct Pc *nextcasevalue;

      if ((selectcase = findLabel(L_SELECTCASE)) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, STRAYCASE);
        }

      for (nextcasevalue = &selectcase->token->u.selectcase->nextcasevalue;
           nextcasevalue->line != -1;
           nextcasevalue = &nextcasevalue->token->u.casevalue->nextcasevalue)
        {
        }

      *nextcasevalue = g_pc;
      if (g_pass == COMPILE)
        {
          g_pc.token->u.casevalue->endselect =
            selectcase->token->u.selectcase->endselect;
        }

      g_pc.token->u.casevalue->nextcasevalue.line = -1;
      ++g_pc.token;
      switch (statementpc.token->type)
        {
        case T_CASEELSE:
          break;

        case T_CASEVALUE:
          {
            struct Pc exprpc;

            do
              {
                if (g_pc.token->type == T_IS)
                  {
                    ++g_pc.token;
                    switch (g_pc.token->type)
                      {
                      case T_LT:
                      case T_LE:
                      case T_EQ:
                      case T_GE:
                      case T_GT:
                      case T_NE:
                        break;

                      default:
                        return Value_new_ERROR(value, MISSINGRELOP);
                      }

                    ++g_pc.token;
                    exprpc = g_pc;
                    if (eval(value, "`is'")->type == V_ERROR)
                      {
                        return value;
                      }

                    if (Value_retype
                        (value,
                         selectcase->token->u.selectcase->type)->type ==
                        V_ERROR)
                      {
                        g_pc = exprpc;
                        return value;
                      }

                    Value_destroy(value);
                  }

                else            /* value or range */
                  {
                    exprpc = g_pc;
                    if (eval(value, "`case'")->type == V_ERROR)
                      {
                        return value;
                      }

                    if (Value_retype
                        (value,
                         selectcase->token->u.selectcase->type)->type ==
                        V_ERROR)
                      {
                        g_pc = exprpc;
                        return value;
                      }

                    Value_destroy(value);
                    if (g_pc.token->type == T_TO)
                      {
                        ++g_pc.token;
                        exprpc = g_pc;
                        if (eval(value, "`case'")->type == V_ERROR)
                          {
                            return value;
                          }

                        if (Value_retype
                            (value,
                             selectcase->token->u.selectcase->type)->type ==
                            V_ERROR)
                          {
                            g_pc = exprpc;
                            return value;
                          }

                        Value_destroy(value);
                      }
                  }

                if (g_pc.token->type == T_COMMA)
                  {
                    ++g_pc.token;
                  }
                else
                  {
                    break;
                  }
              }
            while (1);

            break;
          }

        default:
          assert(0);
        }
    }
  else
    {
      g_pc = g_pc.token->u.casevalue->endselect;
    }

  return (struct Value *)0;
}

struct Value *stmt_CHDIR_MKDIR(struct Value *value)
{
  struct Pc dirpc;
  struct Pc statementpc = g_pc;
  int res = -1;
  int errcode = -1;

  ++g_pc.token;
  dirpc = g_pc;
  if (eval(value, _("directory"))->type == V_ERROR ||
      Value_retype(value, V_STRING)->type == V_ERROR)
    {
      return value;
    }

  if (g_pass == INTERPRET)
    {
      switch (statementpc.token->type)
        {
        case T_CHDIR:
          res = chdir(value->u.string.character);
          break;

        case T_MKDIR:
          res = mkdir(value->u.string.character, 0777);
          break;

        default:
          assert(0);
        }

      errcode = errno;
    }

  Value_destroy(value);
  if (g_pass == INTERPRET && res == -1)
    {
      g_pc = dirpc;
      return Value_new_ERROR(value, IOERROR, strerror(errcode));
    }

  return (struct Value *)0;
}

struct Value *stmt_CLEAR(struct Value *value)
{
  if (g_pass == INTERPRET)
    {
      Global_clear(&g_globals);
      FS_closefiles();
    }

  ++g_pc.token;
  return (struct Value *)0;
}

struct Value *stmt_CLOSE(struct Value *value)
{
  int hasargs = 0;
  struct Pc chnpc;

  ++g_pc.token;
  while (1)
    {
      chnpc = g_pc;
      if (g_pc.token->type == T_CHANNEL)
        {
          hasargs = 1;
          ++g_pc.token;
        }

      if (eval(value, (const char *)0) == (struct Value *)0)
        {
          if (hasargs)
            {
              return Value_new_ERROR(value, MISSINGEXPR, _("channel"));
            }
          else
            {
              break;
            }
        }

      hasargs = 1;
      if (value->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      if (g_pass == INTERPRET && FS_close(value->u.integer) == -1)
        {
          Value_destroy(value);
          g_pc = chnpc;
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }

      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  if (!hasargs && g_pass == INTERPRET)
    {
      FS_closefiles();
    }

  return (struct Value *)0;
}

struct Value *stmt_CLS(struct Value *value)
{
  struct Pc statementpc = g_pc;

  ++g_pc.token;
  if (g_pass == INTERPRET && FS_cls(STDCHANNEL) == -1)
    {
      g_pc = statementpc;
      return Value_new_ERROR(value, IOERROR, FS_errmsg);
    }

  return (struct Value *)0;
}

struct Value *stmt_COLOR(struct Value *value)
{
  int foreground = -1;
  int background = -1;
  struct Pc statementpc = g_pc;

  ++g_pc.token;
  if (eval(value, (const char *)0))
    {
      if (value->type == V_ERROR || (g_pass != DECLARE &&
          Value_retype(value, V_INTEGER)->type == V_ERROR))
        {
          return value;
        }

      foreground = value->u.integer;
      if (foreground < 0 || foreground > 15)
        {
          Value_destroy(value);
          g_pc = statementpc;
          return Value_new_ERROR(value, OUTOFRANGE, _("foreground colour"));
        }
    }

  Value_destroy(value);
  if (g_pc.token->type == T_COMMA)
    {
      ++g_pc.token;
      if (eval(value, (const char *)0))
        {
          if (value->type == V_ERROR ||
              (g_pass != DECLARE &&
               Value_retype(value, V_INTEGER)->type == V_ERROR))
            {
              return value;
            }

          background = value->u.integer;
          if (background < 0 || background > 15)
            {
              Value_destroy(value);
              g_pc = statementpc;
              return Value_new_ERROR(value, OUTOFRANGE,
                                     _("background colour"));
            }
        }

      Value_destroy(value);
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
          if (eval(value, (const char *)0))
            {
              int bordercolour = -1;

              if (value->type == V_ERROR ||
                  (g_pass != DECLARE &&
                   Value_retype(value, V_INTEGER)->type == V_ERROR))
                {
                  return value;
                }

              bordercolour = value->u.integer;
              if (bordercolour < 0 || bordercolour > 15)
                {
                  Value_destroy(value);
                  g_pc = statementpc;
                  return Value_new_ERROR(value, OUTOFRANGE,
                                         _("border colour"));
                }
            }

          Value_destroy(value);
        }
    }

  if (g_pass == INTERPRET)
    {
      FS_colour(STDCHANNEL, foreground, background);
    }

  return (struct Value *)0;
}

struct Value *stmt_DATA(struct Value *value)
{
  if (DIRECTMODE)
    {
      return Value_new_ERROR(value, NOTINDIRECTMODE);
    }

  if (g_pass == DECLARE)
    {
      *g_lastdata = g_pc;
      (g_lastdata = &(g_pc.token->u.nextdata))->line = -1;
    }

  ++g_pc.token;
  while (1)
    {
      if (g_pc.token->type != T_STRING && g_pc.token->type != T_DATAINPUT)
        {
          return Value_new_ERROR(value, MISSINGDATAINPUT);
        }

      ++g_pc.token;
      if (g_pc.token->type != T_COMMA)
        {
          break;
        }
      else
        {
          ++g_pc.token;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_DEFFN_DEFPROC_FUNCTION_SUB(struct Value *value)
{
  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      struct Pc statementpc = g_pc;
      struct Identifier *fn;
      int proc;
      int args = 0;

      if (DIRECTMODE)
        {
          return Value_new_ERROR(value, NOTINDIRECTMODE);
        }

      proc = (g_pc.token->type == T_DEFPROC || g_pc.token->type == T_SUB);
      ++g_pc.token;
      if (g_pc.token->type != T_IDENTIFIER)
        {
          if (proc)
            {
              return Value_new_ERROR(value, MISSINGPROCIDENT);
            }
          else
            {
              return Value_new_ERROR(value, MISSINGFUNCIDENT);
            }
        }

      fn = g_pc.token->u.identifier;
      if (proc)
        {
          fn->defaultType = V_VOID;
        }

      ++g_pc.token;
      if (findLabel(L_FUNC))
        {
          g_pc = statementpc;
          return Value_new_ERROR(value, NESTEDDEFINITION);
        }

      Auto_variable(&g_stack, fn);
      if (g_pc.token->type == T_OP)       /* arguments */
        {
          ++g_pc.token;
          while (1)
            {
              if (g_pc.token->type != T_IDENTIFIER)
                {
                  Auto_funcEnd(&g_stack);
                  return Value_new_ERROR(value, MISSINGFORMIDENT);
                }

              if (Auto_variable(&g_stack, g_pc.token->u.identifier) == 0)
                {
                  Auto_funcEnd(&g_stack);
                  return Value_new_ERROR(value, ALREADYDECLARED);
                }

              ++args;
              ++g_pc.token;
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
              Auto_funcEnd(&g_stack);
              return Value_new_ERROR(value, MISSINGCP);
            }

          ++g_pc.token;
        }

      if (g_pass == DECLARE)
        {
          enum ValueType *t =
            args ? malloc(args * sizeof(enum ValueType)) : 0;
          int i;

          for (i = 0; i < args; ++i)
            {
              t[i] = Auto_argType(&g_stack, i);
            }

          if (Global_function(&g_globals, fn, fn->defaultType,
                              &g_pc, &statementpc, args, t) == 0)
            {
              free(t);
              Auto_funcEnd(&g_stack);
              g_pc = statementpc;
              return Value_new_ERROR(value, REDECLARATION);
            }

          Program_addScope(&g_program, &fn->sym->u.sub.u.def.scope);
        }

      pushLabel(L_FUNC, &statementpc);
      if (g_pc.token->type == T_EQ)
        {
          return stmt_EQ_FNRETURN_FNEND(value);
        }
    }
  else
    {
      g_pc = (g_pc.token + 1)->u.identifier->sym->u.sub.u.def.scope.end;
    }

  return (struct Value *)0;
}

struct Value *stmt_DEC_INC(struct Value *value)
{
  int step;

  step = (g_pc.token->type == T_DEC ? -1 : 1);
  ++g_pc.token;
  while (1)
    {
      struct Value *l;
      struct Value stepValue;
      struct Pc lvaluepc;

      lvaluepc = g_pc;
      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGDECINCIDENT);
        }

      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType,
                          (g_pc.token + 1)->type ==
                          T_OP ? GLOBALARRAY : GLOBALVAR, 0) == 0)
        {
          return Value_new_ERROR(value, REDECLARATION);
        }

      if ((l = lvalue(value))->type == V_ERROR)
        {
          return value;
        }

      if (l->type == V_INTEGER)
        {
          VALUE_NEW_INTEGER(&stepValue, step);
        }
      else if (l->type == V_REAL)
        {
          VALUE_NEW_REAL(&stepValue, (double)step);
        }
      else
        {
          g_pc = lvaluepc;
          return Value_new_ERROR(value, TYPEMISMATCH5);
        }

      if (g_pass == INTERPRET)
        {
          Value_add(l, &stepValue, 1);
        }

      Value_destroy(&stepValue);
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_DEFINT_DEFDBL_DEFSTR(struct Value *value)
{
  enum ValueType dsttype = V_NIL;

  switch (g_pc.token->type)
    {
    case T_DEFINT:
      dsttype = V_INTEGER;
      break;

    case T_DEFDBL:
      dsttype = V_REAL;
      break;

    case T_DEFSTR:
      dsttype = V_STRING;
      break;

    default:
      assert(0);
    }

  ++g_pc.token;
  while (1)
    {
      struct Identifier *ident;

      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGVARIDENT);
        }

      if (g_pc.token->u.identifier->defaultType != V_REAL)
        {
          switch (dsttype)
            {
            case V_INTEGER:
              return Value_new_ERROR(value, BADIDENTIFIER, _("integer"));

            case V_REAL:
              return Value_new_ERROR(value, BADIDENTIFIER, _("real"));

            case V_STRING:
              return Value_new_ERROR(value, BADIDENTIFIER, _("string"));

            default:
              assert(0);
            }
        }

      ident = g_pc.token->u.identifier;
      ++g_pc.token;
      if (g_pc.token->type == T_MINUS)
        {
          struct Identifier i;

          if (strlen(ident->name) != 1)
            {
              return Value_new_ERROR(value, BADRANGE);
            }

          ++g_pc.token;
          if (g_pc.token->type != T_IDENTIFIER)
            {
              return Value_new_ERROR(value, MISSINGVARIDENT);
            }

          if (strlen(g_pc.token->u.identifier->name) != 1)
            {
              return Value_new_ERROR(value, BADRANGE);
            }

          for (i.name[0] = tolower(ident->name[0]), i.name[1] = '\0';
               i.name[0] <= tolower(g_pc.token->u.identifier->name[0]);
               ++i.name[0])
            {
              Global_variable(&g_globals, &i, dsttype, GLOBALVAR, 1);
            }

          ++g_pc.token;
        }
      else
        {
          Global_variable(&g_globals, ident, dsttype, GLOBALVAR, 1);
        }

      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_DELETE(struct Value *value)
{
  struct Pc from;
  struct Pc to;
  int f = 0;
  int t = 0;

  if (g_pass == INTERPRET && !DIRECTMODE)
    {
      return Value_new_ERROR(value, NOTINPROGRAMMODE);
    }

  ++g_pc.token;
  if (g_pc.token->type == T_INTEGER)
    {
      if (g_pass == INTERPRET &&
          Program_goLine(&g_program, g_pc.token->u.integer,
                         &from) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, NOSUCHLINE);
        }

      f = 1;
      ++g_pc.token;
    }

  if (g_pc.token->type == T_MINUS || g_pc.token->type == T_COMMA)
    {
      ++g_pc.token;
      if (g_pc.token->type == T_INTEGER)
        {
          if (g_pass == INTERPRET &&
              Program_goLine(&g_program, g_pc.token->u.integer,
                             &to) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, NOSUCHLINE);
            }

          t = 1;
          ++g_pc.token;
        }
    }
  else if (f == 1)
    {
      to = from;
      t  = 1;
    }

  if (!f && !t)
    {
      return Value_new_ERROR(value, MISSINGLINENUMBER);
    }

  if (g_pass == INTERPRET)
    {
      Program_delete(&g_program, f ? &from : (struct Pc *)0,
                     t ? &to : (struct Pc *)0);
    }

  return (struct Value *)0;
}

struct Value *stmt_DIM(struct Value *value)
{
  ++g_pc.token;
  while (1)
    {
      unsigned int capacity = 0;
      unsigned int *geometry = (unsigned int *)0;
      struct Var *var;
      struct Pc dimpc;
      unsigned int dim;
      enum ValueType vartype;

      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGARRIDENT);
        }

      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType, GLOBALARRAY,
                          0) == 0)
        {
          return Value_new_ERROR(value, REDECLARATION);
        }

      var = &g_pc.token->u.identifier->sym->u.var;
      if (g_pass == INTERPRET && var->dim)
        {
          return Value_new_ERROR(value, REDIM);
        }

      vartype = var->type;
      ++g_pc.token;
      if (g_pc.token->type != T_OP)
        {
          return Value_new_ERROR(value, MISSINGOP);
        }

      ++g_pc.token;
      dim = 0;
      while (1)
        {
          dimpc = g_pc;
          if (eval(value, _("dimension"))->type == V_ERROR ||
              (g_pass != DECLARE &&
               Value_retype(value, V_INTEGER)->type == V_ERROR))
            {
              if (capacity)
                {
                  free(geometry);
                }

              return value;
            }

          if (g_pass == INTERPRET && value->u.integer < g_optionbase)       /* error */
            {
              Value_destroy(value);
              Value_new_ERROR(value, OUTOFRANGE, _("dimension"));
            }

          if (value->type == V_ERROR)   /* abort */
            {
              if (capacity)
                {
                  free(geometry);
                }

              g_pc = dimpc;
              return value;
            }

          if (g_pass == INTERPRET)
            {
              if (dim == capacity)      /* enlarge geometry */
                {
                  unsigned int *more;

                  more =
                    realloc(geometry,
                            sizeof(unsigned int) *
                            (capacity ? (capacity *= 2) : (capacity = 3)));
                  geometry = more;
                }

              geometry[dim] = value->u.integer - g_optionbase + 1;
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

      if (g_pc.token->type != T_CP)       /* abort */
        {
          if (capacity)
            {
              free(geometry);
            }

          return Value_new_ERROR(value, MISSINGCP);
        }

      ++g_pc.token;
      if (g_pass == INTERPRET)
        {
          struct Var newarray;

          assert(capacity);
          if (Var_new(&newarray, vartype, dim, geometry, g_optionbase) ==
              (struct Var *)0)
            {
              free(geometry);
              return Value_new_ERROR(value, OUTOFMEMORY);
            }

          Var_destroy(var);
          *var = newarray;
          free(geometry);
        }

      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;           /* advance to next var */
        }
      else
        {
          break;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_DISPLAY(struct Value *value)
{
  struct Pc statementpc = g_pc;

  ++g_pc.token;
  if (eval(value, _("file name"))->type == V_ERROR ||
      (g_pass != DECLARE && Value_retype(value, V_STRING)->type == V_ERROR))
    {
      return value;
    }

  if (g_pass == INTERPRET && cat(value->u.string.character) == -1)
    {
      const char *msg = strerror(errno);

      Value_destroy(value);
      g_pc = statementpc;
      return Value_new_ERROR(value, IOERROR, msg);
    }
  else
    {
      Value_destroy(value);
    }

  return (struct Value *)0;
}

struct Value *stmt_DO(struct Value *value)
{
  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      pushLabel(L_DO, &g_pc);
    }

  ++g_pc.token;
  return (struct Value *)0;
}

struct Value *stmt_DOcondition(struct Value *value)
{
  struct Pc dowhilepc = g_pc;
  int negate = (g_pc.token->type == T_DOUNTIL);

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      pushLabel(L_DOcondition, &g_pc);
    }

  ++g_pc.token;
  if (eval(value, "condition")->type == V_ERROR)
    {
      return value;
    }

  if (g_pass == INTERPRET)
    {
      int condition;

      condition = Value_isNull(value);
      if (negate)
        {
          condition = !condition;
        }

      if (condition)
        {
          g_pc = dowhilepc.token->u.exitdo;
        }

      Value_destroy(value);
    }

  return (struct Value *)0;
}

struct Value *stmt_EDIT(struct Value *value)
{
#if defined(CONFIG_EXAMPLES_BAS_EDITOR) && defined(CONFIG_EXAMPLES_BAS_SHELL) && defined(CONFIG_ARCH_HAVE_VFORK)
  long int line;
  struct Pc statementpc = g_pc;
  int status;

  ++g_pc.token;
  if (g_pc.token->type == T_INTEGER)
    {
      struct Pc where;

      if (g_program.numbered)
        {
          if (Program_goLine(&g_program, g_pc.token->u.integer, &where) ==
              (struct Pc *)0)
            {
              return Value_new_ERROR(value, NOSUCHLINE);
            }

          line = where.line + 1;
        }
      else
        {
          if (!Program_end(&g_program, &where))
            {
              return Value_new_ERROR(value, NOPROGRAM);
            }

          line = g_pc.token->u.integer;
          if (line < 1 || line > (where.line + 1))
            {
              return Value_new_ERROR(value, NOSUCHLINE);
            }
        }

      ++g_pc.token;
    }
  else
    {
      line = 1;
    }

  if (g_pass == INTERPRET)
    {
      /* variables */

      char *name;
      int chn;
      struct Program newProgram;
      const char *visual;
      const char *basename;
      const char *shell;
      struct String cmd;
      static struct
        {
          const char *editor;
          const char *flag;
        }

      gotoLine[] =
        {
          "Xemacs", "+%ld ",
          "cemacs", "+%ld ",
          "emacs", "+%ld ",
          "emori", "-l%ld ",
          "fe", "-l%ld ",
          "jed", "+%ld ",
          "jmacs", "+%ld ",
          "joe", "+%ld ",
          "modeori", "-l%ld ",
          "origami", "-l%ld ",
          "vi", "-c%ld ",
          "vim", "+%ld ",
          "xemacs", "+%ld "
        };

      unsigned int i;
      pid_t pid;

      if (!DIRECTMODE)
        {
          g_pc = statementpc;
          return Value_new_ERROR(value, NOTINPROGRAMMODE);
        }

      if ((name = tmpnam(NULL)) == (char *)0)
        {
          g_pc = statementpc;
          return Value_new_ERROR(value, IOERROR,
                                 _("generating temporary file name failed"));
        }

      if ((chn = FS_openout(name)) == -1)
        {
          g_pc = statementpc;
          return Value_new_ERROR(value, IOERRORCREATE, name, FS_errmsg);
        }

      FS_width(chn, 0);
      if (Program_list(&g_program, chn, 0, 0, 0, value))
        {
          g_pc = statementpc;
          return value;
        }

      if (FS_close(chn) == -1)
        {
          g_pc = statementpc;
          unlink(name);
          return Value_new_ERROR(value, IOERRORCLOSE, name, FS_errmsg);
        }

      if ((visual = getenv("VISUAL")) == (char *)0 &&
          (visual = getenv("EDITOR")) == (char *)0)
        {
          visual = "vi";
        }

      basename = strrchr(visual, '/');
      if (basename == (char *)0)
        {
          basename = visual;
        }

      if ((shell = getenv("SHELL")) == (char *)0)
        {
          shell = "/bin/sh";
        }

      String_new(&cmd);
      String_appendChars(&cmd, visual);
      String_appendChar(&cmd, ' ');
      for (i = 0; i < sizeof(gotoLine) / sizeof(gotoLine[0]); ++i)
        {
          if (strcmp(basename, gotoLine[i].editor) == 0)
            {
              String_appendPrintf(&cmd, gotoLine[i].flag, line);
              break;
            }
        }

      String_appendChars(&cmd, name);
      FS_shellmode(STDCHANNEL);
      switch (pid = vfork())
        {
        case -1:
          {
            unlink(name);
            FS_fsmode(STDCHANNEL);
            return Value_new_ERROR(value, FORKFAILED, strerror(errno));
          }

        case 0:
          {
            execl(shell, shell, "-c", cmd.character, (const char *)0);
            exit(127);
          }

        default:
          {
            /* Wait for the editor to complete */

            while (waitpid(pid, &status, 0) < 0 && errno != EINTR);
          }
        }

      FS_fsmode(STDCHANNEL);
      String_destroy(&cmd);
      if ((chn = FS_openin(name)) == -1)
        {
          g_pc = statementpc;
          return Value_new_ERROR(value, IOERROROPEN, name, FS_errmsg);
        }

      Program_new(&newProgram);
      if (Program_merge(&newProgram, chn, value))
        {
          FS_close(chn);
          unlink(name);
          g_pc = statementpc;
          return value;
        }

      FS_close(chn);
      Program_setname(&newProgram, g_program.name.character);
      Program_destroy(&g_program);
      g_program = newProgram;
      unlink(name);
    }

  return (struct Value *)0;
#else
  return Value_new_ERROR(value, NOTAVAILABLE);
#endif
}

struct Value *stmt_ELSE_ELSEIFELSE(struct Value *value)
{
  if (g_pass == INTERPRET)
    {
      g_pc = g_pc.token->u.endifpc;
    }

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      struct Pc elsepc = g_pc;
      struct Pc *ifinstr;
      int elseifelse = (g_pc.token->type == T_ELSEIFELSE);

      if ((ifinstr = popLabel(L_IF)) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, STRAYELSE1);
        }

      if (ifinstr->token->type == T_ELSEIFIF)
        {
          (ifinstr->token - 1)->u.elsepc = g_pc;
        }

      ++g_pc.token;
      ifinstr->token->u.elsepc = g_pc;
      assert(ifinstr->token->type == T_ELSEIFIF ||
             ifinstr->token->type == T_IF);
      if (elseifelse)
        {
          return &more_statements;
        }
      else
        {
          pushLabel(L_ELSE, &elsepc);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_END(struct Value *value)
{
  if (g_pass == INTERPRET)
    {
      g_pc      = g_pc.token->u.endpc;
      g_bas_end = true;
    }

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      if (Program_end(&g_program, &g_pc.token->u.endpc))
        {
          ++g_pc.token;
        }
      else
        {
          struct Token *eol;

          for (eol = g_pc.token; eol->type != T_EOL; ++eol);

          g_pc.token->u.endpc = g_pc;
          g_pc.token->u.endpc.token = eol;
          ++g_pc.token;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_ENDIF(struct Value *value)
{
  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      struct Pc endifpc = g_pc;
      struct Pc *ifpc;
      struct Pc *elsepc;

      if ((ifpc = popLabel(L_IF)))
        {
          ifpc->token->u.elsepc = endifpc;
          if (ifpc->token->type == T_ELSEIFIF)
            {
              (ifpc->token - 1)->u.elsepc = g_pc;
            }
        }
      else if ((elsepc = popLabel(L_ELSE)))
        {
          elsepc->token->u.endifpc = endifpc;
        }
      else
        {
          return Value_new_ERROR(value, STRAYENDIF);
        }
    }

  ++g_pc.token;
  return (struct Value *)0;
}

struct Value *stmt_ENDFN(struct Value *value)
{
  struct Pc *curfn = (struct Pc *)0;
  struct Pc eqpc = g_pc;

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      if ((curfn = popLabel(L_FUNC)) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, STRAYENDFN);
        }

      if ((eqpc.token->u.type =
           (curfn->token + 1)->u.identifier->defaultType) == V_VOID)
        {
          return Value_new_ERROR(value, STRAYENDFN);
        }
    }

  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      return Value_clone(value,
                         Var_value(Auto_local(&g_stack, 0), 0, (int *)0,
                                   (struct Value *)0));
    }
  else
    {
      if (g_pass == DECLARE)
        {
          Global_endfunction(&g_globals, (curfn->token + 1)->u.identifier,
                             &g_pc);
        }

      Auto_funcEnd(&g_stack);
    }

  return (struct Value *)0;
}

struct Value *stmt_ENDPROC_SUBEND(struct Value *value)
{
  struct Pc *curfn = (struct Pc *)0;

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      if ((curfn = popLabel(L_FUNC)) == (struct Pc *)0 ||
          (curfn->token + 1)->u.identifier->defaultType != V_VOID)
        {
          if (curfn != (struct Pc *)0)
            {
              pushLabel(L_FUNC, curfn);
            }

          return Value_new_ERROR(value, STRAYSUBEND, topLabelDescription());
        }
    }

  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      return Value_new_VOID(value);
    }
  else
    {
      if (g_pass == DECLARE)
        {
          Global_endfunction(&g_globals, (curfn->token + 1)->u.identifier,
                             &g_pc);
        }

      Auto_funcEnd(&g_stack);
    }

  return (struct Value *)0;
}

struct Value *stmt_ENDSELECT(struct Value *value)
{
  struct Pc statementpc = g_pc;

  ++g_pc.token;
  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      struct Pc *selectcasepc;

      if ((selectcasepc = popLabel(L_SELECTCASE)))
        {
          selectcasepc->token->u.selectcase->endselect = g_pc;
        }
      else
        {
          g_pc = statementpc;
          return Value_new_ERROR(value, STRAYENDSELECT);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_ENVIRON(struct Value *value)
{
  struct Pc epc = g_pc;

  ++g_pc.token;
  if (eval(value, _("environment variable"))->type == V_ERROR ||
      Value_retype(value, V_STRING)->type == V_ERROR)
    {
      return value;
    }

  if (g_pass == INTERPRET && value->u.string.character)
    {
      if (putenv(value->u.string.character) == -1)
        {
          Value_destroy(value);
          g_pc = epc;
          return Value_new_ERROR(value, ENVIRONFAILED, strerror(errno));
        }
    }

  Value_destroy(value);
  return (struct Value *)0;
}

struct Value *stmt_FNEXIT(struct Value *value)
{
  struct Pc *curfn = (struct Pc *)0;

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      if ((curfn = findLabel(L_FUNC)) == (struct Pc *)0 ||
          (curfn->token + 1)->u.identifier->defaultType == V_VOID)
        {
          return Value_new_ERROR(value, STRAYFNEXIT);
        }
    }

  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      return Value_clone(value,
                         Var_value(Auto_local(&g_stack, 0), 0, (int *)0,
                                   (struct Value *)0));
    }

  return (struct Value *)0;
}

struct Value *stmt_COLON_EOL(struct Value *value)
{
  return (struct Value *)0;
}

struct Value *stmt_QUOTE_REM(struct Value *value)
{
  ++g_pc.token;
  return (struct Value *)0;
}

struct Value *stmt_EQ_FNRETURN_FNEND(struct Value *value)
{
  struct Pc *curfn = (struct Pc *)0;
  struct Pc eqpc = g_pc;
  enum TokenType t = g_pc.token->type;

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      if (t == T_EQ)
        {
          if ((curfn = popLabel(L_FUNC)) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, STRAYENDEQ);
            }

          if ((eqpc.token->u.type =
               (curfn->token + 1)->u.identifier->defaultType) == V_VOID)
            {
              return Value_new_ERROR(value, STRAYENDEQ);
            }
        }
      else if (t == T_FNEND)
        {
          if ((curfn = popLabel(L_FUNC)) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, STRAYENDFN);
            }

          if ((eqpc.token->u.type =
               (curfn->token + 1)->u.identifier->defaultType) == V_VOID)
            {
              return Value_new_ERROR(value, STRAYENDFN);
            }
        }
      else
        {
          if ((curfn = findLabel(L_FUNC)) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, STRAYFNRETURN);
            }

          if ((eqpc.token->u.type =
               (curfn->token + 1)->u.identifier->defaultType) == V_VOID)
            {
              return Value_new_ERROR(value, STRAYFNRETURN);
            }
        }
    }

  ++g_pc.token;
  if (eval(value, _("return"))->type == V_ERROR ||
      Value_retype(value, eqpc.token->u.type)->type == V_ERROR)
    {
      if (g_pass != INTERPRET)
        {
          Auto_funcEnd(&g_stack);
        }

      g_pc = eqpc;
      return value;
    }

  if (g_pass == INTERPRET)
    {
      return value;
    }
  else
    {
      Value_destroy(value);
      if (t == T_EQ || t == T_FNEND)
        {
          if (g_pass == DECLARE)
            {
              Global_endfunction(&g_globals,
                                 (curfn->token + 1)->u.identifier, &g_pc);
            }

          Auto_funcEnd(&g_stack);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_ERASE(struct Value *value)
{
  ++g_pc.token;
  while (1)
    {
      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGARRIDENT);
        }

      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType, GLOBALARRAY,
                          0) == 0)
        {
          return Value_new_ERROR(value, REDECLARATION);
        }

      if (g_pass == INTERPRET)
        {
          Var_destroy(&g_pc.token->u.identifier->sym->u.var);
        }

      ++g_pc.token;
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_EXITDO(struct Value *value)
{
  if (g_pass == INTERPRET)
    {
      g_pc = g_pc.token->u.exitdo;
    }
  else
    {
      if (g_pass == COMPILE)
        {
          struct Pc *exitdo;

          if ((exitdo = findLabel(L_DO)) == (struct Pc *)0 &&
              (exitdo = findLabel(L_DOcondition)) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, STRAYEXITDO);
            }

          g_pc.token->u.exitdo = exitdo->token->u.exitdo;
        }

      ++g_pc.token;
    }

  return (struct Value *)0;
}

struct Value *stmt_EXITFOR(struct Value *value)
{
  if (g_pass == INTERPRET)
    {
      g_pc = g_pc.token->u.exitfor;
    }
  else
    {
      if (g_pass == COMPILE)
        {
          struct Pc *exitfor;

          if ((exitfor = findLabel(L_FOR)) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, STRAYEXITFOR);
            }

          g_pc.token->u.exitfor = exitfor->token->u.exitfor;
        }

      ++g_pc.token;
    }

  return (struct Value *)0;
}

struct Value *stmt_FIELD(struct Value *value)
{
  long int chn, offset, recLength = -1;

  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
    }

  if (eval(value, _("channel"))->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  chn = value->u.integer;
  Value_destroy(value);
  if (g_pass == INTERPRET && (recLength = FS_recLength(chn)) == -1)
    {
      return Value_new_ERROR(value, IOERROR, FS_errmsg);
    }

  if (g_pc.token->type != T_COMMA)
    {
      return Value_new_ERROR(value, MISSINGCOMMA);
    }

  ++g_pc.token;
  offset = 0;
  while (1)
    {
      struct Pc curpc;
      struct Value *l;
      long int width;

      curpc = g_pc;
      if (eval(value, _("field width"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      width = value->u.integer;
      Value_destroy(value);
      if (g_pass == INTERPRET && offset + width > recLength)
        {
          g_pc = curpc;
          return Value_new_ERROR(value, OUTOFRANGE, _("field width"));
        }

      if (g_pc.token->type != T_AS)
        {
          return Value_new_ERROR(value, MISSINGAS);
        }

      ++g_pc.token;
      curpc = g_pc;
      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGVARIDENT);
        }

      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType,
                          (g_pc.token + 1)->type ==
                          T_OP ? GLOBALARRAY : GLOBALVAR, 0) == 0)
        {
          return Value_new_ERROR(value, REDECLARATION);
        }

      if ((l = lvalue(value))->type == V_ERROR)
        {
          return value;
        }

      if (g_pass != DECLARE && l->type != V_STRING)
        {
          g_pc = curpc;
          return Value_new_ERROR(value, TYPEMISMATCH4);
        }

      if (g_pass == INTERPRET)
        {
          FS_field(chn, &l->u.string, offset, width);
        }

      offset += width;
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_FOR(struct Value *value)
{
  struct Pc forpc = g_pc;
  struct Pc varpc;
  struct Pc limitpc;
  struct Value limit, stepValue;

  ++g_pc.token;
  varpc = g_pc;
  if (g_pc.token->type != T_IDENTIFIER)
    {
      return Value_new_ERROR(value, MISSINGLOOPIDENT);
    }

  if (assign(value)->type == V_ERROR)
    {
      return value;
    }

  if (g_pass == INTERPRET)
    {
      ++g_pc.token;
      if (eval(&limit, (const char *)0)->type == V_ERROR)
        {
          *value = limit;
          return value;
        }

      Value_retype(&limit, value->type);
      assert(limit.type != V_ERROR);
      if (g_pc.token->type == T_STEP)     /* STEP x */
        {
          struct Pc stepPc;

          ++g_pc.token;
          stepPc = g_pc;
          if (eval(&stepValue, "`step'")->type == V_ERROR)
            {
              Value_destroy(value);
              *value = stepValue;
              g_pc = stepPc;
              return value;
            }

          Value_retype(&stepValue, value->type);
          assert(stepValue.type != V_ERROR);
        }
      else                      /* implicit numeric STEP */
        {
          if (value->type == V_INTEGER)
            {
              VALUE_NEW_INTEGER(&stepValue, 1);
            }
          else
            {
              VALUE_NEW_REAL(&stepValue, 1.0);
            }
        }

      if (Value_exitFor(value, &limit, &stepValue))
        {
          g_pc = forpc.token->u.exitfor;
        }

      Value_destroy(&limit);
      Value_destroy(&stepValue);
      Value_destroy(value);
    }
  else
    {
      pushLabel(L_FOR, &forpc);
      pushLabel(L_FOR_VAR, &varpc);
      if (g_pc.token->type != T_TO)
        {
          Value_destroy(value);
          return Value_new_ERROR(value, MISSINGTO);
        }

      ++g_pc.token;
      pushLabel(L_FOR_LIMIT, &g_pc);
      limitpc = g_pc;
      if (eval(&limit, (const char *)0) == (struct Value *)0)
        {
          Value_destroy(value);
          return Value_new_ERROR(value, MISSINGEXPR, "`to'");
        }

      if (limit.type == V_ERROR)
        {
          Value_destroy(value);
          *value = limit;
          return value;
        }

      if (g_pass != DECLARE)
        {
          struct Symbol *sym = varpc.token->u.identifier->sym;

          if (VALUE_RETYPE
              (&limit, sym->type == GLOBALVAR ||
               sym->type == GLOBALARRAY ? sym->u.var.type :
               Auto_varType(&g_stack, sym))->type == V_ERROR)
            {
              Value_destroy(value);
              *value = limit;
              g_pc = limitpc;
              return value;
            }
        }

      Value_destroy(&limit);
      if (g_pc.token->type == T_STEP)     /* STEP x */
        {
          struct Pc stepPc;

          ++g_pc.token;
          stepPc = g_pc;
          if (eval(&stepValue, "`step'")->type == V_ERROR ||
              (g_pass != DECLARE &&
               Value_retype(&stepValue, value->type)->type == V_ERROR))
            {
              Value_destroy(value);
              *value = stepValue;
              g_pc = stepPc;
              return value;
            }
        }
      else                      /* implicit numeric STEP */
        {
          VALUE_NEW_INTEGER(&stepValue, 1);
          if (g_pass != DECLARE &&
              VALUE_RETYPE(&stepValue, value->type)->type == V_ERROR)
            {
              Value_destroy(value);
              *value = stepValue;
              Value_errorPrefix(value, _("implicit STEP 1:"));
              return value;
            }
        }

      pushLabel(L_FOR_BODY, &g_pc);
      Value_destroy(&stepValue);
      Value_destroy(value);
    }

  return (struct Value *)0;
}

struct Value *stmt_GET_PUT(struct Value *value)
{
  struct Pc statementpc = g_pc;
  int put = g_pc.token->type == T_PUT;
  long int chn;
  struct Pc errpc;

  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
    }

  if (eval(value, _("channel"))->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  chn = value->u.integer;
  Value_destroy(value);
  if (g_pc.token->type == T_COMMA)
    {
      ++g_pc.token;
      errpc = g_pc;
      if (eval(value, (const char *)0)) /* process record number/position */
        {
          int rec;

          if (value->type == V_ERROR ||
              Value_retype(value, V_INTEGER)->type == V_ERROR)
            {
              return value;
            }

          rec = value->u.integer;
          Value_destroy(value);
          if (g_pass == INTERPRET)
            {
              if (rec < 1)
                {
                  g_pc = errpc;
                  return Value_new_ERROR(value, OUTOFRANGE,
                                         _("record number"));
                }

              if (FS_seek((int)chn, rec - 1) == -1)
                {
                  g_pc = statementpc;
                  return Value_new_ERROR(value, IOERROR, FS_errmsg);
                }
            }
        }
    }

  if (g_pc.token->type == T_COMMA)        /* BINARY mode get/put */
    {
      int res = -1;

      ++g_pc.token;
      if (put)
        {
          if (eval(value, _("`put'/`get' data"))->type == V_ERROR)
            {
              return value;
            }

          if (g_pass == INTERPRET)
            {
              switch (value->type)
                {
                case V_INTEGER:
                  res = FS_putbinaryInteger(chn, value->u.integer);
                  break;

                case V_REAL:
                  res = FS_putbinaryReal(chn, value->u.real);
                  break;

                case V_STRING:
                  res = FS_putbinaryString(chn, &value->u.string);
                  break;

                default:
                  assert(0);
                }
            }

          Value_destroy(value);
        }
      else
        {
          struct Value *l;

          if (g_pc.token->type != T_IDENTIFIER)
            {
              return Value_new_ERROR(value, MISSINGPROCIDENT);
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

          if (g_pass == INTERPRET)
            {
              switch (l->type)
                {
                case V_INTEGER:
                  res = FS_getbinaryInteger(chn, &l->u.integer);
                  break;

                case V_REAL:
                  res = FS_getbinaryReal(chn, &l->u.real);
                  break;

                case V_STRING:
                  res = FS_getbinaryString(chn, &l->u.string);
                  break;

                default:
                  assert(0);
                }
            }
        }

      if (g_pass == INTERPRET && res == -1)
        {
          g_pc = statementpc;
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
    }
  else if (g_pass == INTERPRET && ((put ? FS_put : FS_get) (chn)) == -1)
    {
      g_pc = statementpc;
      return Value_new_ERROR(value, IOERROR, FS_errmsg);
    }

  return (struct Value *)0;
}

struct Value *stmt_GOSUB(struct Value *value)
{
  if (g_pass == INTERPRET)
    {
      if (!g_program.runnable &&
          compileProgram(value, !DIRECTMODE)->type == V_ERROR)
        {
          return value;
        }

      g_pc.token += 2;
      Auto_pushGosubRet(&g_stack, &g_pc);
      g_pc = (g_pc.token - 2)->u.gosubpc;
      Program_trace(&g_program, &g_pc, 0, 1);
    }

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      struct Token *gosubpc = g_pc.token;

      ++g_pc.token;
      if (g_pc.token->type != T_INTEGER)
        {
          return Value_new_ERROR(value, MISSINGLINENUMBER);
        }

      if (Program_goLine(&g_program, g_pc.token->u.integer,
                         &gosubpc->u.gosubpc) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, NOSUCHLINE);
        }

      if (g_pass == COMPILE &&
          Program_scopeCheck(&g_program, &gosubpc->u.gosubpc,
                             findLabel(L_FUNC)))
        {
          return Value_new_ERROR(value, OUTOFSCOPE);
        }

      ++g_pc.token;
    }

  return (struct Value *)0;
}

struct Value *stmt_RESUME_GOTO(struct Value *value)
{
  if (g_pass == INTERPRET)
    {
      if (!g_program.runnable &&
          compileProgram(value, !DIRECTMODE)->type == V_ERROR)
        {
          return value;
        }

      if (g_pc.token->type == T_RESUME)
        {
          if (!g_stack.resumeable)
            {
              return Value_new_ERROR(value, STRAYRESUME);
            }

          g_stack.resumeable = 0;
        }

      g_pc = g_pc.token->u.gotopc;
      Program_trace(&g_program, &g_pc, 0, 1);
    }
  else if (g_pass == DECLARE || g_pass == COMPILE)
    {
      struct Token *gotopc = g_pc.token;

      ++g_pc.token;
      if (g_pc.token->type != T_INTEGER)
        {
          return Value_new_ERROR(value, MISSINGLINENUMBER);
        }

      if (Program_goLine(&g_program, g_pc.token->u.integer,
                         &gotopc->u.gotopc) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, NOSUCHLINE);
        }

      if (g_pass == COMPILE &&
          Program_scopeCheck(&g_program, &gotopc->u.gotopc,
                             findLabel(L_FUNC)))
        {
          return Value_new_ERROR(value, OUTOFSCOPE);
        }

      ++g_pc.token;
    }

  return (struct Value *)0;
}

struct Value *stmt_KILL(struct Value *value)
{
  struct Pc statementpc = g_pc;

  ++g_pc.token;
  if (eval(value, _("file name"))->type == V_ERROR ||
      (g_pass != DECLARE && Value_retype(value, V_STRING)->type == V_ERROR))
    {
      return value;
    }

  if (g_pass == INTERPRET && unlink(value->u.string.character) == -1)
    {
      const char *msg = strerror(errno);

      Value_destroy(value);
      g_pc = statementpc;
      return Value_new_ERROR(value, IOERROR, msg);
    }
  else
    {
      Value_destroy(value);
    }

  return (struct Value *)0;
}

struct Value *stmt_LET(struct Value *value)
{
  ++g_pc.token;
  if (g_pc.token->type != T_IDENTIFIER)
    {
      return Value_new_ERROR(value, MISSINGVARIDENT);
    }

  if (assign(value)->type == V_ERROR)
    {
      return value;
    }

  if (g_pass != INTERPRET)
    {
      Value_destroy(value);
    }

  return (struct Value *)0;
}

struct Value *stmt_LINEINPUT(struct Value *value)
{
  int channel = 0;
  struct Pc lpc;
  struct Value *l;

  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
      if (eval(value, _("channel"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      channel = value->u.integer;
      Value_destroy(value);
      if (g_pc.token->type != T_COMMA)
        {
          return Value_new_ERROR(value, MISSINGCOMMA);
        }
      else
        {
          ++g_pc.token;
        }
    }

  /* prompt */

  if (g_pc.token->type == T_STRING)
    {
      if (g_pass == INTERPRET && channel == 0)
        {
          FS_putString(channel, g_pc.token->u.string);
        }

      ++g_pc.token;
      if (g_pc.token->type != T_SEMICOLON && g_pc.token->type != T_COMMA)
        {
          return Value_new_ERROR(value, MISSINGSEMICOMMA);
        }

      ++g_pc.token;
    }

  if (g_pass == INTERPRET && channel == 0)
    {
      FS_flush(channel);
    }

  if (g_pc.token->type != T_IDENTIFIER)
    {
      return Value_new_ERROR(value, MISSINGVARIDENT);
    }

  if (g_pass == DECLARE &&
      Global_variable(&g_globals, g_pc.token->u.identifier,
                      g_pc.token->u.identifier->defaultType,
                      (g_pc.token + 1)->type == T_OP ? GLOBALARRAY :
                      GLOBALVAR, 0) == 0)
    {
      return Value_new_ERROR(value, REDECLARATION);
    }

  lpc = g_pc;
  if (((l = lvalue(value))->type) == V_ERROR)
    {
      return value;
    }

  if (g_pass == COMPILE && l->type != V_STRING)
    {
      g_pc = lpc;
      return Value_new_ERROR(value, TYPEMISMATCH4);
    }

  if (g_pass == INTERPRET)
    {
      String_size(&l->u.string, 0);
      if (FS_appendToString(channel, &l->u.string, 1) == -1)
        {
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }

      if (l->u.string.length == 0)
        {
          return Value_new_ERROR(value, IOERROR, _("end of file"));
        }

      if (l->u.string.character[l->u.string.length - 1] == '\n')
        {
          String_size(&l->u.string, l->u.string.length - 1);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_LIST_LLIST(struct Value *value)
{
  struct Pc from, to;
  int f = 0, t = 0, channel;

  channel = (g_pc.token->type == T_LLIST ? LPCHANNEL : STDCHANNEL);
  ++g_pc.token;
  if (g_pc.token->type == T_INTEGER)
    {
      if (g_pass == INTERPRET &&
          Program_fromLine(&g_program, g_pc.token->u.integer,
                           &from) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, NOSUCHLINE);
        }

      f = 1;
      ++g_pc.token;
    }
  else if (g_pc.token->type != T_MINUS && g_pc.token->type != T_COMMA)
    {
      if (eval(value, (const char *)0))
        {
          if (value->type == V_ERROR ||
              (g_pass != DECLARE &&
               Value_retype(value, V_INTEGER)->type == V_ERROR))
            {
              return value;
            }

          if (g_pass == INTERPRET &&
              Program_fromLine(&g_program, value->u.integer,
                               &from) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, NOSUCHLINE);
            }

          f = 1;
          Value_destroy(value);
        }
    }

  if (g_pc.token->type == T_MINUS || g_pc.token->type == T_COMMA)
    {
      ++g_pc.token;
      if (eval(value, (const char *)0))
        {
          if (value->type == V_ERROR ||
              (g_pass != DECLARE &&
               Value_retype(value, V_INTEGER)->type == V_ERROR))
            {
              return value;
            }

          if (g_pass == INTERPRET &&
              !Program_toLine(&g_program, value->u.integer, &to))
            {
              return Value_new_ERROR(value, NOSUCHLINE);
            }

          t = 1;
          Value_destroy(value);
        }
    }
  else if (f == 1)
    {
      to = from;
      t  = 1;
    }

  if (g_pass == INTERPRET)
    {
      /* Some implementations do not require direct mode */

      if (Program_list
          (&g_program, channel, channel == STDCHANNEL,
           f ? &from : (struct Pc *)0, t ? &to : (struct Pc *)0, value))
        {
          return value;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_LOAD(struct Value *value)
{
  struct Pc loadpc;

  if (g_pass == INTERPRET && !DIRECTMODE)
    {
      return Value_new_ERROR(value, NOTINPROGRAMMODE);
    }

  ++g_pc.token;
  loadpc = g_pc;
  if (eval(value, _("file name"))->type == V_ERROR ||
      Value_retype(value, V_STRING)->type == V_ERROR)
    {
      g_pc = loadpc;
      return value;
    }

  if (g_pass == INTERPRET)
    {
      int dev;

      new();
      Program_setname(&g_program, value->u.string.character);
      if ((dev = FS_openin(value->u.string.character)) == -1)
        {
          g_pc = loadpc;
          Value_destroy(value);
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }

      FS_width(dev, 0);
      Value_destroy(value);
      if (Program_merge(&g_program, dev, value))
        {
          g_pc = loadpc;
          return value;
        }

      FS_close(dev);
      g_program.unsaved = 0;
    }
  else
    {
      Value_destroy(value);
    }

  return (struct Value *)0;
}

struct Value *stmt_LOCAL(struct Value *value)
{
  struct Pc *curfn = (struct Pc *)0;

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      if ((curfn = findLabel(L_FUNC)) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, STRAYLOCAL);
        }
    }

  ++g_pc.token;
  while (1)
    {
      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGVARIDENT);
        }

      if (g_pass == DECLARE || g_pass == COMPILE)
        {
          struct Symbol *fnsym;

          if (Auto_variable(&g_stack, g_pc.token->u.identifier) == 0)
            {
              return Value_new_ERROR(value, ALREADYLOCAL);
            }

          if (g_pass == DECLARE)
            {
              assert(curfn->token->type == T_DEFFN ||
                     curfn->token->type == T_DEFPROC ||
                     curfn->token->type == T_SUB ||
                     curfn->token->type == T_FUNCTION);
              fnsym = (curfn->token + 1)->u.identifier->sym;
              assert(fnsym);
              fnsym->u.sub.u.def.localTypes =
                realloc(fnsym->u.sub.u.def.localTypes,
                        sizeof(enum ValueType) *
                        (fnsym->u.sub.u.def.localLength + 1));
              fnsym->u.sub.u.def.localTypes[fnsym->u.sub.u.def.localLength] =
                g_pc.token->u.identifier->defaultType;
              ++fnsym->u.sub.u.def.localLength;
            }
        }

      ++g_pc.token;
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_LOCATE(struct Value *value)
{
  long int line, column;
  struct Pc argpc;
  struct Pc statementpc = g_pc;

  ++g_pc.token;
  argpc = g_pc;
  if (eval(value, _("row"))->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  line = value->u.integer;
  Value_destroy(value);
  if (g_pass == INTERPRET && line < 1)
    {
      g_pc = argpc;
      return Value_new_ERROR(value, OUTOFRANGE, _("row"));
    }

  if (g_pc.token->type == T_COMMA)
    {
      ++g_pc.token;
    }
  else
    {
      return Value_new_ERROR(value, MISSINGCOMMA);
    }

  argpc = g_pc;
  if (eval(value, _("column"))->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  column = value->u.integer;
  Value_destroy(value);
  if (g_pass == INTERPRET && column < 1)
    {
      g_pc = argpc;
      return Value_new_ERROR(value, OUTOFRANGE, _("column"));
    }

  if (g_pass == INTERPRET && FS_locate(STDCHANNEL, line, column) == -1)
    {
      g_pc = statementpc;
      return Value_new_ERROR(value, IOERROR, FS_errmsg);
    }

  return (struct Value *)0;
}

struct Value *stmt_LOCK_UNLOCK(struct Value *value)
{
  int lock = g_pc.token->type == T_LOCK;
  int channel;

  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
    }

  if (eval(value, _("channel"))->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  channel = value->u.integer;
  Value_destroy(value);
  if (g_pass == INTERPRET)
    {
      if (FS_lock(channel, 0, 0, lock ? FS_LOCK_EXCLUSIVE : FS_LOCK_NONE, 1)
          == -1)
        {
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_LOOP(struct Value *value)
{
  struct Pc looppc = g_pc;
  struct Pc *dopc;

  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      g_pc = looppc.token->u.dopc;
    }

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      if ((dopc = popLabel(L_DO)) == (struct Pc *)0 &&
          (dopc = popLabel(L_DOcondition)) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, STRAYLOOP);
        }

      looppc.token->u.dopc = *dopc;
      dopc->token->u.exitdo = g_pc;
    }

  return (struct Value *)0;
}

struct Value *stmt_LOOPUNTIL(struct Value *value)
{
  struct Pc loopuntilpc = g_pc;
  struct Pc *dopc;

  ++g_pc.token;
  if (eval(value, _("condition"))->type == V_ERROR)
    {
      return value;
    }

  if (g_pass == INTERPRET)
    {
      if (Value_isNull(value))
        g_pc = loopuntilpc.token->u.dopc;
      Value_destroy(value);
    }

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      if ((dopc = popLabel(L_DO)) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, STRAYLOOPUNTIL);
        }

      loopuntilpc.token->u.until = *dopc;
      dopc->token->u.exitdo = g_pc;
    }

  return (struct Value *)0;
}

struct Value *stmt_LSET_RSET(struct Value *value)
{
  struct Value *l;
  struct Pc tmppc;
  int lset = (g_pc.token->type == T_LSET);

  ++g_pc.token;
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

  tmppc = g_pc;
  if ((l = lvalue(value))->type == V_ERROR)
    {
      return value;
    }

  if (g_pass == COMPILE && l->type != V_STRING)
    {
      g_pc = tmppc;
      return Value_new_ERROR(value, TYPEMISMATCH4);
    }

  if (g_pc.token->type != T_EQ)
    {
      return Value_new_ERROR(value, MISSINGEQ);
    }

  ++g_pc.token;
  tmppc = g_pc;
  if (eval(value, _("rhs"))->type == V_ERROR ||
      (g_pass != DECLARE && Value_retype(value, l->type)->type == V_ERROR))
    {
      g_pc = tmppc;
      return value;
    }

  if (g_pass == INTERPRET)
    {
      (lset ? String_lset : String_rset) (&l->u.string, &value->u.string);
    }

  Value_destroy(value);
  return (struct Value *)0;
}

struct Value *stmt_IDENTIFIER(struct Value *value)
{
  struct Pc here = g_pc;

  if (g_pass == DECLARE)
    {
      if (func(value)->type == V_ERROR)
        {
          return value;
        }
      else
        {
          Value_destroy(value);
        }

      if (g_pc.token->type == T_EQ || g_pc.token->type == T_COMMA)
        {
          g_pc = here;
          if (assign(value)->type == V_ERROR)
            {
              return value;
            }

          Value_destroy(value);
        }
    }
  else
    {
      if (g_pass == COMPILE)
        {
          if (((g_pc.token + 1)->type == T_OP ||
               Auto_find(&g_stack, g_pc.token->u.identifier) == 0) &&
              Global_find(&g_globals, g_pc.token->u.identifier,
                          (g_pc.token + 1)->type == T_OP) == 0)
            {
              return Value_new_ERROR(value, UNDECLARED);
            }
        }

      if (strcasecmp(g_pc.token->u.identifier->name, "mid$")
          && (g_pc.token->u.identifier->sym->type == USERFUNCTION ||
              g_pc.token->u.identifier->sym->type == BUILTINFUNCTION))
        {
          func(value);
          if (Value_retype(value, V_VOID)->type == V_ERROR)
            {
              return value;
            }

          Value_destroy(value);
        }
      else
        {
          if (assign(value)->type == V_ERROR)
            {
              return value;
            }

          if (g_pass != INTERPRET)
            {
              Value_destroy(value);
            }
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_IF_ELSEIFIF(struct Value *value)
{
  struct Pc ifpc = g_pc;

  ++g_pc.token;
  if (eval(value, _("condition"))->type == V_ERROR)
    {
      return value;
    }

  if (g_pc.token->type != T_THEN)
    {
      Value_destroy(value);
      return Value_new_ERROR(value, MISSINGTHEN);
    }

  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      if (Value_isNull(value))
        {
          g_pc = ifpc.token->u.elsepc;
        }

      Value_destroy(value);
    }
  else
    {
      Value_destroy(value);
      if (g_pc.token->type == T_EOL)
        {
          pushLabel(L_IF, &ifpc);
        }
      else                      /* compile single line IF THEN ELSE recursively */
        {
          if (statements(value)->type == V_ERROR)
            {
              return value;
            }

          Value_destroy(value);
          if (g_pc.token->type == T_ELSE)
            {
              struct Pc elsepc = g_pc;

              ++g_pc.token;
              ifpc.token->u.elsepc = g_pc;
              if (ifpc.token->type == T_ELSEIFIF)
                {
                  (ifpc.token - 1)->u.elsepc = g_pc;
                }

              if (statements(value)->type == V_ERROR)
                {
                  return value;
                }

              Value_destroy(value);
              elsepc.token->u.endifpc = g_pc;
            }
          else
            {
              ifpc.token->u.elsepc = g_pc;
              if (ifpc.token->type == T_ELSEIFIF)
                {
                  (ifpc.token - 1)->u.elsepc = g_pc;
                }
            }
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_IMAGE(struct Value *value)
{
  ++g_pc.token;
  if (g_pc.token->type != T_STRING)
    {
      return Value_new_ERROR(value, MISSINGFMT);
    }

  ++g_pc.token;
  return (struct Value *)0;
}

struct Value *stmt_INPUT(struct Value *value)
{
  int channel = STDCHANNEL;
  int nl = 1;
  int extraprompt = 1;
  struct Token *inputdata = (struct Token *)0, *t = (struct Token *)0;
  struct Pc lvaluepc;

  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
      if (eval(value, _("channel"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      channel = value->u.integer;
      Value_destroy(value);
      if (g_pc.token->type != T_COMMA)
        {
          return Value_new_ERROR(value, MISSINGCOMMA);
        }
      else
        {
          ++g_pc.token;
        }
    }

  if (g_pc.token->type == T_SEMICOLON)
    {
      nl = 0;
      ++g_pc.token;
    }

  /* prompt */

  if (g_pc.token->type == T_STRING)
    {
      if (g_pass == INTERPRET && channel == STDCHANNEL)
        {
          FS_putString(STDCHANNEL, g_pc.token->u.string);
        }

      ++g_pc.token;
      if (g_pc.token->type == T_COMMA || g_pc.token->type == T_COLON)
        {
          ++g_pc.token;
          extraprompt = 0;
        }
      else if (g_pc.token->type == T_SEMICOLON)
        {
          ++g_pc.token;
        }
      else
        {
          extraprompt = 0;
        }
    }

  if (g_pass == INTERPRET && channel == STDCHANNEL && extraprompt)
    {
      FS_putChars(STDCHANNEL, "? ");
    }

retry:
  if (g_pass == INTERPRET)        /* read input line and tokenise it */
    {
      struct String s;

      if (channel == STDCHANNEL)
        {
          FS_flush(STDCHANNEL);
        }

      String_new(&s);
      if (FS_appendToString(channel, &s, nl) == -1)
        {
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }

      if (s.length == 0)
        {
          return Value_new_ERROR(value, IOERROR, _("end of file"));
        }

      inputdata = t = Token_newData(s.character);
      String_destroy(&s);
    }

  while (1)
    {
      struct Value *l;

      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGVARIDENT);
        }

      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType,
                          (g_pc.token + 1)->type ==
                          T_OP ? GLOBALARRAY : GLOBALVAR, 0) == 0)
        {
          return Value_new_ERROR(value, REDECLARATION);
        }

      lvaluepc = g_pc;
      if (((l = lvalue(value))->type) == V_ERROR)
        {
          return value;
        }

      if (g_pass == INTERPRET)
        {
          if (t->type == T_COMMA || t->type == T_EOL)
            {
              enum ValueType ltype = l->type;

              Value_destroy(l);
              Value_new_null(l, ltype);
            }
          else if (convert(value, l, t))
            {
              g_pc = lvaluepc;
              if (channel == STDCHANNEL)
                {
                  struct String s;

                  String_new(&s);
                  Value_toString(value, &s, ' ', -1, 0, 0, 0, 0, -1, 0, 0);
                  String_appendChars(&s, " ?? ");
                  FS_putString(STDCHANNEL, &s);
                  String_destroy(&s);
                  Value_destroy(value);
                  Token_destroy(inputdata);
                  goto retry;
                }
              else
                {
                  Token_destroy(inputdata);
                  return value;
                }
            }
          else
            {
              ++t;
            }

          if (g_pc.token->type == T_COMMA)
            {
              if (t->type == T_COMMA)
                {
                  ++t;
                }
              else
                {
                  Token_destroy(inputdata);
                  if (channel == STDCHANNEL)
                    {
                      FS_putChars(STDCHANNEL, "?? ");
                      ++g_pc.token;
                      goto retry;
                    }
                  else
                    {
                      g_pc = lvaluepc;
                      return Value_new_ERROR(value, MISSINGINPUTDATA);
                    }
                }
            }
        }

      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  if (g_pass == INTERPRET)
    {
      if (t->type != T_EOL)
        {
          FS_putChars(STDCHANNEL, _("Too much input data\n"));
        }

      Token_destroy(inputdata);
    }

  return (struct Value *)0;
}

struct Value *stmt_MAT(struct Value *value)
{
  struct Var *var1, *var2, *var3 = (struct Var *)0;
  struct Pc oppc;
  enum TokenType op = T_EOL;

  oppc.line = -1;
  oppc.token = (struct Token *)0;
  ++g_pc.token;
  if (g_pc.token->type != T_IDENTIFIER)
    {
      return Value_new_ERROR(value, MISSINGMATIDENT);
    }

  if (g_pass == DECLARE &&
      !Global_variable(&g_globals, g_pc.token->u.identifier,
                      g_pc.token->u.identifier->defaultType, GLOBALARRAY, 0))
    {
      return Value_new_ERROR(value, REDECLARATION);
    }

  var1 = &g_pc.token->u.identifier->sym->u.var;
  ++g_pc.token;
  if (g_pc.token->type != T_EQ)
    {
      return Value_new_ERROR(value, MISSINGEQ);
    }

  ++g_pc.token;
  if (g_pc.token->type == T_IDENTIFIER)   /* a = b [ +|-|* c ] */
    {
      if (g_pass == COMPILE)
        {
          if (((g_pc.token + 1)->type == T_OP ||
               Auto_find(&g_stack, g_pc.token->u.identifier) == 0) &&
              Global_find(&g_globals, g_pc.token->u.identifier, 1) == 0)
            {
              return Value_new_ERROR(value, UNDECLARED);
            }
        }

      var2 = &g_pc.token->u.identifier->sym->u.var;
      if (g_pass == INTERPRET &&
          ((var2->dim != 1 && var2->dim != 2) || var2->base < 0 ||
           var2->base > 1))
        {
          return Value_new_ERROR(value, NOMATRIX, var2->dim, var2->base);
        }

      if (g_pass == COMPILE &&
          Value_commonType[var1->type][var2->type] == V_ERROR)
        {
          return Value_new_typeError(value, var2->type, var1->type);
        }

      ++g_pc.token;
      if (g_pc.token->type == T_PLUS || g_pc.token->type == T_MINUS ||
          g_pc.token->type == T_MULT)
        {
          oppc = g_pc;
          op = g_pc.token->type;
          ++g_pc.token;
          if (g_pc.token->type != T_IDENTIFIER)
            {
              return Value_new_ERROR(value, MISSINGARRIDENT);
            }

          if (g_pass == COMPILE)
            {
              if (((g_pc.token + 1)->type == T_OP ||
                   Auto_find(&g_stack, g_pc.token->u.identifier) == 0) &&
                  Global_find(&g_globals, g_pc.token->u.identifier, 1) == 0)
                {
                  return Value_new_ERROR(value, UNDECLARED);
                }
            }

          var3 = &g_pc.token->u.identifier->sym->u.var;
          if (g_pass == INTERPRET &&
              ((var3->dim != 1 && var3->dim != 2) || var3->base < 0 ||
               var3->base > 1))
            {
              return Value_new_ERROR(value, NOMATRIX, var3->dim, var3->base);
            }

          ++g_pc.token;
        }

      if (g_pass != DECLARE)
        {
          if (var3 == (struct Var *)0)
            {
              if (Var_mat_assign(var1, var2, value, g_pass == INTERPRET))
                {
                  assert(oppc.line != -1);
                  g_pc = oppc;
                  return value;
                }
            }
          else if (op == T_MULT)
            {
              if (Var_mat_mult(var1, var2, var3, value, g_pass == INTERPRET))
                {
                  assert(oppc.line != -1);
                  g_pc = oppc;
                  return value;
                }
            }
          else if (Var_mat_addsub(var1, var2, var3, op == T_PLUS,
                                  value, g_pass == INTERPRET))
            {
              assert(oppc.line != -1);
              g_pc = oppc;
              return value;
            }
        }
    }
  else if (g_pc.token->type == T_OP)
    {
      if (var1->type == V_STRING)
        {
          return Value_new_ERROR(value, TYPEMISMATCH5);
        }

      ++g_pc.token;
      if (eval(value, _("factor"))->type == V_ERROR)
        {
          return value;
        }

      if (g_pass == COMPILE &&
          Value_commonType[var1->type][value->type] == V_ERROR)
        {
          return Value_new_typeError(value, var1->type, value->type);
        }

      if (g_pc.token->type != T_CP)
        {
          Value_destroy(value);
          return Value_new_ERROR(value, MISSINGCP);
        }

      ++g_pc.token;
      if (g_pc.token->type != T_MULT)
        {
          Value_destroy(value);
          return Value_new_ERROR(value, MISSINGMULT);
        }

      oppc = g_pc;
      ++g_pc.token;
      if (g_pass == COMPILE)
        {
          if (((g_pc.token + 1)->type == T_OP ||
               Auto_find(&g_stack, g_pc.token->u.identifier) == 0) &&
              Global_find(&g_globals, g_pc.token->u.identifier, 1) == 0)
            {
              Value_destroy(value);
              return Value_new_ERROR(value, UNDECLARED);
            }
        }

      var2 = &g_pc.token->u.identifier->sym->u.var;
      if (g_pass == INTERPRET &&
          ((var2->dim != 1 && var2->dim != 2) || var2->base < 0 ||
           var2->base > 1))
        {
          Value_destroy(value);
          return Value_new_ERROR(value, NOMATRIX, var2->dim, var2->base);
        }

      if (g_pass != DECLARE &&
          Var_mat_scalarMult(var1, value, var2, g_pass == INTERPRET))
        {
          assert(oppc.line != -1);
          g_pc = oppc;
          return value;
        }

      Value_destroy(value);
      ++g_pc.token;
    }

  else if (g_pc.token->type == T_CON || g_pc.token->type == T_ZER ||
           g_pc.token->type == T_IDN)
    {
      op = g_pc.token->type;
      if (g_pass == COMPILE &&
          Value_commonType[var1->type][V_INTEGER] == V_ERROR)
        {
          return Value_new_typeError(value, V_INTEGER, var1->type);
        }

      ++g_pc.token;
      if (g_pc.token->type == T_OP)
        {
          unsigned int dim, geometry[2];
          enum ValueType vartype = var1->type;

          ++g_pc.token;
          if (evalGeometry(value, &dim, geometry))
            {
              return value;
            }

          if (g_pass == INTERPRET)
            {
              Var_destroy(var1);
              Var_new(var1, vartype, dim, geometry, g_optionbase);
            }
        }

      if (g_pass == INTERPRET)
        {
          unsigned int i;
          int unused = 1 - var1->base;

          if ((var1->dim != 1 && var1->dim != 2) || var1->base < 0 ||
              var1->base > 1)
            {
              return Value_new_ERROR(value, NOMATRIX, var1->dim, var1->base);
            }

          if (var1->dim == 1)
            {
              for (i = unused; i < var1->geometry[0]; ++i)
                {
                  int c = -1;

                  Value_destroy(&(var1->value[i]));
                  switch (op)
                    {
                    case T_CON:
                      c = 1;
                      break;

                    case T_ZER:
                      c = 0;
                      break;

                    case T_IDN:
                      c = (i == unused ? 1 : 0);
                      break;

                    default:
                      assert(0);
                    }

                  if (var1->type == V_INTEGER)
                    {
                      Value_new_INTEGER(&(var1->value[i]), c);
                    }
                  else
                    {
                      Value_new_REAL(&(var1->value[i]), (double)c);
                    }
                }
            }
          else
            {
              int j;

              for (i = unused; i < var1->geometry[0]; ++i)
                {
                  for (j = unused; j < var1->geometry[1]; ++j)
                    {
                      int c = -1;

                      Value_destroy(&var1->value[i * var1->geometry[1] + j]);
                      switch (op)
                        {
                        case T_CON:
                          c = 1;
                          break;

                        case T_ZER:
                          c = 0;
                          break;

                        case T_IDN:
                          c = (i == j ? 1 : 0);
                          break;

                        default:
                          assert(0);
                        }

                      if (var1->type == V_INTEGER)
                        {
                          Value_new_INTEGER(&(var1->value
                                             [i * var1->geometry[1] + j]),
                                             c);
                        }
                      else
                        {
                          Value_new_REAL(&(var1->
                                          value[i * var1->geometry[1] + j]),
                                         (double)c);
                        }
                    }
                }
            }
        }
    }

  else if (g_pc.token->type == T_TRN || g_pc.token->type == T_INV)
    {
      op = g_pc.token->type;
      ++g_pc.token;
      if (g_pc.token->type != T_OP)
        {
          return Value_new_ERROR(value, MISSINGOP);
        }

      ++g_pc.token;
      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGMATIDENT);
        }

      if (g_pass == COMPILE)
        {
          if (((g_pc.token + 1)->type == T_OP ||
               Auto_find(&g_stack, g_pc.token->u.identifier) == 0) &&
              Global_find(&g_globals, g_pc.token->u.identifier, 1) == 0)
            {
              return Value_new_ERROR(value, UNDECLARED);
            }
        }

      var2 = &g_pc.token->u.identifier->sym->u.var;
      if (g_pass == COMPILE &&
          Value_commonType[var1->type][var2->type] == V_ERROR)
        {
          return Value_new_typeError(value, var2->type, var1->type);
        }

      if (g_pass == INTERPRET)
        {
          if (var2->dim != 2 || var2->base < 0 || var2->base > 1)
            {
              return Value_new_ERROR(value, NOMATRIX, var2->dim, var2->base);
            }

          switch (op)
            {
            case T_TRN:
              Var_mat_transpose(var1, var2);
              break;

            case T_INV:
              if (Var_mat_invert(var1, var2, &g_stack.lastdet, value))
                {
                  return value;
                }

              break;

            default:
              assert(0);
            }
        }

      ++g_pc.token;
      if (g_pc.token->type != T_CP)
        {
          return Value_new_ERROR(value, MISSINGCP);
        }

      ++g_pc.token;
    }
  else
    {
      return Value_new_ERROR(value, MISSINGEXPR, _("matrix"));
    }

  return (struct Value *)0;
}

struct Value *stmt_MATINPUT(struct Value *value)
{
  int channel = STDCHANNEL;

  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
      if (eval(value, _("channel"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      channel = value->u.integer;
      Value_destroy(value);
      if (g_pc.token->type != T_COMMA)
        {
          return Value_new_ERROR(value, MISSINGCOMMA);
        }
      else
        {
          ++g_pc.token;
        }
    }

  while (1)
    {
      struct Pc lvaluepc;
      struct Var *var;

      lvaluepc = g_pc;
      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGMATIDENT);
        }

      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType, GLOBALARRAY,
                          0) == 0)
        {
          return Value_new_ERROR(value, REDECLARATION);
        }

      var = &g_pc.token->u.identifier->sym->u.var;
      ++g_pc.token;
      if (g_pc.token->type == T_OP)
        {
          unsigned int dim, geometry[2];
          enum ValueType vartype = var->type;

          ++g_pc.token;
          if (evalGeometry(value, &dim, geometry))
            {
              return value;
            }

          if (g_pass == INTERPRET)
            {
              Var_destroy(var);
              Var_new(var, vartype, dim, geometry, g_optionbase);
            }
        }

      if (g_pass == INTERPRET)
        {
          unsigned int i, j;
          int unused = 1 - var->base;
          int columns;
          struct Token *inputdata, *t;

          if (var->dim != 1 && var->dim != 2)
            {
              return Value_new_ERROR(value, DIMENSION);
            }

          columns = var->dim == 1 ? 0 : var->geometry[1];
          inputdata = t = (struct Token *)0;
          for (i = unused, j = unused; i < var->geometry[0]; )
            {
              struct String s;

              if (!inputdata)
                {
                  if (channel == STDCHANNEL)
                    {
                      FS_putChars(STDCHANNEL, "? ");
                      FS_flush(STDCHANNEL);
                    }

                  String_new(&s);
                  if (FS_appendToString(channel, &s, 1) == -1)
                    {
                      return Value_new_ERROR(value, IOERROR, FS_errmsg);
                    }

                  if (s.length == 0)
                    {
                      return Value_new_ERROR(value, IOERROR,
                                             _("end of file"));
                    }

                  inputdata = t = Token_newData(s.character);
                  String_destroy(&s);
                }

              if (t->type == T_COMMA)
                {
                  Value_destroy(&(var->value[j * columns + i]));
                  Value_new_null(&(var->value[j * columns + i]), var->type);
                  ++t;
                }
              else if (t->type == T_EOL)
                {
                  while (i < var->geometry[0])
                    {
                      Value_destroy(&(var->value[j * columns + i]));
                      Value_new_null(&(var->value[j * columns + i]),
                                     var->type);
                      ++i;
                    }
                }
              else if (convert(value, &(var->value[j * columns + i]), t))
                {
                  Token_destroy(inputdata);
                  g_pc = lvaluepc;
                  return value;
                }
              else
                {
                  ++t;
                  ++i;
                  if (t->type == T_COMMA)
                    {
                      ++t;
                    }
                }

              if (i == var->geometry[0] && j < (columns - 1))
                {
                  i = unused;
                  ++j;
                  if (t->type == T_EOL)
                    {
                      Token_destroy(inputdata);
                      inputdata = (struct Token *)0;
                    }
                }
            }
        }

      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_MATPRINT(struct Value *value)
{
  int chn = STDCHANNEL;
  int printusing = 0;
  struct Value usingval;
  struct String *using = (struct String *)0;
  size_t usingpos = 0;
  int notfirst = 0;

  ++g_pc.token;
  if (chn == STDCHANNEL && g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
      if (eval(value, _("channel"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      chn = value->u.integer;
      Value_destroy(value);
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
    }

  if (g_pc.token->type == T_USING)
    {
      struct Pc usingpc;

      usingpc = g_pc;
      printusing = 1;
      ++g_pc.token;
      if (g_pc.token->type == T_INTEGER)
        {
          if (g_pass == COMPILE &&
              Program_imageLine(&g_program, g_pc.token->u.integer,
                                &usingpc.token->u.image) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, NOSUCHIMAGELINE);
            }
          else if (g_pass == INTERPRET)
            {
              using = usingpc.token->u.image.token->u.string;
            }

          Value_new_STRING(&usingval);
          ++g_pc.token;
        }
      else
        {
          if (eval(&usingval, _("format string"))->type == V_ERROR ||
              Value_retype(&usingval, V_STRING)->type == V_ERROR)
            {
              *value = usingval;
              return value;
            }

          using = &usingval.u.string;
        }

      if (g_pc.token->type != T_SEMICOLON)
        {
          Value_destroy(&usingval);
          return Value_new_ERROR(value, MISSINGSEMICOLON);
        }

      ++g_pc.token;
    }
  else
    {
      Value_new_STRING(&usingval);
      using = &usingval.u.string;
    }
  while (1)
    {
      struct Var *var;
      int zoned = 1;

      if (g_pc.token->type != T_IDENTIFIER)
        {
          if (notfirst)
            {
              break;
            }

          Value_destroy(&usingval);
          return Value_new_ERROR(value, MISSINGMATIDENT);
        }

      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType, GLOBALARRAY,
                          0) == 0)
        {
          Value_destroy(&usingval);
          return Value_new_ERROR(value, REDECLARATION);
        }

      var = &g_pc.token->u.identifier->sym->u.var;
      ++g_pc.token;
      if (g_pc.token->type == T_SEMICOLON)
        {
          zoned = 0;
        }

      if (g_pass == INTERPRET)
        {
          unsigned int i, j;
          int unused = 1 - var->base;
          int g0, g1;

          if ((var->dim != 1 && var->dim != 2) || var->base < 0 ||
              var->base > 1)
            {
              return Value_new_ERROR(value, NOMATRIX, var->dim, var->base);
            }

          if ((notfirst ? FS_putChar(chn, '\n') : FS_nextline(chn)) == -1)
            {
              Value_destroy(&usingval);
              return Value_new_ERROR(value, IOERROR, FS_errmsg);
            }

          g0 = var->geometry[0];
          g1 = var->dim == 1 ? unused + 1 : var->geometry[1];
          for (i = unused; i < g0; ++i)
            {
              for (j = unused; j < g1; ++j)
                {
                  struct String s;

                  String_new(&s);
                  Value_clone(value,
                              &(var->value[var->dim == 1 ? i : i * g1 + j]));
                  if (Value_toStringUsing(value, &s, using, &usingpos)->type
                      == V_ERROR)
                    {
                      Value_destroy(&usingval);
                      String_destroy(&s);
                      return value;
                    }

                  Value_destroy(value);
                  if (FS_putString(chn, &s) == -1)
                    {
                      Value_destroy(&usingval);
                      String_destroy(&s);
                      return Value_new_ERROR(value, IOERROR, FS_errmsg);
                    }

                  String_destroy(&s);
                  if (!printusing && zoned)
                    {
                      FS_nextcol(chn);
                    }
                }

              if (FS_putChar(chn, '\n') == -1)
                {
                  return Value_new_ERROR(value, IOERROR, FS_errmsg);
                }
            }
        }

      if (g_pc.token->type == T_COMMA || g_pc.token->type == T_SEMICOLON)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }

      notfirst = 1;
    }

  Value_destroy(&usingval);
  if (g_pass == INTERPRET)
    {
      if (FS_flush(chn) == -1)
        {
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_MATREAD(struct Value *value)
{
  ++g_pc.token;
  while (1)
    {
      struct Pc lvaluepc;
      struct Var *var;

      lvaluepc = g_pc;
      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGMATIDENT);
        }

      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType, GLOBALARRAY,
                          0) == 0)
        {
          return Value_new_ERROR(value, REDECLARATION);
        }

      var = &g_pc.token->u.identifier->sym->u.var;
      ++g_pc.token;
      if (g_pc.token->type == T_OP)
        {
          unsigned int dim, geometry[2];
          enum ValueType vartype = var->type;

          ++g_pc.token;
          if (evalGeometry(value, &dim, geometry))
            {
              return value;
            }

          if (g_pass == INTERPRET)
            {
              Var_destroy(var);
              Var_new(var, vartype, dim, geometry, g_optionbase);
            }
        }

      if (g_pass == INTERPRET)
        {
          unsigned int i;
          int unused = 1 - var->base;

          if ((var->dim != 1 && var->dim != 2) || var->base < 0 ||
              var->base > 1)
            {
              return Value_new_ERROR(value, NOMATRIX, var->dim, var->base);
            }

          if (var->dim == 1)
            {
              for (i = unused; i < var->geometry[0]; ++i)
                {
                  if (dataread(value, &(var->value[i])))
                    {
                      g_pc = lvaluepc;
                      return value;
                    }
                }
            }
          else
            {
              int j;

              for (i = unused; i < var->geometry[0]; ++i)
                {
                  for (j = unused; j < var->geometry[1]; ++j)
                    {
                      if (dataread
                          (value, &(var->value[i * var->geometry[1] + j])))
                        {
                          g_pc = lvaluepc;
                          return value;
                        }
                    }
                }
            }
        }

      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_MATREDIM(struct Value *value)
{
  ++g_pc.token;
  while (1)
    {
      struct Var *var;
      unsigned int dim, geometry[2];

      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGMATIDENT);
        }

      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType, GLOBALARRAY,
                          0) == 0)
        {
          return Value_new_ERROR(value, REDECLARATION);
        }

      var = &g_pc.token->u.identifier->sym->u.var;
      ++g_pc.token;
      if (g_pc.token->type != T_OP)
        {
          return Value_new_ERROR(value, MISSINGOP);
        }

      ++g_pc.token;
      if (evalGeometry(value, &dim, geometry))
        {
          return value;
        }

      if (g_pass == INTERPRET &&
          Var_mat_redim(var, dim, geometry, value) != (struct Value *)0)
        {
          return value;
        }

      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_MATWRITE(struct Value *value)
{
  int chn = STDCHANNEL;
  int notfirst = 0;
  int comma = 0;

  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
      if (eval(value, _("channel"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      chn = value->u.integer;
      Value_destroy(value);
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
    }

  while (1)
    {
      struct Var *var;

      if (g_pc.token->type != T_IDENTIFIER)
        {
          if (notfirst)
            {
              break;
            }

          return Value_new_ERROR(value, MISSINGMATIDENT);
        }

      notfirst = 1;
      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType, GLOBALARRAY,
                          0) == 0)
        {
          return Value_new_ERROR(value, REDECLARATION);
        }

      var = &g_pc.token->u.identifier->sym->u.var;
      ++g_pc.token;
      if (g_pass == INTERPRET)
        {
          unsigned int i, j;
          int unused = 1 - var->base;
          int g0, g1;

          if ((var->dim != 1 && var->dim != 2) || var->base < 0 ||
              var->base > 1)
            {
              return Value_new_ERROR(value, NOMATRIX, var->dim, var->base);
            }

          g0 = var->geometry[0];
          g1 = var->dim == 1 ? unused + 1 : var->geometry[1];
          for (i = unused; i < g0; ++i)
            {
              comma = 0;
              for (j = unused; j < g1; ++j)
                {
                  struct String s;

                  String_new(&s);
                  Value_clone(value,
                              &(var->value[var->dim == 1 ? i : i * g1 + j]));
                  if (comma)
                    {
                      String_appendChar(&s, ',');
                    }

                  if (FS_putString(chn, Value_toWrite(value, &s)) == -1)
                    {
                      Value_destroy(value);
                      return Value_new_ERROR(value, IOERROR, FS_errmsg);
                    }

                  if (FS_flush(chn) == -1)
                    {
                      return Value_new_ERROR(value, IOERROR, FS_errmsg);
                    }

                  String_destroy(&s);
                  comma = 1;
                }

              FS_putChar(chn, '\n');
            }
        }

      if (g_pc.token->type == T_COMMA || g_pc.token->type == T_SEMICOLON)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  if (g_pass == INTERPRET)
    {
      if (FS_flush(chn) == -1)
        {
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_NAME(struct Value *value)
{
  struct Pc namepc = g_pc;
  struct Value old;
  int res = -1, reserrno = -1;

  ++g_pc.token;
  if (eval(value, _("file name"))->type == V_ERROR ||
      Value_retype(value, V_STRING)->type == V_ERROR)
    {
      return value;
    }

  if (g_pc.token->type != T_AS)
    {
      Value_destroy(value);
      return Value_new_ERROR(value, MISSINGAS);
    }

  old = *value;
  ++g_pc.token;
  if (eval(value, _("file name"))->type == V_ERROR ||
      Value_retype(value, V_STRING)->type == V_ERROR)
    {
      Value_destroy(&old);
      return value;
    }

  if (g_pass == INTERPRET)
    {
      res = rename(old.u.string.character, value->u.string.character);
      reserrno = errno;
    }

  Value_destroy(&old);
  Value_destroy(value);
  if (g_pass == INTERPRET && res == -1)
    {
      g_pc = namepc;
      return Value_new_ERROR(value, IOERROR, strerror(reserrno));
    }

  return (struct Value *)0;
}

struct Value *stmt_NEW(struct Value *value)
{
  if (g_pass == INTERPRET)
    {
      if (!DIRECTMODE)
        {
          return Value_new_ERROR(value, NOTINPROGRAMMODE);
        }

      new();
    }

  ++g_pc.token;
  return (struct Value *)0;
}

struct Value *stmt_NEXT(struct Value *value)
{
  struct Next **next = &g_pc.token->u.next;
  int level = 0;

  if (g_pass == INTERPRET)
    {
      struct Value *l, inc;
      struct Pc savepc;

      ++g_pc.token;
      while (1)
        {
          /* get variable lvalue */

          savepc = g_pc;
          g_pc = (*next)[level].var;
          if ((l = lvalue(value))->type == V_ERROR)
            {
              return value;
            }

          g_pc = savepc;

          /* get limit value and increment */

          savepc = g_pc;
          g_pc = (*next)[level].limit;
          if (eval(value, _("limit"))->type == V_ERROR)
            {
              return value;
            }

          Value_retype(value, l->type);
          assert(value->type != V_ERROR);
          if (g_pc.token->type == T_STEP)
            {
              ++g_pc.token;
              if (eval(&inc, _("step"))->type == V_ERROR)
                {
                  Value_destroy(value);
                  *value = inc;
                  return value;
                }
            }
          else
            {
              VALUE_NEW_INTEGER(&inc, 1);
            }

          VALUE_RETYPE(&inc, l->type);
          assert(inc.type != V_ERROR);
          g_pc = savepc;

          Value_add(l, &inc, 1);
          if (Value_exitFor(l, value, &inc))
            {
              Value_destroy(value);
              Value_destroy(&inc);
              if (g_pc.token->type == T_IDENTIFIER)
                {
                  if (lvalue(value)->type == V_ERROR)
                    {
                      return value;
                    }

                  if (g_pc.token->type == T_COMMA)
                    {
                      ++g_pc.token;
                      ++level;
                    }
                  else
                    {
                      break;
                    }
                }
              else
                {
                  break;
                }
            }
          else
            {
              g_pc = (*next)[level].body;
              Value_destroy(value);
              Value_destroy(&inc);
              break;
            }
        }
    }
  else
    {
      struct Pc *body;

      ++g_pc.token;
      while (1)
        {
          if ((body = popLabel(L_FOR_BODY)) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, STRAYNEXT,
                                     topLabelDescription());
            }

          if (level)
            {
              struct Next *more;

              more = realloc(*next, sizeof(struct Next) * (level + 1));
              *next = more;
            }

          (*next)[level].body = *body;
          (*next)[level].limit = *popLabel(L_FOR_LIMIT);
          (*next)[level].var = *popLabel(L_FOR_VAR);
          (*next)[level].fr = *popLabel(L_FOR);
          if (g_pc.token->type == T_IDENTIFIER)
            {
              if (cistrcmp
                  (g_pc.token->u.identifier->name,
                   (*next)[level].var.token->u.identifier->name))
                {
                  return Value_new_ERROR(value, FORMISMATCH);
                }

              if (g_pass == DECLARE &&
                  Global_variable(&g_globals, g_pc.token->u.identifier,
                                  g_pc.token->u.identifier->defaultType,
                                  (g_pc.token + 1)->type ==
                                  T_OP ? GLOBALARRAY : GLOBALVAR, 0) == 0)
                {
                  return Value_new_ERROR(value, REDECLARATION);
                }

              if (lvalue(value)->type == V_ERROR)
                {
                  return value;
                }

              if (g_pc.token->type == T_COMMA)
                {
                  ++g_pc.token;
                  ++level;
                }
              else
                {
                  break;
                }
            }
          else
            {
              break;
            }
        }

      while (level >= 0)
        {
          (*next)[level--].fr.token->u.exitfor = g_pc;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_ON(struct Value *value)
{
  struct On *on = &g_pc.token->u.on;

  ++g_pc.token;
  if (eval(value, _("selector"))->type == V_ERROR)
    {
      return value;
    }

  if (Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  if (g_pass == INTERPRET)
    {
      struct Pc newpc;

      if (value->u.integer > 0 && value->u.integer < on->pcLength)
        {
          newpc = on->pc[value->u.integer];
        }
      else
        {
          newpc = on->pc[0];
        }

      if (g_pc.token->type == T_GOTO)
        {
          g_pc = newpc;
        }
      else
        {
          g_pc = on->pc[0];
          Auto_pushGosubRet(&g_stack, &g_pc);
          g_pc = newpc;
        }

      Program_trace(&g_program, &g_pc, 0, 1);
    }
  else if (g_pass == DECLARE || g_pass == COMPILE)
    {
      Value_destroy(value);
      if (g_pc.token->type != T_GOTO && g_pc.token->type != T_GOSUB)
        {
          return Value_new_ERROR(value, MISSINGGOTOSUB);
        }

      ++g_pc.token;
      on->pcLength = 1;
      while (1)
        {
          on->pc = realloc(on->pc, sizeof(struct Pc) * ++on->pcLength);
          if (g_pc.token->type != T_INTEGER)
            {
              return Value_new_ERROR(value, MISSINGLINENUMBER);
            }

          if (Program_goLine
              (&g_program, g_pc.token->u.integer,
               &on->pc[on->pcLength - 1]) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, NOSUCHLINE);
            }

          if (g_pass == COMPILE &&
              Program_scopeCheck(&g_program, &on->pc[on->pcLength - 1],
                                 findLabel(L_FUNC)))
            {
              return Value_new_ERROR(value, OUTOFSCOPE);
            }

          ++g_pc.token;
          if (g_pc.token->type == T_COMMA)
            {
              ++g_pc.token;
            }
          else
            {
              break;
            }
        }

      on->pc[0] = g_pc;
    }

  return (struct Value *)0;
}

struct Value *stmt_ONERROR(struct Value *value)
{
  if (DIRECTMODE)
    {
      return Value_new_ERROR(value, NOTINDIRECTMODE);
    }

  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      g_stack.onerror = g_pc;
      Program_nextLine(&g_program, &g_pc);
      return (struct Value *)0;
    }
  else
    {
      return &more_statements;
    }
}

struct Value *stmt_ONERRORGOTO0(struct Value *value)
{
  if (DIRECTMODE)
    {
      return Value_new_ERROR(value, NOTINDIRECTMODE);
    }

  if (g_pass == INTERPRET)
    {
      g_stack.onerror.line = -1;
      if (g_stack.resumeable)
        {
          g_pc = g_stack.erpc;
          return Value_clone(value, &g_stack.err);
        }
    }

  ++g_pc.token;
  return (struct Value *)0;
}

struct Value *stmt_ONERROROFF(struct Value *value)
{
  if (DIRECTMODE)
    {
      return Value_new_ERROR(value, NOTINDIRECTMODE);
    }

  if (g_pass == INTERPRET)
    {
      g_stack.onerror.line = -1;
    }

  ++g_pc.token;
  return (struct Value *)0;
}

struct Value *stmt_OPEN(struct Value *value)
{
  int inout = -1, append = 0;
  int mode = FS_ACCESS_NONE, lock = FS_LOCK_NONE;
  long int channel;
  long int recLength = -1;
  struct Pc errpc;
  struct Value recLengthValue;
  struct Pc statementpc = g_pc;

  ++g_pc.token;
  errpc = g_pc;
  if (eval(value, _("mode or file"))->type == V_ERROR ||
      Value_retype(value, V_STRING)->type == V_ERROR)
    {
      return value;
    }

  if (g_pc.token->type == T_COMMA)        /* parse MBASIC syntax */
    {
      if (value->u.string.length >= 1)
        {
          switch (tolower(value->u.string.character[0]))
            {
            case 'i':
              inout = 0;
              mode = FS_ACCESS_READ;
              break;

            case 'o':
              inout = 1;
              mode = FS_ACCESS_WRITE;
              break;

            case 'a':
              inout = 1;
              mode = FS_ACCESS_WRITE;
              append = 1;
              break;

            case 'r':
              inout = 3;
              mode = FS_ACCESS_READWRITE;
              break;
            }
        }

      Value_destroy(value);
      if (g_pass == INTERPRET && inout == -1)
        {
          g_pc = errpc;
          return Value_new_ERROR(value, BADMODE);
        }

      if (g_pc.token->type != T_COMMA)
        {
          return Value_new_ERROR(value, MISSINGCOMMA);
        }

      ++g_pc.token;
      if (g_pc.token->type == T_CHANNEL)
        {
          ++g_pc.token;
        }

      errpc = g_pc;
      if (eval(value, _("channel"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          g_pc = errpc;
          return value;
        }

      channel = value->u.integer;
      Value_destroy(value);
      if (g_pass == INTERPRET && channel < 0)
        {
          return Value_new_ERROR(value, OUTOFRANGE, _("channel"));
        }

      if (g_pc.token->type != T_COMMA)
        {
          return Value_new_ERROR(value, MISSINGCOMMA);
        }

      ++g_pc.token;
      if (eval(value, _("file name"))->type == V_ERROR ||
          Value_retype(value, V_STRING)->type == V_ERROR)
        {
          return value;
        }

      if (inout == 3)
        {
          if (g_pc.token->type != T_COMMA)
            {
              Value_destroy(value);
              return Value_new_ERROR(value, MISSINGCOMMA);
            }

          ++g_pc.token;
          errpc = g_pc;
          if (eval(&recLengthValue, _("record length"))->type == V_ERROR ||
              Value_retype(&recLengthValue, V_INTEGER)->type == V_ERROR)
            {
              Value_destroy(value);
              *value = recLengthValue;
              return value;
            }

          recLength = recLengthValue.u.integer;
          Value_destroy(&recLengthValue);
          if (g_pass == INTERPRET && recLength <= 0)
            {
              Value_destroy(value);
              g_pc = errpc;
              return Value_new_ERROR(value, OUTOFRANGE, _("record length"));
            }
        }
    }
  else                          /* parse ANSI syntax */
    {
      struct Value channelValue;
      int newMode;

      switch (g_pc.token->type)
        {
        case T_FOR_INPUT:
          inout = 0;
          mode = FS_ACCESS_READ;
          ++g_pc.token;
          break;

        case T_FOR_OUTPUT:
          inout = 1;
          mode = FS_ACCESS_WRITE;
          ++g_pc.token;
          break;

        case T_FOR_APPEND:
          inout = 1;
          mode = FS_ACCESS_WRITE;
          append = 1;
          ++g_pc.token;
          break;

        case T_FOR_RANDOM:
          inout = 3;
          mode = FS_ACCESS_READWRITE;
          ++g_pc.token;
          break;

        case T_FOR_BINARY:
          inout = 4;
          mode = FS_ACCESS_READWRITE;
          ++g_pc.token;
          break;

        default:
          inout = 3;
          mode = FS_ACCESS_READWRITE;
          break;
        }

      switch (g_pc.token->type)
        {
        case T_ACCESS_READ:
          newMode = FS_ACCESS_READ;
          break;

        case T_ACCESS_READ_WRITE:
          newMode = FS_ACCESS_READWRITE;
          break;

        case T_ACCESS_WRITE:
          newMode = FS_ACCESS_WRITE;
          break;

        default:
          newMode = FS_ACCESS_NONE;
        }

      if (newMode != FS_ACCESS_NONE)
        {
          if ((newMode & mode) == 0)
            {
              return Value_new_ERROR(value, WRONGMODE);
            }

          mode = newMode;
          ++g_pc.token;
        }

      switch (g_pc.token->type)
        {
        case T_SHARED:
          lock = FS_LOCK_NONE;
          ++g_pc.token;
          break;

        case T_LOCK_READ:
          lock = FS_LOCK_SHARED;
          ++g_pc.token;
          break;

        case T_LOCK_WRITE:
          lock = FS_LOCK_EXCLUSIVE;
          ++g_pc.token;
          break;

        default:;
        }

      if (g_pc.token->type != T_AS)
        {
          Value_destroy(value);
          return Value_new_ERROR(value, MISSINGAS);
        }

      ++g_pc.token;
      if (g_pc.token->type == T_CHANNEL)
        {
          ++g_pc.token;
        }

      errpc = g_pc;
      if (eval(&channelValue, _("channel"))->type == V_ERROR ||
          Value_retype(&channelValue, V_INTEGER)->type == V_ERROR)
        {
          g_pc = errpc;
          Value_destroy(value);
          *value = channelValue;
          return value;
        }

      channel = channelValue.u.integer;
      Value_destroy(&channelValue);
      if (inout == 3)
        {
          if (g_pc.token->type == T_IDENTIFIER)
            {
              if (cistrcmp(g_pc.token->u.identifier->name, "len"))
                {
                  Value_destroy(value);
                  return Value_new_ERROR(value, MISSINGLEN);
                }

              ++g_pc.token;
              if (g_pc.token->type != T_EQ)
                {
                  Value_destroy(value);
                  return Value_new_ERROR(value, MISSINGEQ);
                }

              ++g_pc.token;
              errpc = g_pc;
              if (eval(&recLengthValue, _("record length"))->type == V_ERROR
                || Value_retype(&recLengthValue, V_INTEGER)->type == V_ERROR)
                {
                  Value_destroy(value);
                  *value = recLengthValue;
                  return value;
                }

              recLength = recLengthValue.u.integer;
              Value_destroy(&recLengthValue);
              if (g_pass == INTERPRET && recLength <= 0)
                {
                  Value_destroy(value);
                  g_pc = errpc;
                  return Value_new_ERROR(value, OUTOFRANGE,
                                         _("record length"));
                }
            }
          else
            {
              recLength = 1;
            }
        }
    }

  /* open file with name value */

  if (g_pass == INTERPRET)
    {
      int res = -1;

      if (inout == 0)
        {
          res = FS_openinChn(channel, value->u.string.character, mode);
        }
      else if (inout == 1)
        {
          res = FS_openoutChn(channel, value->u.string.character,
                              mode, append);
        }
      else if (inout == 3)
        {
          res =
            FS_openrandomChn(channel, value->u.string.character, mode,
                             recLength);
        }
      else if (inout == 4)
        {
          res = FS_openbinaryChn(channel, value->u.string.character, mode);
        }

      if (res == -1)
        {
          g_pc = statementpc;
          Value_destroy(value);
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
      else
        {
          if (lock != FS_LOCK_NONE && FS_lock(channel, 0, 0, lock, 0) == -1)
            {
              g_pc = statementpc;
              Value_destroy(value);
              Value_new_ERROR(value, IOERROR, FS_errmsg);
              FS_close(channel);
              return value;
            }
        }
    }

  Value_destroy(value);
  return (struct Value *)0;
}

struct Value *stmt_OPTIONBASE(struct Value *value)
{
  ++g_pc.token;
  if (eval(value, _("array subscript base"))->type == V_ERROR ||
      (g_pass != DECLARE && Value_retype(value, V_INTEGER)->type == V_ERROR))
    {
      return value;
    }

  if (g_pass == INTERPRET)
    {
      g_optionbase = value->u.integer;
    }

  Value_destroy(value);
  return (struct Value *)0;
}

struct Value *stmt_OPTIONRUN(struct Value *value)
{
  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      FS_xonxoff(STDCHANNEL, 0);
    }

  return (struct Value *)0;
}

struct Value *stmt_OPTIONSTOP(struct Value *value)
{
  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      FS_xonxoff(STDCHANNEL, 1);
    }

  return (struct Value *)0;
}

struct Value *stmt_OUT_POKE(struct Value *value)
{
  int out, address, val;
  struct Pc lpc;

  out = (g_pc.token->type == T_OUT);
  lpc = g_pc;
  ++g_pc.token;
  if (eval(value, _("address"))->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  address = value->u.integer;
  Value_destroy(value);
  if (g_pc.token->type != T_COMMA)
    {
      return Value_new_ERROR(value, MISSINGCOMMA);
    }

  ++g_pc.token;
  if (eval(value, _("output value"))->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  val = value->u.integer;
  Value_destroy(value);
  if (g_pass == INTERPRET)
    {
      if ((out ? FS_portOutput : FS_memOutput) (address, val) == -1)
        {
          g_pc = lpc;
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_PRINT_LPRINT(struct Value *value)
{
  int nl = 1;
  int chn = (g_pc.token->type == T_PRINT ? STDCHANNEL : LPCHANNEL);
  int printusing = 0;
  struct Value usingval;
  struct String *using = (struct String *)0;
  size_t usingpos = 0;

  ++g_pc.token;
  if (chn == STDCHANNEL && g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
      if (eval(value, _("channel"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      chn = value->u.integer;
      Value_destroy(value);
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
    }

  if (g_pc.token->type == T_USING)
    {
      struct Pc usingpc;

      usingpc = g_pc;
      printusing = 1;
      ++g_pc.token;
      if (g_pc.token->type == T_INTEGER)
        {
          if (g_pass == COMPILE &&
              Program_imageLine(&g_program, g_pc.token->u.integer,
                                &usingpc.token->u.image) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, NOSUCHIMAGELINE);
            }
          else if (g_pass == INTERPRET)
            {
              using = usingpc.token->u.image.token->u.string;
            }

          Value_new_STRING(&usingval);
          ++g_pc.token;
        }
      else
        {
          if (eval(&usingval, _("format string"))->type == V_ERROR ||
              Value_retype(&usingval, V_STRING)->type == V_ERROR)
            {
              *value = usingval;
              return value;
            }

          using = &usingval.u.string;
        }

      if (g_pc.token->type != T_SEMICOLON)
        {
          Value_destroy(&usingval);
          return Value_new_ERROR(value, MISSINGSEMICOLON);
        }

      ++g_pc.token;
    }
  else
    {
      Value_new_STRING(&usingval);
      using = &usingval.u.string;
    }

  while (1)
    {
      struct Pc valuepc;

      valuepc = g_pc;
      if (eval(value, (const char *)0))
        {
          if (value->type == V_ERROR)
            {
              Value_destroy(&usingval);
              return value;
            }

          if (g_pass == INTERPRET)
            {
              struct String s;

              String_new(&s);
              if (Value_toStringUsing(value, &s, using, &usingpos)->type ==
                  V_ERROR)
                {
                  Value_destroy(&usingval);
                  String_destroy(&s);
                  g_pc = valuepc;
                  return value;
                }

              if (FS_putItem(chn, &s) == -1)
                {
                  Value_destroy(&usingval);
                  Value_destroy(value);
                  String_destroy(&s);
                  return Value_new_ERROR(value, IOERROR, FS_errmsg);
                }

              String_destroy(&s);
            }

          Value_destroy(value);
          nl = 1;
        }
      else if (g_pc.token->type == T_TAB || g_pc.token->type == T_SPC)
        {
          int tab = g_pc.token->type == T_TAB;

          ++g_pc.token;
          if (g_pc.token->type != T_OP)
            {
              Value_destroy(&usingval);
              return Value_new_ERROR(value, MISSINGOP);
            }

          ++g_pc.token;
          if (eval(value, _("count"))->type == V_ERROR ||
              Value_retype(value, V_INTEGER)->type == V_ERROR)
            {
              Value_destroy(&usingval);
              return value;
            }

          if (g_pass == INTERPRET)
            {
              int s = value->u.integer;
              int r = 0;

              if (tab)
                {
                  r = FS_tab(chn, s);
                }
              else
                {
                  while (s-- > 0 && (r = FS_putChar(chn, ' ')) != -1);
                }

              if (r == -1)
                {
                  Value_destroy(&usingval);
                  Value_destroy(value);
                  return Value_new_ERROR(value, IOERROR, FS_errmsg);
                }
            }

          Value_destroy(value);
          if (g_pc.token->type != T_CP)
            {
              Value_destroy(&usingval);
              return Value_new_ERROR(value, MISSINGCP);
            }

          ++g_pc.token;
          nl = 1;
        }

      else if (g_pc.token->type == T_SEMICOLON)
        {
          ++g_pc.token;
          nl = 0;
        }

      else if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
          if (g_pass == INTERPRET && !printusing)
            {
              FS_nextcol(chn);
            }

          nl = 0;
        }

      else
        {
          break;
        }

      if (g_pass == INTERPRET && FS_flush(chn) == -1)
        {
          Value_destroy(&usingval);
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
    }

  Value_destroy(&usingval);
  if (g_pass == INTERPRET)
    {
      if (nl && FS_putChar(chn, '\n') == -1)
        {
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }

      if (FS_flush(chn) == -1)
        {
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_RANDOMIZE(struct Value *value)
{
  struct Pc argpc;

  ++g_pc.token;
  argpc = g_pc;
  if (eval(value, (const char *)0))
    {
      Value_retype(value, V_INTEGER);
      if (value->type == V_ERROR)
        {
          g_pc = argpc;
          Value_destroy(value);
          return Value_new_ERROR(value, MISSINGEXPR,
                                 _("random number generator seed"));
        }

      if (g_pass == INTERPRET)
        {
          srand(g_pc.token->u.integer);
        }

      Value_destroy(value);
    }
  else
    {
      srand(getpid() ^ time((time_t *) 0));
    }

  return (struct Value *)0;
}

struct Value *stmt_READ(struct Value *value)
{
  ++g_pc.token;
  while (1)
    {
      struct Value *l;
      struct Pc lvaluepc;

      lvaluepc = g_pc;
      if (g_pc.token->type != T_IDENTIFIER)
        {
          return Value_new_ERROR(value, MISSINGREADIDENT);
        }

      if (g_pass == DECLARE &&
          Global_variable(&g_globals, g_pc.token->u.identifier,
                          g_pc.token->u.identifier->defaultType,
                          (g_pc.token + 1)->type ==
                          T_OP ? GLOBALARRAY : GLOBALVAR, 0) == 0)
        {
          return Value_new_ERROR(value, REDECLARATION);
        }

      if ((l = lvalue(value))->type == V_ERROR)
        {
          return value;
        }

      if (g_pass == INTERPRET && dataread(value, l))
        {
          g_pc = lvaluepc;
          return value;
        }

      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_COPY_RENAME(struct Value *value)
{
  struct Pc argpc;
  struct Value from;
  struct Pc statementpc = g_pc;

  ++g_pc.token;
  argpc = g_pc;
  if (eval(&from, _("source file"))->type == V_ERROR ||
      (g_pass != DECLARE && Value_retype(&from, V_STRING)->type == V_ERROR))
    {
      g_pc = argpc;
      *value = from;
      return value;
    }

  if (g_pc.token->type != T_TO)
    {
      Value_destroy(&from);
      return Value_new_ERROR(value, MISSINGTO);
    }

  ++g_pc.token;
  argpc = g_pc;
  if (eval(value, _("destination file"))->type == V_ERROR ||
      (g_pass != DECLARE && Value_retype(value, V_STRING)->type == V_ERROR))
    {
      g_pc = argpc;
      return value;
    }

  if (g_pass == INTERPRET)
    {
      const char *msg;
      int res;

      if (statementpc.token->type == T_RENAME)
        {
          res = rename(from.u.string.character, value->u.string.character);
          msg = strerror(errno);
        }
      else
        {
          res = FS_copy(from.u.string.character, value->u.string.character);
          msg = FS_errmsg;
        }

      if (res == -1)
        {
          Value_destroy(&from);
          Value_destroy(value);
          g_pc = statementpc;
          return Value_new_ERROR(value, IOERROR, msg);
        }
    }

  Value_destroy(&from);
  Value_destroy(value);
  return (struct Value *)0;
}

struct Value *stmt_RENUM(struct Value *value)
{
  int first = 10, inc = 10;

  ++g_pc.token;
  if (g_pc.token->type == T_INTEGER)
    {
      first = g_pc.token->u.integer;
      ++g_pc.token;
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
          if (g_pc.token->type != T_INTEGER)
            {
              return Value_new_ERROR(value, MISSINGINCREMENT);
            }

          inc = g_pc.token->u.integer;
          ++g_pc.token;
        }
    }

  if (g_pass == INTERPRET)
    {
      if (!DIRECTMODE)
        {
          return Value_new_ERROR(value, NOTINPROGRAMMODE);
        }

      Program_renum(&g_program, first, inc);
    }

  return (struct Value *)0;
}

struct Value *stmt_REPEAT(struct Value *value)
{
  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      pushLabel(L_REPEAT, &g_pc);
    }

  ++g_pc.token;
  return (struct Value *)0;
}

struct Value *stmt_RESTORE(struct Value *value)
{
  struct Token *restorepc = g_pc.token;

  if (g_pass == INTERPRET)
    {
      g_curdata = g_pc.token->u.restore;
    }

  ++g_pc.token;
  if (g_pc.token->type == T_INTEGER)
    {
      if (g_pass == COMPILE &&
          Program_dataLine(&g_program, g_pc.token->u.integer,
                           &restorepc->u.restore) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, NOSUCHDATALINE);
        }

      ++g_pc.token;
    }
  else if (g_pass == COMPILE)
    {
      restorepc->u.restore = g_stack.begindata;
    }

  return (struct Value *)0;
}

struct Value *stmt_RETURN(struct Value *value)
{
  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      ++g_pc.token;
    }

  if (g_pass == INTERPRET)
    {
      if (Auto_gosubReturn(&g_stack, &g_pc))
        {
          Program_trace(&g_program, &g_pc, 0, 1);
        }
      else
        {
          return Value_new_ERROR(value, STRAYRETURN);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_RUN(struct Value *value)
{
  struct Pc argpc, begin;

  g_stack.resumeable = 0;
  ++g_pc.token;
  argpc = g_pc;
  if (g_pc.token->type == T_INTEGER)
    {
      if (Program_goLine(&g_program, g_pc.token->u.integer, &begin) ==
          (struct Pc *)0)
        {
          return Value_new_ERROR(value, NOSUCHLINE);
        }

      if (g_pass == COMPILE &&
          Program_scopeCheck(&g_program, &begin, findLabel(L_FUNC)))
        {
          return Value_new_ERROR(value, OUTOFSCOPE);
        }

      ++g_pc.token;
    }
  else if (eval(value, (const char *)0))
    {
      if (value->type == V_ERROR ||
          Value_retype(value, V_STRING)->type == V_ERROR)
        {
          g_pc = argpc;
          return value;
        }
      else if (g_pass == INTERPRET)
        {
          int chn;
          struct Program newprogram;

          if ((chn = FS_openin(value->u.string.character)) == -1)
            {
              g_pc = argpc;
              Value_destroy(value);
              return Value_new_ERROR(value, IOERROR, FS_errmsg);
            }

          Value_destroy(value);
          Program_new(&newprogram);
          if (Program_merge(&newprogram, chn, value))
            {
              g_pc = argpc;
              Program_destroy(&newprogram);
              return value;
            }

          FS_close(chn);
          new();
          Program_destroy(&g_program);
          g_program = newprogram;
          if (Program_beginning(&g_program, &begin) == (struct Pc *)0)
            {
              return Value_new_ERROR(value, NOPROGRAM);
            }
        }
      else
        {
          Value_destroy(value);
        }
    }
  else
    {
      if (Program_beginning(&g_program, &begin) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, NOPROGRAM);
        }
    }

  if (g_pass == INTERPRET)
    {
      if (compileProgram(value, 1)->type == V_ERROR)
        {
          return value;
        }

      g_pc = begin;
      g_curdata = g_stack.begindata;
      Global_clear(&g_globals);
      FS_closefiles();
      Program_trace(&g_program, &g_pc, 0, 1);
    }

  return (struct Value *)0;
}

struct Value *stmt_SAVE(struct Value *value)
{
  struct Pc loadpc;
  int name;

  if (g_pass == INTERPRET && !DIRECTMODE)
    {
      return Value_new_ERROR(value, NOTINPROGRAMMODE);
    }

  ++g_pc.token;
  loadpc = g_pc;
  if (g_pc.token->type == T_EOL && g_program.name.length)
    {
      name = 0;
    }
  else
    {
      name = 1;
      if (eval(value, _("file name"))->type == V_ERROR ||
          Value_retype(value, V_STRING)->type == V_ERROR)
        {
          g_pc = loadpc;
          return value;
        }
    }

  if (g_pass == INTERPRET)
    {
      int chn;

      if (name)
        {
          Program_setname(&g_program, value->u.string.character);
        }

      if ((chn = FS_openout(g_program.name.character)) == -1)
        {
          g_pc = loadpc;
          if (name)
            {
              Value_destroy(value);
            }

          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }

      FS_width(chn, 0);
      if (name)
        {
          Value_destroy(value);
        }

      if (Program_list(&g_program, chn, 0, 0, 0, value))
        {
          g_pc = loadpc;
          return value;
        }

      FS_close(chn);
      g_program.unsaved = 0;
    }
  else if (name)
    {
      Value_destroy(value);
    }

  return (struct Value *)0;
}

struct Value *stmt_SELECTCASE(struct Value *value)
{
  struct Pc statementpc = g_pc;

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      pushLabel(L_SELECTCASE, &g_pc);
    }

  ++g_pc.token;
  if (eval(value, _("selector"))->type == V_ERROR)
    {
      return value;
    }

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      statementpc.token->u.selectcase->type = value->type;
      statementpc.token->u.selectcase->nextcasevalue.line = -1;
    }
  else
    {
      struct Pc casevaluepc;
      int match = 0;

      g_pc = casevaluepc = statementpc.token->u.selectcase->nextcasevalue;
      do
        {
          ++g_pc.token;
          switch (casevaluepc.token->type)
            {
            case T_CASEVALUE:
              {
                do
                  {
                    struct Value casevalue1;

                    if (g_pc.token->type == T_IS)
                      {
                        enum TokenType relop;

                        ++g_pc.token;
                        relop = g_pc.token->type;
                        ++g_pc.token;
                        if (eval(&casevalue1, "`is'")->type == V_ERROR)
                          {
                            Value_destroy(value);
                            *value = casevalue1;
                            return value;
                          }

                        Value_retype(&casevalue1,
                                     statementpc.token->u.selectcase->type);
                        assert(casevalue1.type != V_ERROR);
                        if (!match)
                          {
                            struct Value cmp;

                            Value_clone(&cmp, value);
                            switch (relop)
                              {
                              case T_LT:
                                Value_lt(&cmp, &casevalue1, 1);
                                break;

                              case T_LE:
                                Value_le(&cmp, &casevalue1, 1);
                                break;

                              case T_EQ:
                                Value_eq(&cmp, &casevalue1, 1);
                                break;
                              case T_GE:
                                Value_ge(&cmp, &casevalue1, 1);
                                break;

                              case T_GT:
                                Value_gt(&cmp, &casevalue1, 1);
                                break;
                              case T_NE:
                                Value_ne(&cmp, &casevalue1, 1);
                                break;

                              default:
                                assert(0);
                              }

                            assert(cmp.type == V_INTEGER);
                            match = cmp.u.integer;
                            Value_destroy(&cmp);
                          }

                        Value_destroy(&casevalue1);
                      }
                    else
                      {
                        if (eval(&casevalue1, "`case'")->type == V_ERROR)
                          {
                            Value_destroy(value);
                            *value = casevalue1;
                            return value;
                          }

                        Value_retype(&casevalue1,
                                     statementpc.token->u.selectcase->type);
                        assert(casevalue1.type != V_ERROR);
                        if (g_pc.token->type == T_TO)     /* match range */
                          {
                            struct Value casevalue2;

                            ++g_pc.token;
                            if (eval(&casevalue2, "`case'")->type == V_ERROR)
                              {
                                Value_destroy(&casevalue1);
                                Value_destroy(value);
                                *value = casevalue2;
                                return value;
                              }

                            Value_retype(&casevalue2,
                                      statementpc.token->u.selectcase->type);
                            assert(casevalue2.type != V_ERROR);
                            if (!match)
                              {
                                struct Value cmp1, cmp2;

                                Value_clone(&cmp1, value);
                                Value_clone(&cmp2, value);
                                Value_ge(&cmp1, &casevalue1, 1);
                                assert(cmp1.type == V_INTEGER);
                                Value_le(&cmp2, &casevalue2, 1);
                                assert(cmp2.type == V_INTEGER);
                                match = cmp1.u.integer && cmp2.u.integer;
                                Value_destroy(&cmp1);
                                Value_destroy(&cmp2);
                              }

                            Value_destroy(&casevalue2);
                          }
                        else    /* match value */
                          {
                            if (!match)
                              {
                                struct Value cmp;

                                Value_clone(&cmp, value);
                                Value_eq(&cmp, &casevalue1, 1);
                                assert(cmp.type == V_INTEGER);
                                match = cmp.u.integer;
                                Value_destroy(&cmp);
                              }
                          }

                        Value_destroy(&casevalue1);
                      }

                    if (g_pc.token->type == T_COMMA)
                      {
                        ++g_pc.token;
                      }
                    else
                      {
                        break;
                      }
                  }
                while (1);

                break;
              }

            case T_CASEELSE:
              {
                match = 1;
                break;
              }

            default:
              assert(0);
            }

          if (!match)
            {
              if (casevaluepc.token->u.casevalue->nextcasevalue.line != -1)
                {
                  g_pc = casevaluepc =
                    casevaluepc.token->u.casevalue->nextcasevalue;
                }
              else
                {
                  g_pc = statementpc.token->u.selectcase->endselect;
                  break;
                }
            }
        }
      while (!match);
    }

  Value_destroy(value);
  return (struct Value *)0;
}

struct Value *stmt_SHELL(struct Value *value)
{
#if defined(CONFIG_EXAMPLES_BAS_SHELL) && defined(CONFIG_ARCH_HAVE_VFORK)
  pid_t pid;
  int status;

  ++g_pc.token;
  if (eval(value, (const char *)0))
    {
      if (value->type == V_ERROR ||
          Value_retype(value, V_STRING)->type == V_ERROR)
        {
          return value;
        }

      if (g_pass == INTERPRET)
        {
          if (g_run_restricted)
            {
              Value_destroy(value);
              return Value_new_ERROR(value, RESTRICTED, strerror(errno));
            }

          FS_shellmode(STDCHANNEL);
          switch (pid = vfork())
            {
            case -1:
              {
                FS_fsmode(STDCHANNEL);
                Value_destroy(value);
                return Value_new_ERROR(value, FORKFAILED, strerror(errno));
              }

            case 0:
              {
                execl("/bin/sh", "sh", "-c", value->u.string.character,
                      (const char *)0);
                exit(127);
              }

            default:
              {
                /* Wait for the shell to complete */

                while (waitpid(pid, &status, 0) < 0 && errno != EINTR);
              }
            }

          FS_fsmode(STDCHANNEL);
        }

      Value_destroy(value);
    }
  else
    {
      if (g_pass == INTERPRET)
        {
          if (g_run_restricted)
            {
              return Value_new_ERROR(value, RESTRICTED, strerror(errno));
            }

          FS_shellmode(STDCHANNEL);
          switch (pid = vfork())
            {
            case -1:
              {
                FS_fsmode(STDCHANNEL);
                return Value_new_ERROR(value, FORKFAILED, strerror(errno));
              }

            case 0:
              {
                const char *shell;

                shell = getenv("SHELL");
                if (shell == (const char *)0)
                  {
                    shell = "/bin/sh";
                  }

                execl(shell,
                      strrchr(shell, '/') ? strrchr(shell, '/') + 1 : shell,
                      (const char *)0);
                exit(127);
              }

            default:
              {
                /* Wait for the shell to complete */

                while (waitpid(pid, &status, 0) < 0 && errno != EINTR);
              }
            }

          FS_fsmode(STDCHANNEL);
        }
    }

  return (struct Value *)0;
#else
  return Value_new_ERROR(value, NOTAVAILABLE);
#endif
}

struct Value *stmt_SLEEP(struct Value *value)
{
  double s;

  ++g_pc.token;
  if (eval(value, _("pause"))->type == V_ERROR ||
      Value_retype(value, V_REAL)->type == V_ERROR)
    {
      return value;
    }

  s = value->u.real;
  Value_destroy(value);
  if (g_pass == INTERPRET)
    {
      if (s < 0.0)
        {
          return Value_new_ERROR(value, OUTOFRANGE, _("pause"));
        }

      FS_sleep(s);
    }

  return (struct Value *)0;
}

struct Value *stmt_STOP(struct Value *value)
{
  if (g_pass != INTERPRET)
    {
      ++g_pc.token;
    }

  return (struct Value *)0;
}

struct Value *stmt_SUBEXIT(struct Value *value)
{
  struct Pc *curfn = (struct Pc *)0;

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      if ((curfn = findLabel(L_FUNC)) == (struct Pc *)0 ||
          (curfn->token + 1)->u.identifier->defaultType != V_VOID)
        {
          return Value_new_ERROR(value, STRAYSUBEXIT);
        }
    }

  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      return Value_new_VOID(value);
    }

  return (struct Value *)0;
}

struct Value *stmt_SWAP(struct Value *value)
{
  struct Value *l1, *l2;
  struct Pc lvaluepc;

  ++g_pc.token;
  lvaluepc = g_pc;
  if (g_pc.token->type != T_IDENTIFIER)
    {
      return Value_new_ERROR(value, MISSINGSWAPIDENT);
    }

  if (g_pass == DECLARE &&
      Global_variable(&g_globals, g_pc.token->u.identifier,
                      g_pc.token->u.identifier->defaultType,
                      (g_pc.token + 1)->type == T_OP ?
                      GLOBALARRAY : GLOBALVAR,
                      0) == 0)
    {
      return Value_new_ERROR(value, REDECLARATION);
    }

  if ((l1 = lvalue(value))->type == V_ERROR)
    {
      return value;
    }

  if (g_pc.token->type == T_COMMA)
    {
      ++g_pc.token;
    }
  else
    {
      return Value_new_ERROR(value, MISSINGCOMMA);
    }

  lvaluepc = g_pc;
  if (g_pc.token->type != T_IDENTIFIER)
    {
      return Value_new_ERROR(value, MISSINGSWAPIDENT);
    }

  if (g_pass == DECLARE &&
      Global_variable(&g_globals, g_pc.token->u.identifier,
                      g_pc.token->u.identifier->defaultType,
                      (g_pc.token + 1)->type == T_OP ?
                      GLOBALARRAY : GLOBALVAR,
                      0) == 0)
    {
      return Value_new_ERROR(value, REDECLARATION);
    }

  if ((l2 = lvalue(value))->type == V_ERROR)
    {
      return value;
    }

  if (l1->type != l2->type)
    {
      g_pc = lvaluepc;
      return Value_new_typeError(value, l2->type, l1->type);
    }

  if (g_pass == INTERPRET)
    {
      struct Value foo;

      foo = *l1;
      *l1 = *l2;
      *l2 = foo;
    }

  return (struct Value *)0;
}

struct Value *stmt_SYSTEM(struct Value *value)
{
  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      if (g_program.unsaved)
        {
          int ch;

          FS_putChars(STDCHANNEL, _("Quit without saving? (y/n) "));
          FS_flush(STDCHANNEL);
          if ((ch = FS_getChar(STDCHANNEL)) != -1)
            {
              FS_putChar(STDCHANNEL, ch);
              FS_flush(STDCHANNEL);
              FS_nextline(STDCHANNEL);
              if (tolower(ch) == *_("yes"))
                {
                  bas_exit();
                  exit(0);
                }
            }
        }
      else
        {
          bas_exit();
          exit(0);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_TROFF(struct Value *value)
{
  ++g_pc.token;
  g_program.trace = 0;
  return (struct Value *)0;
}

struct Value *stmt_TRON(struct Value *value)
{
  ++g_pc.token;
  g_program.trace = 1;
  return (struct Value *)0;
}

struct Value *stmt_TRUNCATE(struct Value *value)
{
  struct Pc chnpc;
  int chn;

  chnpc = g_pc;
  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
    }

  if (eval(value, (const char *)0) == (struct Value *)0)
    {
      return Value_new_ERROR(value, MISSINGEXPR, _("channel"));
    }

  if (value->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  chn = value->u.integer;
  Value_destroy(value);
  if (g_pass == INTERPRET && FS_truncate(chn) == -1)
    {
      g_pc = chnpc;
      return Value_new_ERROR(value, IOERROR, FS_errmsg);
    }

  return (struct Value *)0;
}

struct Value *stmt_UNNUM(struct Value *value)
{
  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      if (!DIRECTMODE)
        {
          return Value_new_ERROR(value, NOTINPROGRAMMODE);
        }

      Program_unnum(&g_program);
    }

  return (struct Value *)0;
}

struct Value *stmt_UNTIL(struct Value *value)
{
  struct Pc untilpc = g_pc;
  struct Pc *repeatpc;

  ++g_pc.token;
  if (eval(value, _("condition"))->type == V_ERROR)
    {
      return value;
    }

  if (g_pass == INTERPRET)
    {
      if (Value_isNull(value))
        {
          g_pc = untilpc.token->u.until;
        }

      Value_destroy(value);
    }

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      if ((repeatpc = popLabel(L_REPEAT)) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, STRAYUNTIL);
        }

      untilpc.token->u.until = *repeatpc;
    }

  return (struct Value *)0;
}

struct Value *stmt_WAIT(struct Value *value)
{
  int address, mask, sel = -1, usesel;
  struct Pc lpc;

  lpc = g_pc;
  ++g_pc.token;
  if (eval(value, _("address"))->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  address = value->u.integer;
  Value_destroy(value);
  if (g_pc.token->type != T_COMMA)
    {
      return Value_new_ERROR(value, MISSINGCOMMA);
    }

  ++g_pc.token;
  if (eval(value, _("mask"))->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  mask = value->u.integer;
  Value_destroy(value);
  if (g_pc.token->type == T_COMMA)
    {
      ++g_pc.token;
      if (eval(value, _("select"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      sel = value->u.integer;
      usesel = 1;
      Value_destroy(value);
    }
  else
    {
      usesel = 0;
    }

  if (g_pass == INTERPRET)
    {
      int v;

      do
        {
          if ((v = FS_portInput(address)) == -1)
            {
              g_pc = lpc;
              return Value_new_ERROR(value, IOERROR, FS_errmsg);
            }
        }
      while ((usesel ? (v ^ sel) & mask : v ^ mask) == 0);
    }

  return (struct Value *)0;
}

struct Value *stmt_WHILE(struct Value *value)
{
  struct Pc whilepc = g_pc;

  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      pushLabel(L_WHILE, &g_pc);
    }

  ++g_pc.token;
  if (eval(value, _("condition"))->type == V_ERROR)
    {
      return value;
    }

  if (g_pass == INTERPRET)
    {
      if (Value_isNull(value))
        {
          g_pc = *whilepc.token->u.afterwend;
        }

      Value_destroy(value);
    }

  return (struct Value *)0;
}

struct Value *stmt_WEND(struct Value *value)
{
  if (g_pass == DECLARE || g_pass == COMPILE)
    {
      struct Pc *whilepc;

      if ((whilepc = popLabel(L_WHILE)) == (struct Pc *)0)
        {
          return Value_new_ERROR(value, STRAYWEND, topLabelDescription());
        }

      *g_pc.token->u.whilepc = *whilepc;
      ++g_pc.token;
      *(whilepc->token->u.afterwend) = g_pc;
    }
  else
    {
      g_pc = *g_pc.token->u.whilepc;
    }

  return (struct Value *)0;
}

struct Value *stmt_WIDTH(struct Value *value)
{
  int chn = STDCHANNEL, width;

  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
      if (eval(value, _("channel"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      chn = value->u.integer;
      Value_destroy(value);
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
    }

  if (eval(value, (const char *)0))
    {
      if (value->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      width = value->u.integer;
      Value_destroy(value);
      if (g_pass == INTERPRET && FS_width(chn, width) == -1)
        {
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
    }

  if (g_pc.token->type == T_COMMA)
    {
      ++g_pc.token;
      if (eval(value, _("zone width"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      width = value->u.integer;
      Value_destroy(value);
      if (g_pass == INTERPRET && FS_zone(chn, width) == -1)
        {
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_WRITE(struct Value *value)
{
  int chn = STDCHANNEL;
  int comma = 0;

  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
      if (eval(value, _("channel"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      chn = value->u.integer;
      Value_destroy(value);
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
    }

  while (1)
    {
      if (eval(value, (const char *)0))
        {
          if (value->type == V_ERROR)
            {
              return value;
            }

          if (g_pass == INTERPRET)
            {
              struct String s;

              String_new(&s);
              if (comma)
                {
                  String_appendChar(&s, ',');
                }

              if (FS_putString(chn, Value_toWrite(value, &s)) == -1)
                {
                  Value_destroy(value);
                  return Value_new_ERROR(value, IOERROR, FS_errmsg);
                }

              if (FS_flush(chn) == -1)
                {
                  return Value_new_ERROR(value, IOERROR, FS_errmsg);
                }

              String_destroy(&s);
            }

          Value_destroy(value);
          comma = 1;
        }
      else if (g_pc.token->type == T_COMMA ||
               g_pc.token->type == T_SEMICOLON)
        {
          ++g_pc.token;
        }
      else
        {
          break;
        }
    }

  if (g_pass == INTERPRET)
    {
      FS_putChar(chn, '\n');
      if (FS_flush(chn) == -1)
        {
          return Value_new_ERROR(value, IOERROR, FS_errmsg);
        }
    }

  return (struct Value *)0;
}

struct Value *stmt_XREF(struct Value *value)
{
  g_stack.resumeable = 0;
  ++g_pc.token;
  if (g_pass == INTERPRET)
    {
      if (!g_program.runnable && compileProgram(value, 1)->type == V_ERROR)
        {
          return value;
        }

      Program_xref(&g_program, STDCHANNEL);
    }

  return (struct Value *)0;
}

struct Value *stmt_ZONE(struct Value *value)
{
  int chn = STDCHANNEL, width;

  ++g_pc.token;
  if (g_pc.token->type == T_CHANNEL)
    {
      ++g_pc.token;
      if (eval(value, _("channel"))->type == V_ERROR ||
          Value_retype(value, V_INTEGER)->type == V_ERROR)
        {
          return value;
        }

      chn = value->u.integer;
      Value_destroy(value);
      if (g_pc.token->type == T_COMMA)
        {
          ++g_pc.token;
        }
    }

  if (eval(value, _("zone width"))->type == V_ERROR ||
      Value_retype(value, V_INTEGER)->type == V_ERROR)
    {
      return value;
    }

  width = value->u.integer;
  Value_destroy(value);
  if (g_pass == INTERPRET && FS_zone(chn, width) == -1)
    {
      return Value_new_ERROR(value, IOERROR, FS_errmsg);
    }

  return (struct Value *)0;
}
