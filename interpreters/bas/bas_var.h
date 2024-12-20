/****************************************************************************
 * apps/interpreters/bas/bas_var.h
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
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

#define VAR_SCALAR_VALUE(self) ((self)->value)

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

struct Var *Var_new(struct Var *self, enum ValueType type, unsigned int dim,
                    const unsigned int *geometry, int base);
struct Var *Var_new_scalar(struct Var *self);
void Var_destroy(struct Var *self);
void Var_retype(struct Var *self, enum ValueType type);
struct Value *Var_value(struct Var *self, unsigned int dim, int idx[],
                        struct Value *value);
void Var_clear(struct Var *self);
struct Value *Var_mat_assign(struct Var *self, struct Var *x,
                             struct Value *err, int work);
struct Value *Var_mat_addsub(struct Var *self, struct Var *x, struct Var *y,
                             int add, struct Value *err, int work);
struct Value *Var_mat_mult(struct Var *self, struct Var *x, struct Var *y,
                           struct Value *err, int work);
struct Value *Var_mat_scalarMult(struct Var *self, struct Value *factor,
                                 struct Var *x, int work);
void Var_mat_transpose(struct Var *self, struct Var *x);
struct Value *Var_mat_invert(struct Var *self, struct Var *x,
                             struct Value *det, struct Value *err);
struct Value *Var_mat_redim(struct Var *self, unsigned int dim,
                            const unsigned int *geometry,
                            struct Value *err);

#endif /* __APPS_EXAMPLES_BAS_BAS_VAR_H */
