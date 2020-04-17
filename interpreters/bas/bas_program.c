/****************************************************************************
 * apps/interpreters/bas/bas_program.c
 * Program storage.
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
      } *lines;
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

          sprintf(buf, " %ld", Program_lineNumber(p, &cur->line));
          FS_putChars(chn, buf);
        }
      while (cur != tail);

      FS_putChar(chn, '\n');
      Xref_print(root->r, print, p, chn);
    }
}

static int cmpLine(const void *a, const void *b)
{
  const register struct Pc *pcA = (const struct Pc *)a, *pcB =
    (const struct Pc *)b;

  return pcA->line - pcB->line;
}

static void printLine(const void *k, struct Program *p, int chn)
{
  char buf[80];

  sprintf(buf, "%8ld", Program_lineNumber(p, (const struct Pc *)k));
  FS_putChars(chn, buf);
}

static int cmpName(const void *a, const void *b)
{
  const register char *funcA = (const char *)a, *funcB = (const char *)b;

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

struct Program *Program_new(struct Program *this)
{
  this->trace = 0;
  this->size = 0;
  this->numbered = 1;
  this->capacity = 0;
  this->runnable = 0;
  this->unsaved = 0;
  this->code = (struct Token **)0;
  this->scope = (struct Scope *)0;
  String_new(&this->name);
  return this;
}

void Program_destroy(struct Program *this)
{
  while (this->size)
    {
      Token_destroy(this->code[--this->size]);
    }

  if (this->capacity)
    {
      free(this->code);
    }

  this->code = (struct Token **)0;
  this->scope = (struct Scope *)0;
  String_destroy(&this->name);
}

void Program_norun(struct Program *this)
{
  this->runnable = 0;
  this->scope = (struct Scope *)0;
}

void Program_store(struct Program *this, struct Token *line, long int where)
{
  int i;

  assert(line->type == T_INTEGER || line->type == T_UNNUMBERED);
  this->runnable = 0;
  this->unsaved = 1;
  if (line->type == T_UNNUMBERED)
    {
      this->numbered = 0;
    }

  if (where)
    {
      int last = -1;

      for (i = 0; i < this->size; ++i)
        {
          assert(this->code[i]->type == T_INTEGER ||
                 this->code[i]->type == T_UNNUMBERED);
          if (where > last && where < this->code[i]->u.integer)
            {
              if ((this->size + 1) >= this->capacity)
                {
                  this->code =
                    realloc(this->code,
                            sizeof(struct Token *) *
                            (this->capacity ? (this->capacity *=
                                               2) : (this->capacity = 256)));
                }

              memmove(&this->code[i + 1], &this->code[i],
                      (this->size - i) * sizeof(struct Token *));
              this->code[i] = line;
              ++this->size;
              return;
            }
          else if (where == this->code[i]->u.integer)
            {
              Token_destroy(this->code[i]);
              this->code[i] = line;
              return;
            }

          last = this->code[i]->u.integer;
        }
    }
  else
    {
      i = this->size;
    }

  if ((this->size + 1) >= this->capacity)
    {
      this->code =
        realloc(this->code,
                sizeof(struct Token *) *
                (this->capacity ? (this->capacity *= 2)
                 : (this->capacity = 256)));
    }

  this->code[i] = line;
  ++this->size;
}

void Program_delete(struct Program *this, const struct Pc *from,
                    const struct Pc *to)
{
  int i, first, last;

  this->runnable = 0;
  this->unsaved = 1;
  first = from ? from->line : 0;
  last = to ? to->line : this->size - 1;
  for (i = first; i <= last; ++i)
    {
      Token_destroy(this->code[i]);
    }

  if ((last + 1) != this->size)
    {
      memmove(&this->code[first], &this->code[last + 1],
              (this->size - last + 1) * sizeof(struct Token *));
    }

  this->size -= (last - first + 1);
}

void Program_addScope(struct Program *this, struct Scope *scope)
{
  struct Scope *s;

  s = this->scope;
  this->scope = scope;
  scope->next = s;
}

struct Pc *Program_goLine(struct Program *this, long int line, struct Pc *pc)
{
  int i;

  for (i = 0; i < this->size; ++i)
    {
      if (this->code[i]->type == T_INTEGER &&
          line == this->code[i]->u.integer)
        {
          pc->line = i;
          pc->token = this->code[i] + 1;
          return pc;
        }
    }

  return (struct Pc *)0;
}

struct Pc *Program_fromLine(struct Program *this, long int line,
                            struct Pc *pc)
{
  int i;

  for (i = 0; i < this->size; ++i)
    {
      if (this->code[i]->type == T_INTEGER &&
          this->code[i]->u.integer >= line)
        {
          pc->line = i;
          pc->token = this->code[i] + 1;
          return pc;
        }
    }

  return (struct Pc *)0;
}

struct Pc *Program_toLine(struct Program *this, long int line, struct Pc *pc)
{
  int i;

  for (i = this->size - 1; i >= 0; --i)
    {
      if (this->code[i]->type == T_INTEGER &&
          this->code[i]->u.integer <= line)
        {
          pc->line = i;
          pc->token = this->code[i] + 1;
          return pc;
        }
    }

  return (struct Pc *)0;
}

int Program_scopeCheck(struct Program *this, struct Pc *pc, struct Pc *fn)
{
  struct Scope *scope;

  if (fn == (struct Pc *)0)     /* jump from global block must go to global pc */
    {
      for (scope = this->scope; scope; scope = scope->next)
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

struct Pc *Program_dataLine(struct Program *this, long int line,
                            struct Pc *pc)
{
  if ((pc = Program_goLine(this, line, pc)) == (struct Pc *)0)
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

struct Pc *Program_imageLine(struct Program *this, long int line,
                             struct Pc *pc)
{
  if ((pc = Program_goLine(this, line, pc)) == (struct Pc *)0)
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

long int Program_lineNumber(const struct Program *this, const struct Pc *pc)
{
  if (pc->line == -1)
    {
      return 0;
    }

  if (this->numbered)
    {
      return (this->code[pc->line]->u.integer);
    }
  else
    {
      return (pc->line + 1);
    }
}

struct Pc *Program_beginning(struct Program *this, struct Pc *pc)
{
  if (this->size == 0)
    {
      return (struct Pc *)0;
    }
  else
    {
      pc->line = 0;
      pc->token = this->code[0] + 1;
      return pc;
    }
}

struct Pc *Program_end(struct Program *this, struct Pc *pc)
{
  if (this->size == 0)
    {
      return (struct Pc *)0;
    }
  else
    {
      pc->line = this->size - 1;
      pc->token = this->code[this->size - 1];
      while (pc->token->type != T_EOL)
        {
          ++pc->token;
        }

      return pc;
    }
}

struct Pc *Program_nextLine(struct Program *this, struct Pc *pc)
{
  if (pc->line + 1 == this->size)
    {
      return (struct Pc *)0;
    }
  else
    {
      pc->token = this->code[++pc->line] + 1;
      return pc;
    }
}

int Program_skipEOL(struct Program *this, struct Pc *pc, int dev, int tr)
{
  if (pc->token->type == T_EOL)
    {
      if (pc->line == -1 || pc->line + 1 == this->size)
        {
          return 0;
        }
      else
        {
          pc->token = this->code[++pc->line] + 1;
          Program_trace(this, pc, dev, tr);
          return 1;
        }
    }
  else
    {
      return 1;
    }
}

void Program_trace(struct Program *this, struct Pc *pc, int dev, int tr)
{
  if (tr && this->trace && pc->line != -1)
    {
      char buf[40];

      sprintf(buf, "<%ld>\n", this->code[pc->line]->u.integer);
      FS_putChars(dev, buf);
    }
}

void Program_PCtoError(struct Program *this, struct Pc *pc, struct Value *v)
{
  struct String s;

  String_new(&s);
  if (pc->line >= 0)
    {
      if (pc->line < (this->size - 1) || pc->token->type != T_EOL)
        {
          String_appendPrintf(&s, _(" in line %ld at:\n"),
                              Program_lineNumber(this, pc));
          Token_toString(this->code[pc->line], (struct Token *)0, &s,
                         (int *)0, -1);
          Token_toString(this->code[pc->line], pc->token, &s, (int *)0, -1);
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

struct Value *Program_merge(struct Program *this, int dev,
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
              Program_store(this, line,
                            this->numbered ? line->u.integer : 0);
            }
          else if (line->type == T_UNNUMBERED)
            {
              Program_store(this, line, 0);
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

int Program_lineNumberWidth(struct Program *this)
{
  int i, w = 0;

  for (i = 0; i < this->size; ++i)
    {
      if (this->code[i]->type == T_INTEGER)
        {
          int nw, ln;
          for (ln = this->code[i]->u.integer, nw = 1; ln /= 10; ++nw);
          if (nw > w)
            {
              w = nw;
            }
        }
    }

  return w;
}

struct Value *Program_list(struct Program *this, int dev, int watchIntr,
                           struct Pc *from, struct Pc *to,
                           struct Value *value)
{
  int i, w;
  int indent = 0;
  struct String s;

  w = Program_lineNumberWidth(this);
  for (i = 0; i < this->size; ++i)
    {
      String_new(&s);
      Token_toString(this->code[i], (struct Token *)0, &s, &indent, w);
      if ((from == (struct Pc *)0 || from->line <= i) &&
          (to == (struct Pc *)0 || to->line >= i))
        {
          if (FS_putString(dev, &s) == -1)
            {
              return Value_new_ERROR(value, IOERROR, FS_errmsg);
            }

          if (watchIntr)
            {
              return Value_new_ERROR(value, BREAK);
            }
        }

      String_destroy(&s);
    }

  return (struct Value *)0;
}

struct Value *Program_analyse(struct Program *this, struct Pc *pc,
                              struct Value *value)
{
  int i;

  for (i = 0; i < this->size; ++i)
    {
      pc->token = this->code[i];
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

void Program_renum(struct Program *this, int first, int inc)
{
  int i;
  struct Token *token;

  for (i = 0; i < this->size; ++i)
    {
      for (token = this->code[i]; token->type != T_EOL; )
        {
          if (token->type == T_GOTO || token->type == T_GOSUB ||
              token->type == T_RESTORE || token->type == T_RESUME ||
              token->type == T_USING)
            {
              ++token;
              while (token->type == T_INTEGER)
                {
                  struct Pc dst;

                  if (Program_goLine(this, token->u.integer, &dst))
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

  for (i = 0; i < this->size; ++i)
    {
      assert(this->code[i]->type == T_INTEGER ||
             this->code[i]->type == T_UNNUMBERED);
      this->code[i]->type = T_INTEGER;
      this->code[i]->u.integer = first + i * inc;
    }

  this->numbered = 1;
  this->runnable = 0;
  this->unsaved = 1;
}

void Program_unnum(struct Program *this)
{
  char *ref;
  int i;
  struct Token *token;

  ref = malloc(this->size);
  memset(ref, 0, this->size);
  for (i = 0; i < this->size; ++i)
    {
      for (token = this->code[i]; token->type != T_EOL; ++token)
        {
          if (token->type == T_GOTO || token->type == T_GOSUB ||
              token->type == T_RESTORE || token->type == T_RESUME)
            {
              ++token;
              while (token->type == T_INTEGER)
                {
                  struct Pc dst;

                  if (Program_goLine(this, token->u.integer, &dst))
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

  for (i = 0; i < this->size; ++i)
    {
      assert(this->code[i]->type == T_INTEGER ||
             this->code[i]->type == T_UNNUMBERED);
      if (!ref[i])
        {
          this->code[i]->type = T_UNNUMBERED;
          this->numbered = 0;
        }
    }

  free(ref);
  this->runnable = 0;
  this->unsaved = 1;
}

int Program_setname(struct Program *this, const char *filename)
{
  if (this->name.length)
    {
      String_delete(&this->name, 0, this->name.length);
    }

  if (filename)
    {
      return String_appendChars(&this->name, filename);
    }
  else
    {
      return 0;
    }
}

void Program_xref(struct Program *this, int chn)
{
  struct Pc pc;
  struct Xref *func, *var, *gosub, *goto_;
  int nl = 0;

  assert(this->runnable);
  func = (struct Xref *)0;
  var = (struct Xref *)0;
  gosub = (struct Xref *)0;
  goto_ = (struct Xref *)0;

  for (pc.line = 0; pc.line < this->size; ++pc.line)
    {
      struct On *on;

      for (on = (struct On *)0, pc.token = this->code[pc.line];
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

  for (pc.line = 0; pc.line < this->size; ++pc.line)
    {
      for (pc.token = this->code[pc.line]; pc.token->type != T_EOL;
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
      Xref_print(func, printName, this, chn);
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
      Xref_print(var, printName, this, chn);
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
      Xref_print(gosub, printLine, this, chn);
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
      Xref_print(goto_, printLine, this, chn);
      Xref_destroy(goto_);
    }
}
