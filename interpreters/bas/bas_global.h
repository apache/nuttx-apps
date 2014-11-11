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

struct Global *Global_new(struct Global *this);
void Global_destroy(struct Global *this);
void Global_clear(struct Global *this);
void Global_clearFunctions(struct Global *this);
int Global_find(struct Global *this, struct Identifier *ident, int oparen);
int Global_function(struct Global *this, struct Identifier *ident,
                    enum ValueType type, struct Pc *deffn, struct Pc *begin,
                    int argTypesLength, enum ValueType *argTypes);
void Global_endfunction(struct Global *this, struct Identifier *ident,
                        struct Pc *end);
int Global_variable(struct Global *this, struct Identifier *ident,
                    enum ValueType type, enum SymbolType symbolType,
                    int redeclare);

#endif /* __APPS_EXAMPLES_BAS_BAS_GLOBAL_H */
