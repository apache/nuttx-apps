/****************************************************************************
 * apps/interpreters/bas/bas_var.h
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

#ifndef __APPS_EXAMPLES_BAS_BAS_VAR_H
#define __APPS_EXAMPLES_BAS_BAS_VAR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "bas_value.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VAR_SCALAR_VALUE(this) ((this)->value)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct Var
{
  unsigned int dim;
  unsigned int *geometry;
  struct Value *value;
  unsigned int size;
  enum ValueType type;
  char base;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct Var *Var_new(struct Var *this, enum ValueType type, unsigned int dim,
                    const unsigned int *geometry, int base);
struct Var *Var_new_scalar(struct Var *this);
void Var_destroy(struct Var *this);
void Var_retype(struct Var *this, enum ValueType type);
struct Value *Var_value(struct Var *this, unsigned int dim, int idx[],
                        struct Value *value);
void Var_clear(struct Var *this);
struct Value *Var_mat_assign(struct Var *this, struct Var *x,
                             struct Value *err, int work);
struct Value *Var_mat_addsub(struct Var *this, struct Var *x, struct Var *y,
                             int add, struct Value *err, int work);
struct Value *Var_mat_mult(struct Var *this, struct Var *x, struct Var *y,
                           struct Value *err, int work);
struct Value *Var_mat_scalarMult(struct Var *this, struct Value *factor,
                                 struct Var *x, int work);
void Var_mat_transpose(struct Var *this, struct Var *x);
struct Value *Var_mat_invert(struct Var *this, struct Var *x,
                             struct Value *det, struct Value *err);
struct Value *Var_mat_redim(struct Var *this, unsigned int dim,
                            const unsigned int *geometry,
                            struct Value *err);

#endif /* __APPS_EXAMPLES_BAS_BAS_VAR_H */
