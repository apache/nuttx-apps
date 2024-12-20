/****************************************************************************
 * apps/interpreters/bas/bas_auto.c
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "bas_auto.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define INCREASE_STACK 16
#define _(String) String

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* interpretation methods */

struct Auto *Auto_new(struct Auto *self)
{
  self->stackPointer = 0;
  self->stackCapacity = 0;
  self->framePointer = 0;
  self->frameSize = 0;
  self->onerror.line = -1;
  self->erl = 0;
  Value_new_NIL(&self->err);
  Value_new_NIL(&self->lastdet);
  self->begindata.line = -1;
  self->slot = (union AutoSlot *)0;
  self->cur = self->all = (struct Symbol *)0;
  return self;
}

void Auto_destroy(struct Auto *self)
{
  struct Symbol *l;

  Value_destroy(&self->err);
  Value_destroy(&self->lastdet);
  if (self->stackCapacity)
    {
      free(self->slot);
    }

  for (l = self->all; l != (struct Symbol *)0; )
    {
      struct Symbol *f;

      f = l;
      l = l->next;
      free(f->name);
      free(f);
    }
}

struct Var *Auto_pushArg(struct Auto *self)
{
  if ((self->stackPointer + 1) >= self->stackCapacity)
    {
      self->slot =
        realloc(self->slot,
                sizeof(self->slot[0]) *
                (self->
                 stackCapacity ? (self->stackCapacity =
                                  self->stackPointer +
                                  INCREASE_STACK) : (self->stackCapacity =
                                                     INCREASE_STACK)));
    }

  return &self->slot[self->stackPointer++].var;
}

void Auto_pushFuncRet(struct Auto *self, int firstarg, struct Pc *pc)
{
  if (self->stackPointer + 2 >= self->stackCapacity)
    {
      self->slot =
        realloc(self->slot,
                sizeof(self->slot[0]) *
                (self->
                 stackCapacity ? (self->stackCapacity =
                                  self->stackCapacity +
                                  INCREASE_STACK) : (self->stackCapacity =
                                                     INCREASE_STACK)));
    }

  self->slot[self->stackPointer].retException.onerror = self->onerror;
  self->slot[self->stackPointer].retException.resumeable = self->resumeable;
  ++self->stackPointer;
  self->slot[self->stackPointer].retFrame.pc = *pc;
  self->slot[self->stackPointer].retFrame.framePointer = self->framePointer;
  self->slot[self->stackPointer].retFrame.frameSize = self->frameSize;
  ++self->stackPointer;
  self->framePointer = firstarg;
  self->frameSize = self->stackPointer - firstarg;
  self->onerror.line = -1;
}

void Auto_pushGosubRet(struct Auto *self, struct Pc *pc)
{
  if ((self->stackPointer + 1) >= self->stackCapacity)
    {
      self->slot =
        realloc(self->slot,
                sizeof(self->slot[0]) *
                (self->
                 stackCapacity ? (self->stackCapacity =
                                  self->stackPointer +
                                  INCREASE_STACK) : (self->stackCapacity =
                                                     INCREASE_STACK)));
    }

  self->slot[self->stackPointer].retFrame.pc = *pc;
  ++self->stackPointer;
}

struct Var *Auto_local(struct Auto *self, int l)
{
  assert(self->frameSize > (l + 2));
  return &(self->slot[self->framePointer + l].var);
}

int Auto_funcReturn(struct Auto *self, struct Pc *pc)
{
  int retException;
  int retFrame;
  int i;

  if (self->stackPointer == 0)
    {
      return 0;
    }

  assert(self->frameSize);
  retFrame = self->framePointer + self->frameSize - 1;
  retException = self->framePointer + self->frameSize - 2;
  assert(retException >= 0 && retFrame < self->stackPointer);
  for (i = 0; i < self->frameSize - 2; ++i)
    {
      Var_destroy(&self->slot[self->framePointer + i].var);
    }

  self->stackPointer = self->framePointer;
  if (pc != (struct Pc *)0)
    {
      *pc = self->slot[retFrame].retFrame.pc;
    }

  self->frameSize = self->slot[retFrame].retFrame.frameSize;
  self->framePointer = self->slot[retFrame].retFrame.framePointer;
  self->onerror = self->slot[retException].retException.onerror;
  return 1;
}

int Auto_gosubReturn(struct Auto *self, struct Pc *pc)
{
  if (self->stackPointer <= self->framePointer + self->frameSize)
    {
      return 0;
    }

  --self->stackPointer;
  if (pc)
    {
      *pc = self->slot[self->stackPointer].retFrame.pc;
    }

  return 1;
}

void Auto_frameToError(struct Auto *self,
                       struct Program *program, struct Value *v)
{
  struct Pc p;
  int framePointer;
  int frameSize;
  int retFrame;
  int i = self->stackPointer;

  framePointer = self->framePointer;
  frameSize = self->frameSize;
  while (i > framePointer + frameSize)
    {
      p = self->slot[--i].retFrame.pc;
      Value_errorSuffix(v, _("Called"));
      Program_PCtoError(program, &p, v);
    }

  if (i)
    {
      retFrame = framePointer + frameSize - 1;
      p = self->slot[retFrame].retFrame.pc;
      Value_errorSuffix(v, _("Proc Called"));
      Program_PCtoError(program, &p, v);
    }
}

void Auto_setError(struct Auto *self, long int line,
                   struct Pc *pc, struct Value *v)
{
  self->erpc = *pc;
  self->erl = line;
  Value_destroy(&self->err);
  Value_clone(&self->err, v);
}

/* compilation methods */

int Auto_find(struct Auto *self, struct Identifier *ident)
{
  struct Symbol *find;

  for (find = self->cur; find != (struct Symbol *)0; find = find->next)
    {
      const char *s = ident->name;
      const char *r = find->name;

      while (*s && tolower(*s) == tolower(*r))
        {
          ++s;
          ++r;
        }

      if (tolower(*s) == tolower(*r))
        {
          ident->sym = find;
          return 1;
        }
    }

  return 0;
}

int Auto_variable(struct Auto *self, const struct Identifier *ident)
{
  struct Symbol **tail;
  int offset;

  for (offset = 0, tail = &self->cur;
       *tail != (struct Symbol *)0;
       tail = &(*tail)->next, ++offset)
    {
      const char *s = ident->name;
      const char *r = (*tail)->name;

      while (*s && tolower(*s) == tolower(*r))
        {
          ++s;
          ++r;
        }

      if (tolower(*s) == tolower(*r))
        {
          return 0;
        }
    }

  (*tail) = malloc(sizeof(struct Symbol));
  (*tail)->next = (struct Symbol *)0;
  (*tail)->name = strdup(ident->name);
  (*tail)->type = LOCALVAR;
  (*tail)->u.local.type = ident->defaultType;

  /* the offset -1 of the V_VOID procedure return symbol is ok,
   * it is not used
   */

  (*tail)->u.local.offset =
    offset - (self->cur->u.local.type == V_VOID ? 1 : 0);
  return 1;
}

enum ValueType Auto_argType(const struct Auto *self, int l)
{
  struct Symbol *find;
  int offset;

  if (self->cur->u.local.type == V_VOID)
    {
      ++l;
    }

  for (offset = 0, find = self->cur; l != offset;
       find = find->next, ++offset)
    {
      assert(find != (struct Symbol *)0);
    }

  assert(find != (struct Symbol *)0);
  return find->u.local.type;
}

enum ValueType Auto_varType(const struct Auto *self, struct Symbol *sym)
{
  struct Symbol *find;

  for (find = self->cur;
       find->u.local.offset != sym->u.local.offset;
       find = find->next)
    {
      assert(find != (struct Symbol *)0);
    }

  assert(find != (struct Symbol *)0);
  return find->u.local.type;
}

void Auto_funcEnd(struct Auto *self)
{
  struct Symbol **tail;

  for (tail = &self->all; *tail != NULL; tail = &(*tail)->next);
  *tail = self->cur;
  self->cur = (struct Symbol *)0;
}
