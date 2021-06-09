/****************************************************************************
 * apps/interpreters/bas/bas_auto.h
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#ifndef __APPS_EXAMPLES_BAS_BAS_AUTO_H
#define __APPS_EXAMPLES_BAS_BAS_AUTO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "bas_programtypes.h"
#include "bas_var.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct Auto
{
  long int stackPointer;
  long int stackCapacity;
  long int framePointer;
  long int frameSize;
  struct Pc onerror;
  union AutoSlot *slot;
  long int erl;
  struct Pc erpc;
  struct Value err;
  struct Value lastdet;
  struct Pc begindata;
  int resumeable;
  struct Symbol *cur,*all; /* should be hung off the funcs/procs */
};

struct AutoFrameSlot
{
  long int framePointer;
  long int frameSize;
  struct Pc pc;
};

struct AutoExceptionSlot
{
  struct Pc onerror;
  int resumeable;
};

union AutoSlot
{
  struct AutoFrameSlot retFrame;
  struct AutoExceptionSlot retException;
  struct Var var;
};

#include "bas_token.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct Auto *Auto_new(struct Auto *this);
void Auto_destroy(struct Auto *this);
struct Var *Auto_pushArg(struct Auto *this);
void Auto_pushFuncRet(struct Auto *this, int firstarg, struct Pc *pc);
void Auto_pushGosubRet(struct Auto *this, struct Pc *pc);
struct Var *Auto_local(struct Auto *this, int l);
int Auto_funcReturn(struct Auto *this, struct Pc *pc);
int Auto_gosubReturn(struct Auto *this, struct Pc *pc);
void Auto_frameToError(struct Auto *this, struct Program *program, struct Value *v);
void Auto_setError(struct Auto *this, long int line, struct Pc *pc, struct Value *v);

int Auto_find(struct Auto *this, struct Identifier *ident);
int Auto_variable(struct Auto *this, const struct Identifier *ident);
enum ValueType Auto_argType(const struct Auto *this, int l);
enum ValueType Auto_varType(const struct Auto *this, struct Symbol *sym);
void Auto_funcEnd(struct Auto *this);

#endif /* __APPS_EXAMPLES_BAS_BAS_AUTO_H */
