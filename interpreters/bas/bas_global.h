/****************************************************************************
 * apps/interpreters/bas/bas_global.h
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

#ifndef __APPS_EXAMPLES_BAS_BAS_GLOBAL_H
#define __APPS_EXAMPLES_BAS_BAS_GLOBAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "bas_token.h"
#include "bas_value.h"
#include "bas_var.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GLOBAL_HASHSIZE 31

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct GlobalFunctionChain
{
  struct Pc begin,end;
  struct GlobalFunctionChain *next;
};

struct Global
{
  struct String command;
  struct Symbol *table[GLOBAL_HASHSIZE];
  struct GlobalFunctionChain *chain;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct Global *Global_new(struct Global *self);
void Global_destroy(struct Global *self);
void Global_clear(struct Global *self);
void Global_clearFunctions(struct Global *self);
int Global_find(struct Global *self, struct Identifier *ident, int oparen);
int Global_function(struct Global *self, struct Identifier *ident,
                    enum ValueType type, struct Pc *deffn, struct Pc *begin,
                    int argTypesLength, enum ValueType *argTypes);
void Global_endfunction(struct Global *self, struct Identifier *ident,
                        struct Pc *end);
int Global_variable(struct Global *self, struct Identifier *ident,
                    enum ValueType type, enum SymbolType symbolType,
                    int redeclare);

#endif /* __APPS_EXAMPLES_BAS_BAS_GLOBAL_H */
