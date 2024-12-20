/****************************************************************************
 * apps/interpreters/bas/bas_program.c
 *
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 1999-2014 Michael Haardt
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bas_auto.h"
#include "bas_error.h"
#include "bas_fs.h"
#include "bas_program.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define _(String) String

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The list of line numbers is circular, which avoids the need to have one
 * extra pointer for the head (for ordered output).  Instead only a pointer
 * to the tail is needed.  The tail's next element is the head of the list.
 *
 * tail --> last element <-- ... <-- first element <--,
 *              \                                   /
 *               \_________________________________/
 */

struct Xref
  {
    const void *key;
    struct LineNumber
      {
        struct Pc line;
        struct LineNumber *next;
      }
    *lines;
    struct Xref *l;
    struct Xref *r;
  };

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void Xref_add(struct Xref **root,
                     int (*cmp) (const void *, const void *),
                     const void *key, struct Pc *line)
{
  int res;
  struct LineNumber **tail;
  struct LineNumber *new;

  while (*root && (res = cmp(key, (*root)->key)))
    {
      root = (res < 0) ? &(*root)->l : &(*root)->r;
    }

  if (*root == (struct Xref *)0)
    {
      *root = malloc(sizeof(struct Xref));
      (*root)->key = key;
      (*root)->l = (*root)->r = (struct Xref *)0;

      /* create new circular list */

      (*root)->lines = new = malloc(sizeof(struct LineNumber));
      new->line = *line;
      new->next = new;
    }
  else
    {
      /* add to existing circular list */

      tail = &(*root)->lines;
      if ((*tail)->line.line != line->line)
        {
          new = malloc(sizeof(struct LineNumber));
          new->line = *line;
          new->next = (*tail)->next;
          (*tail)->next = new;
          *tail = new;
        }
    }
}

static void Xref_destroy(struct Xref *root)
{
  if (root)
    {
      struct LineNumber *cur;
      struct LineNumber *next;
      struct LineNumber *tail;

      Xref_destroy(root->l);
      Xref_destroy(root->r);
      cur = tail = root->lines;
      do
        {
          next = cur->next;
          free(cur);
          cur = next;
        }
      while (cur != tail);

      free(root);
    }
}

static void Xref_print(struct Xref *root,
                       void (*print) (const void *key, struct Program * p,
                                      int chn), struct Program *p, int chn)
{
  if (root)
    {
      const struct LineNumber *cur;
      const struct LineNumber *tail;

      Xref_print(root->l, print, p, chn);
      print(root->key, p, chn);
      cur = tail = root->lines;
      do
        {
          char buf[128];

          cur = cur->next;
          if (FS_charpos(chn) > 72)
            {
              FS_putChars(chn, "\n        ");
            }

          snprintf(buf, sizeof(buf), " %ld",
                   Program_lineNumber(p, &cur->line));
          FS_putChars(chn, buf);
        }
      while (cur != tail);

      FS_putChar(chn, '\n');
      Xref_print(root->r, print, p, chn);
    }
}

static int cmpLine(const void *a, const void *b)
{
  const register struct Pc *pcA = (const struct Pc *)a;
  const register struct Pc *pcB = (const struct Pc *)b;

  return pcA->line - pcB->line;
}

static void printLine(const void *k, struct Program *p, int chn)
{
  char buf[80];

  snprintf(buf, sizeof(buf), "%8ld",
           Program_lineNumber(p, (const struct Pc *)k));
  FS_putChars(chn, buf);
}

static int cmpName(const void *a, const void *b)
{
  const register char *funcA = (const char *)a;
  const register char *funcB = (const char *)b;

  return strcmp(funcA, funcB);
}

static void printName(const void *k, struct Program *p, int chn)
{
  size_t len = strlen((const char *)k);

  FS_putChars(chn, (const char *)k);
  if (len < 8)
    {
      FS_putChars(chn, &("        "[len]));
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

struct Program *Program_new(struct Program *self)
{
  self->trace = 0;
  self->size = 0;
  self->numbered = 1;
  self->capacity = 0;
  self->runnable = 0;
  self->unsaved = 0;
  self->code = (struct Token **)0;
  self->scope = (struct Scope *)0;
  String_new(&self->name);
  return self;
}

void Program_destroy(struct Program *self)
{
  while (self->size)
    {
      Token_destroy(self->code[--self->size]);
    }

  if (self->capacity)
    {
      free(self->code);
    }

  self->code = (struct Token **)0;
  self->scope = (struct Scope *)0;
  String_destroy(&self->name);
}

void Program_norun(struct Program *self)
{
  self->runnable = 0;
  self->scope = (struct Scope *)0;
}

void Program_store(struct Program *self, struct Token *line, long int where)
{
  int i;

  assert(line->type == T_INTEGER || line->type == T_UNNUMBERED);
  self->runnable = 0;
  self->unsaved = 1;
  if (line->type == T_UNNUMBERED)
    {
      self->numbered = 0;
    }

  if (where)
    {
      int last = -1;

      for (i = 0; i < self->size; ++i)
        {
          assert(self->code[i]->type == T_INTEGER ||
                 self->code[i]->type == T_UNNUMBERED);
          if (where > last && where < self->code[i]->u.integer)
            {
              if ((self->size + 1) >= self->capacity)
                {
                  self->code =
                    realloc(self->code,
                            sizeof(struct Token *) *
                            (self->capacity ? (self->capacity *=
                                               2) : (self->capacity = 256)));
                }

              memmove(&self->code[i + 1], &self->code[i],
                      (self->size - i) * sizeof(struct Token *));
              self->code[i] = line;
              ++self->size;
              return;
            }
          else if (where == self->code[i]->u.integer)
            {
              Token_destroy(self->code[i]);
              self->code[i] = line;
              return;
            }

          last = self->code[i]->u.integer;
        }
    }
  else
    {
      i = self->size;
    }

  if ((self->size + 1) >= self->capacity)
    {
      self->code =
        realloc(self->code,
                sizeof(struct Token *) *
                (self->capacity ? (self->capacity *= 2)
                 : (self->capacity = 256)));
    }

  self->code[i] = line;
  ++self->size;
}

void Program_delete(struct Program *self, const struct Pc *from,
                    const struct Pc *to)
{
  int i;
  int first;
  int last;

  self->runnable = 0;
  self->unsaved = 1;
  first = from ? from->line : 0;
  last = to ? to->line : self->size - 1;
  for (i = first; i <= last; ++i)
    {
      Token_destroy(self->code[i]);
    }

  if ((last + 1) != self->size)
    {
      memmove(&self->code[first], &self->code[last + 1],
              (self->size - last + 1) * sizeof(struct Token *));
    }

  self->size -= (last - first + 1);
}

void Program_addScope(struct Program *self, struct Scope *scope)
{
  struct Scope *s;

  s = self->scope;
  self->scope = scope;
  scope->next = s;
}

struct Pc *Program_goLine(struct Program *self, long int line, struct Pc *pc)
{
  int i;

  for (i = 0; i < self->size; ++i)
    {
      if (self->code[i]->type == T_INTEGER &&
          line == self->code[i]->u.integer)
        {
          pc->line = i;
          pc->token = self->code[i] + 1;
          return pc;
        }
    }

  return (struct Pc *)0;
}

struct Pc *Program_fromLine(struct Program *self, long int line,
                            struct Pc *pc)
{
  int i;

  for (i = 0; i < self->size; ++i)
    {
      if (self->code[i]->type == T_INTEGER &&
          self->code[i]->u.integer >= line)
        {
          pc->line = i;
          pc->token = self->code[i] + 1;
          return pc;
        }
    }

  return (struct Pc *)0;
}

struct Pc *Program_toLine(struct Program *self, long int line, struct Pc *pc)
{
  int i;

  for (i = self->size - 1; i >= 0; --i)
    {
      if (self->code[i]->type == T_INTEGER &&
          self->code[i]->u.integer <= line)
        {
          pc->line = i;
          pc->token = self->code[i] + 1;
          return pc;
        }
    }

  return (struct Pc *)0;
}

int Program_scopeCheck(struct Program *self, struct Pc *pc, struct Pc *fn)
{
  struct Scope *scope;

  if (fn == (struct Pc *)0)     /* jump from global block must go to global pc */
    {
      for (scope = self->scope; scope; scope = scope->next)
        {
          if (pc->line < scope->begin.line)
            {
              continue;
            }

          if (pc->line == scope->begin.line &&
              pc->token <= scope->begin.token)
            {
              continue;
            }

          if (pc->line > scope->end.line)
            {
              continue;
            }

          if (pc->line == scope->end.line && pc->token > scope->end.token)
            {
              continue;
            }

          return -1;
        }
    }

  /* jump from local block must go to local block */

  else
    {
      scope = &(fn->token + 1)->u.identifier->sym->u.sub.u.def.scope;
      if (pc->line < scope->begin.line)
        {
          return -1;
        }

      if (pc->line == scope->begin.line && pc->token <= scope->begin.token)
        {
          return -1;
        }

      if (pc->line > scope->end.line)
        {
          return -1;
        }

      if (pc->line == scope->end.line && pc->token > scope->end.token)
        {
          return -1;
        }
    }

  return 0;
}

struct Pc *Program_dataLine(struct Program *self, long int line,
                            struct Pc *pc)
{
  if ((pc = Program_goLine(self, line, pc)) == (struct Pc *)0)
    {
      return (struct Pc *)0;
    }

  while (pc->token->type != T_DATA)
    {
      if (pc->token->type == T_EOL)
        {
          return (struct Pc *)0;
        }
      else
        {
          ++pc->token;
        }
    }

  return pc;
}

struct Pc *Program_imageLine(struct Program *self, long int line,
                             struct Pc *pc)
{
  if ((pc = Program_goLine(self, line, pc)) == (struct Pc *)0)
    {
      return (struct Pc *)0;
    }

  while (pc->token->type != T_IMAGE)
    {
      if (pc->token->type == T_EOL)
        {
          return (struct Pc *)0;
        }
      else
        {
          ++pc->token;
        }
    }

  ++pc->token;
  if (pc->token->type != T_STRING)
    {
      return (struct Pc *)0;
    }

  return pc;
}

long int Program_lineNumber(const struct Program *self, const struct Pc *pc)
{
  if (pc->line == -1)
    {
      return 0;
    }

  if (self->numbered)
    {
      return (self->code[pc->line]->u.integer);
    }
  else
    {
      return (pc->line + 1);
    }
}

struct Pc *Program_beginning(struct Program *self, struct Pc *pc)
{
  if (self->size == 0)
    {
      return (struct Pc *)0;
    }
  else
    {
      pc->line = 0;
      pc->token = self->code[0] + 1;
      return pc;
    }
}

struct Pc *Program_end(struct Program *self, struct Pc *pc)
{
  if (self->size == 0)
    {
      return (struct Pc *)0;
    }
  else
    {
      pc->line = self->size - 1;
      pc->token = self->code[self->size - 1];
      while (pc->token->type != T_EOL)
        {
          ++pc->token;
        }

      return pc;
    }
}

struct Pc *Program_nextLine(struct Program *self, struct Pc *pc)
{
  if (pc->line + 1 == self->size)
    {
      return (struct Pc *)0;
    }
  else
    {
      pc->token = self->code[++pc->line] + 1;
      return pc;
    }
}

int Program_skipEOL(struct Program *self, struct Pc *pc, int dev, int tr)
{
  if (pc->token->type == T_EOL)
    {
      if (pc->line == -1 || pc->line + 1 == self->size)
        {
          return 0;
        }
      else
        {
          pc->token = self->code[++pc->line] + 1;
          Program_trace(self, pc, dev, tr);
          return 1;
        }
    }
  else
    {
      return 1;
    }
}

void Program_trace(struct Program *self, struct Pc *pc, int dev, int tr)
{
  if (tr && self->trace && pc->line != -1)
    {
      char buf[40];

      snprintf(buf, sizeof(buf), "<%ld>\n",
               self->code[pc->line]->u.integer);
      FS_putChars(dev, buf);
    }
}

void Program_PCtoError(struct Program *self, struct Pc *pc, struct Value *v)
{
  struct String s;

  String_new(&s);
  if (pc->line >= 0)
    {
      if (pc->line < (self->size - 1) || pc->token->type != T_EOL)
        {
          String_appendPrintf(&s, _(" in line %ld at:\n"),
                              Program_lineNumber(self, pc));
          Token_toString(self->code[pc->line], (struct Token *)0, &s,
                         (int *)0, -1);
          Token_toString(self->code[pc->line], pc->token, &s, (int *)0, -1);
          String_appendPrintf(&s, "^\n");
        }
      else
        {
          String_appendPrintf(&s, _(" at: end of program\n"));
        }
    }
  else
    {
      String_appendPrintf(&s, _(" at: "));
      if (pc->token->type != T_EOL)
        {
          Token_toString(pc->token, (struct Token *)0, &s, (int *)0, -1);
        }
      else
        {
          String_appendPrintf(&s, _("end of line\n"));
        }
    }

  Value_errorSuffix(v, s.character);
  String_destroy(&s);
}

struct Value *Program_merge(struct Program *self, int dev,
                            struct Value *value)
{
  struct String s;
  int l;
  int errcode = 0;

  l = 0;
  while (String_new(&s), (errcode = FS_appendToString(dev, &s, 1)) != -1 &&
         s.length)
    {
      struct Token *line;

      ++l;
      if (l != 1 || s.character[0] != '#')
        {
          line = Token_newCode(s.character);
          if (line->type == T_INTEGER && line->u.integer > 0)
            {
              Program_store(self, line,
                            self->numbered ? line->u.integer : 0);
            }
          else if (line->type == T_UNNUMBERED)
            {
              Program_store(self, line, 0);
            }
          else
            {
              Token_destroy(line);
              return Value_new_ERROR(value, INVALIDLINE, l);
            }
        }

      String_destroy(&s);
    }

  String_destroy(&s);
  if (errcode)
    {
      return Value_new_ERROR(value, IOERROR, FS_errmsg);
    }

  return (struct Value *)0;
}

int Program_lineNumberWidth(struct Program *self)
{
  int i;
  int w = 0;

  for (i = 0; i < self->size; ++i)
    {
      if (self->code[i]->type == T_INTEGER)
        {
          int nw;
          int ln;

          for (ln = self->code[i]->u.integer, nw = 1; ln /= 10; ++nw);
          if (nw > w)
            {
              w = nw;
            }
        }
    }

  return w;
}

struct Value *Program_list(struct Program *self, int dev, int watchIntr,
                           struct Pc *from, struct Pc *to,
                           struct Value *value)
{
  int i;
  int w;
  int indent = 0;
  struct String s;

  w = Program_lineNumberWidth(self);
  for (i = 0; i < self->size; ++i)
    {
      String_new(&s);
      Token_toString(self->code[i], (struct Token *)0, &s, &indent, w);
      if ((from == (struct Pc *)0 || from->line <= i) &&
          (to == (struct Pc *)0 || to->line >= i))
        {
          if (FS_putString(dev, &s) == -1)
            {
              return Value_new_ERROR(value, IOERROR, FS_errmsg);
            }
        }

      String_destroy(&s);
    }

  return (struct Value *)0;
}

struct Value *Program_analyse(struct Program *self, struct Pc *pc,
                              struct Value *value)
{
  int i;

  for (i = 0; i < self->size; ++i)
    {
      pc->token = self->code[i];
      pc->line = i;
      if (pc->token->type == T_INTEGER || pc->token->type == T_UNNUMBERED)
        {
          ++pc->token;
        }

      for (; ; )
        {
          if (pc->token->type == T_GOTO || pc->token->type == T_RESUME ||
              pc->token->type == T_RETURN || pc->token->type == T_END ||
              pc->token->type == T_STOP)
            {
              ++pc->token;
              while (pc->token->type == T_INTEGER)
                {
                  ++pc->token;
                  if (pc->token->type == T_COMMA)
                    {
                      ++pc->token;
                    }
                  else
                    {
                      break;
                    }
                }

              if (pc->token->type == T_COLON)
                {
                  ++pc->token;
                  switch (pc->token->type)
                    {
                    case T_EOL:
                    case T_DEFPROC:
                    case T_SUB:
                    case T_DEFFN:
                    case T_FUNCTION:
                    case T_COLON:
                    case T_REM:
                    case T_QUOTE:
                      break;    /* those are fine to be unreachable */

                    default:
                      return Value_new_ERROR(value, UNREACHABLE);
                    }
                }
            }

          if (pc->token->type == T_EOL)
            {
              break;
            }
          else
            {
              ++pc->token;
            }
        }
    }

  return (struct Value *)0;
}

void Program_renum(struct Program *self, int first, int inc)
{
  int i;
  struct Token *token;

  for (i = 0; i < self->size; ++i)
    {
      for (token = self->code[i]; token->type != T_EOL; )
        {
          if (token->type == T_GOTO || token->type == T_GOSUB ||
              token->type == T_RESTORE || token->type == T_RESUME ||
              token->type == T_USING)
            {
              ++token;
              while (token->type == T_INTEGER)
                {
                  struct Pc dst;

                  if (Program_goLine(self, token->u.integer, &dst))
                    {
                      token->u.integer = first + dst.line * inc;
                    }

                  ++token;
                  if (token->type == T_COMMA)
                    {
                      ++token;
                    }
                  else
                    {
                      break;
                    }
                }
            }
          else
            {
              ++token;
            }
        }
    }

  for (i = 0; i < self->size; ++i)
    {
      assert(self->code[i]->type == T_INTEGER ||
             self->code[i]->type == T_UNNUMBERED);
      self->code[i]->type = T_INTEGER;
      self->code[i]->u.integer = first + i * inc;
    }

  self->numbered = 1;
  self->runnable = 0;
  self->unsaved = 1;
}

void Program_unnum(struct Program *self)
{
  char *ref;
  int i;
  struct Token *token;

  ref = malloc(self->size);
  memset(ref, 0, self->size);
  for (i = 0; i < self->size; ++i)
    {
      for (token = self->code[i]; token->type != T_EOL; ++token)
        {
          if (token->type == T_GOTO || token->type == T_GOSUB ||
              token->type == T_RESTORE || token->type == T_RESUME)
            {
              ++token;
              while (token->type == T_INTEGER)
                {
                  struct Pc dst;

                  if (Program_goLine(self, token->u.integer, &dst))
                    {
                      ref[dst.line] = 1;
                    }

                  ++token;
                  if (token->type == T_COMMA)
                    {
                      ++token;
                    }
                  else
                    {
                      break;
                    }
                }
            }
        }
    }

  for (i = 0; i < self->size; ++i)
    {
      assert(self->code[i]->type == T_INTEGER ||
             self->code[i]->type == T_UNNUMBERED);
      if (!ref[i])
        {
          self->code[i]->type = T_UNNUMBERED;
          self->numbered = 0;
        }
    }

  free(ref);
  self->runnable = 0;
  self->unsaved = 1;
}

int Program_setname(struct Program *self, const char *filename)
{
  if (self->name.length)
    {
      String_delete(&self->name, 0, self->name.length);
    }

  if (filename)
    {
      return String_appendChars(&self->name, filename);
    }
  else
    {
      return 0;
    }
}

void Program_xref(struct Program *self, int chn)
{
  struct Pc pc;
  struct Xref *func;
  struct Xref *var;
  struct Xref *gosub;
  struct Xref *goto_;
  int nl = 0;

  assert(self->runnable);
  func = (struct Xref *)0;
  var = (struct Xref *)0;
  gosub = (struct Xref *)0;
  goto_ = (struct Xref *)0;

  for (pc.line = 0; pc.line < self->size; ++pc.line)
    {
      struct On *on;

      for (on = (struct On *)0, pc.token = self->code[pc.line];
           pc.token->type != T_EOL; ++pc.token)
        {
          switch (pc.token->type)
            {
            case T_ON:
              {
                on = &pc.token->u.on;
                break;
              }

            case T_GOTO:
              {
                if (on)
                  {
                    int key;

                    for (key = 0; key < on->pcLength; ++key)
                      Xref_add(&goto_, cmpLine, &on->pc[key], &pc);
                    on = (struct On *)0;
                  }
                else
                  Xref_add(&goto_, cmpLine, &pc.token->u.gotopc, &pc);
                break;
              }

            case T_GOSUB:
              {
                if (on)
                  {
                    int key;

                    for (key = 0; key < on->pcLength; ++key)
                      Xref_add(&gosub, cmpLine, &on->pc[key], &pc);
                    on = (struct On *)0;
                  }
                else
                  Xref_add(&gosub, cmpLine, &pc.token->u.gosubpc, &pc);
                break;
              }

            case T_DEFFN:
            case T_DEFPROC:
            case T_FUNCTION:
            case T_SUB:
              {
                ++pc.token;
                Xref_add(&func, cmpName, &pc.token->u.identifier->name, &pc);
                break;
              }

            default:
              break;
            }
        }
    }

  for (pc.line = 0; pc.line < self->size; ++pc.line)
    {
      for (pc.token = self->code[pc.line]; pc.token->type != T_EOL;
           ++pc.token)
        {
          switch (pc.token->type)
            {
            case T_DEFFN:
            case T_DEFPROC:
            case T_FUNCTION:
            case T_SUB:        /* skip identifier already added above */
              {
                ++pc.token;
                break;
              }

            case T_IDENTIFIER:
              {
                /* formal parameters have no assigned symbol */

                if (pc.token->u.identifier->sym)
                  {
                    switch (pc.token->u.identifier->sym->type)
                      {
                      case GLOBALVAR:
                        {
                          Xref_add(&var, cmpName,
                                   &pc.token->u.identifier->name, &pc);
                          break;
                        }

                      case USERFUNCTION:
                        {
                          Xref_add(&func, cmpName,
                                   &pc.token->u.identifier->name, &pc);
                          break;
                        }

                      default:
                        break;
                      }
                  }
                break;
              }

            default:
              break;
            }
        }
    }

  if (func)
    {
      FS_putChars(chn, _("Function Referenced in line\n"));
      Xref_print(func, printName, self, chn);
      Xref_destroy(func);
      nl = 1;
    }

  if (var)
    {
      if (nl)
        {
          FS_putChar(chn, '\n');
        }

      FS_putChars(chn, _("Variable Referenced in line\n"));
      Xref_print(var, printName, self, chn);
      Xref_destroy(func);
      nl = 1;
    }

  if (gosub)
    {
      if (nl)
        {
          FS_putChar(chn, '\n');
        }

      FS_putChars(chn, _("Gosub    Referenced in line\n"));
      Xref_print(gosub, printLine, self, chn);
      Xref_destroy(gosub);
      nl = 1;
    }

  if (goto_)
    {
      if (nl)
        {
          FS_putChar(chn, '\n');
        }

      FS_putChars(chn, _("Goto     Referenced in line\n"));
      Xref_print(goto_, printLine, self, chn);
      Xref_destroy(goto_);
    }
}
