/****************************************************************************
 * apps/interpreters/bas/bas_var.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "bas_error.h"
#include "bas_var.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define _(String) String

/****************************************************************************
 * Public Functions
 ****************************************************************************/

struct Var *Var_new(struct Var *this, enum ValueType type, unsigned int dim,
                    const unsigned int *geometry, int base)
{
  unsigned int i;
  size_t newsize;

  this->type = type;
  this->dim = dim;
  this->base = base;
  for (newsize = this->size = 1, dim = 0; dim < this->dim; ++dim)
    {
      if ((newsize *= geometry[dim]) < this->size)
        return (struct Var *)0;
      this->size = newsize;
    }

  if ((newsize *= sizeof(struct Value)) < this->size)
    {
      return (struct Var *)0;
    }

  if ((this->value = malloc(newsize)) == (struct Value *)0)
    {
      return (struct Var *)0;
    }

  if (dim)
    {
      this->geometry = malloc(sizeof(unsigned int) * dim);
      for (i = 0; i < dim; ++i)
        {
          this->geometry[i] = geometry[i];
        }
    }
  else
    {
      this->geometry = (unsigned int *)0;
    }

  for (i = 0; i < this->size; ++i)
    {
      Value_new_null(&(this->value[i]), type);
    }

  return this;
}

struct Var *Var_new_scalar(struct Var *this)
{
  this->dim = 0;
  this->size = 1;
  this->geometry = (unsigned int *)0;
  this->value = malloc(sizeof(struct Value));
  return this;
}

void Var_destroy(struct Var *this)
{
  while (this->size--)
    {
      Value_destroy(&(this->value[this->size]));
    }

  free(this->value);
  this->value = (struct Value *)0;
  this->size = 0;
  this->dim = 0;
  if (this->geometry)
    {
      free(this->geometry);
      this->geometry = (unsigned int *)0;
    }
}

void Var_retype(struct Var *this, enum ValueType type)
{
  unsigned int i;

  for (i = 0; i < this->size; ++i)
    {
      Value_destroy(&(this->value[i]));
      Value_new_null(&(this->value[i]), type);
    }
}

struct Value *Var_value(struct Var *this, unsigned int dim, int idx[],
                        struct Value *value)
{
  unsigned int offset;
  unsigned int i;

  assert(this->value);
  if (dim != this->dim)
    {
      return Value_new_ERROR(value, DIMENSION);
    }

  for (offset = 0, i = 0; i < dim; ++i)
    {
      if (idx[i] < this->base || (idx[i] - this->base) >= this->geometry[i])
        {
          return Value_new_ERROR(value, OUTOFRANGE, _("array index"));
        }

      offset = offset * this->geometry[i] + (idx[i] - this->base);
    }

  assert(offset < this->size);
  return this->value + offset;
}

void Var_clear(struct Var *this)
{
  size_t i;

  for (i = 0; i < this->size; ++i)
    {
      Value_destroy(&(this->value[i]));
    }

  if (this->geometry)
    {
      free(this->geometry);
      this->geometry = (unsigned int *)0;
      this->size = 1;
      this->dim = 0;
    }

  Value_new_null(&(this->value[0]), this->type);
}

struct Value *Var_mat_assign(struct Var *this, struct Var *x, struct Value *err,
                             int work)
{
  enum ValueType thisType = this->type;

  if (work)
    {
      unsigned int i, j;
      int unused = 1 - x->base;
      int g0, g1;

      assert(x->base == 0 || x->base == 1);
      assert(x->dim == 1 || x->dim == 2);
      if (this == x)
        {
          return (struct Value *)0;
        }

      Var_destroy(this);
      Var_new(this, thisType, x->dim, x->geometry, x->base);
      g0 = x->geometry[0];
      g1 = x->dim == 1 ? unused + 1 : x->geometry[1];
      for (i = unused; i < g0; ++i)
        {
          for (j = unused; j < g1; ++j)
            {
              unsigned int element = x->dim == 1 ? i : i * g1 + j;

              Value_destroy(&(this->value[element]));
              Value_clone(&(this->value[element]), &(x->value[element]));
              Value_retype(&(this->value[element]), thisType);
            }
        }
    }
  else
    {
      if (Value_commonType[this->type][x->type] == V_ERROR)
        {
          return Value_new_typeError(err, this->type, x->type);
        }
    }

  return (struct Value *)0;
}

struct Value *Var_mat_addsub(struct Var *this, struct Var *x, struct Var *y,
                             int add, struct Value *err, int work)
{
  enum ValueType thisType = this->type;
  struct Value foo, bar;

  if (work)
    {
      unsigned int i, j;
      int unused = 1 - x->base;
      int g0, g1;

      assert(x->base == 0 || x->base == 1);
      assert(x->dim == 1 || x->dim == 2);
      if (x->base != y->base || x->dim != y->dim ||
          x->geometry[0] != y->geometry[0] ||
          (x->dim == 2 && x->geometry[1] != y->geometry[1]))
        {
          return Value_new_ERROR(err, DIMENSION);
        }

      if (this != x && this != y)
        {
          Var_destroy(this);
          Var_new(this, thisType, x->dim, x->geometry, x->base);
        }

      g0 = x->geometry[0];
      g1 = x->dim == 1 ? unused + 1 : x->geometry[1];
      for (i = unused; i < g0; ++i)
        {
          for (j = unused; j < g1; ++j)
            {
              unsigned int element = x->dim == 1 ? i : i * g1 + j;

              Value_clone(&foo, &(x->value[element]));
              Value_clone(&bar, &(y->value[element]));
              if (add)
                {
                  Value_add(&foo, &bar, 1);
                }
              else
                {
                  Value_sub(&foo, &bar, 1);
                }

              if (foo.type == V_ERROR)
                {
                  *err = foo;
                  Value_destroy(&bar);
                  return err;
                }

              Value_destroy(&bar);
              Value_destroy(&(this->value[element]));
              this->value[element] = *Value_retype(&foo, thisType);
            }
        }
    }
  else
    {
      Value_clone(err, x->value);
      if (add)
        {
          Value_add(err, y->value, 0);
        }
      else
        {
          Value_sub(err, y->value, 0);
        }

      if (err->type == V_ERROR)
        {
          return err;
        }

      Value_destroy(err);
    }

  return (struct Value *)0;
}

struct Value *Var_mat_mult(struct Var *this, struct Var *x, struct Var *y,
                           struct Value *err, int work)
{
  enum ValueType thisType = this->type;
  struct Var foo;

  if (work)
    {
      unsigned int newdim[2];
      unsigned int i, j, k;
      int unused = 1 - x->base;

      assert(x->base == 0 || x->base == 1);
      if (x->dim != 2 || y->dim != 2 || x->base != y->base ||
          x->geometry[1] != y->geometry[0])
        {
          return Value_new_ERROR(err, DIMENSION);
        }

      newdim[0] = x->geometry[0];
      newdim[1] = y->geometry[1];
      Var_new(&foo, thisType, 2, newdim, 0);
      for (i = unused; i < newdim[0]; ++i)
        {
          for (j = unused; j < newdim[1]; ++j)
            {
              struct Value *dp = &foo.value[i * newdim[1] + j];

              Value_new_null(dp, thisType);
              for (k = unused; k < x->geometry[1]; ++k)
                {
                  struct Value p;

                  Value_clone(&p, &(x->value[i * x->geometry[1] + k]));
                  Value_mult(&p, &(y->value[k * y->geometry[1] + j]), 1);
                  if (p.type == V_ERROR)
                    {
                      *err = p;
                      Var_destroy(&foo);
                      return err;
                    }

                  Value_add(dp, &p, 1);
                  Value_destroy(&p);
                }

              Value_retype(dp, thisType);
            }
        }

      Var_destroy(this);
      *this = foo;
    }
  else
    {
      Value_clone(err, x->value);
      Value_mult(err, y->value, 0);
      if (err->type == V_ERROR)
        {
          return err;
        }

      Value_destroy(err);
    }

  return (struct Value *)0;
}

struct Value *Var_mat_scalarMult(struct Var *this, struct Value *factor,
                                 struct Var *x, int work)
{
  enum ValueType thisType = this->type;

  if (work)
    {
      unsigned int i, j;
      int unused = 1 - x->base;
      int g0, g1;

      assert(x->base == 0 || x->base == 1);
      assert(x->dim == 1 || x->dim == 2);
      if (this != x)
        {
          Var_destroy(this);
          Var_new(this, thisType, x->dim, x->geometry, 0);
        }

      g0 = x->geometry[0];
      g1 = x->dim == 1 ? unused + 1 : x->geometry[1];
      for (i = unused; i < g0; ++i)
        {
          for (j = unused; j < g1; ++j)
            {
              unsigned int element = x->dim == 1 ? i : i * g1 + j;
              struct Value foo;

              Value_clone(&foo, &(x->value[element]));
              Value_mult(&foo, factor, 1);
              if (foo.type == V_ERROR)
                {
                  Value_destroy(factor);
                  *factor = foo;
                  return factor;
                }

              Value_destroy(&(this->value[element]));
              this->value[element] = *Value_retype(&foo, thisType);
            }
        }
    }
  else
    {
      if (Value_mult(factor, this->value, 0)->type == V_ERROR)
        {
          return factor;
        }
    }

  return (struct Value *)0;
}

void Var_mat_transpose(struct Var *this, struct Var *x)
{
  unsigned int geometry[2];
  enum ValueType thisType = this->type;
  unsigned int i, j;
  struct Var foo;

  geometry[0] = x->geometry[1];
  geometry[1] = x->geometry[0];
  Var_new(&foo, thisType, 2, geometry, 0);
  for (i = 0; i < x->geometry[0]; ++i)
    {
      for (j = 0; j < x->geometry[1]; ++j)
        {
          Value_destroy(&foo.value[j * x->geometry[0] + i]);
          Value_clone(&foo.value[j * x->geometry[0] + i],
                      &(x->value[i * x->geometry[1] + j]));
          Value_retype(&foo.value[j * x->geometry[0] + i], thisType);
        }
    }

  Var_destroy(this);
  *this = foo;
}

struct Value *Var_mat_invert(struct Var *this, struct Var *x, struct Value *det,
                             struct Value *err)
{
  enum ValueType thisType = this->type;
  int n, i, j, k, max;
  double t, *a, *u, d;
  int unused = 1 - x->base;

  if (x->type != V_INTEGER && x->type != V_REAL)
    {
      return Value_new_ERROR(err, TYPEMISMATCH5);
    }

  assert(x->base == 0 || x->base == 1);
  if (x->geometry[0] != x->geometry[1])
    {
      return Value_new_ERROR(err, DIMENSION);
    }

  n = x->geometry[0] - unused;

  a = malloc(sizeof(double) * n * n);
  u = malloc(sizeof(double) * n * n);
  for (i = 0; i < n; ++i)
    {
      for (j = 0; j < n; ++j)
        {
          if (x->type == V_INTEGER)
            {
              a[i * n + j] =
                x->value[(i + unused) * (n + unused) + j + unused].u.integer;
            }
          else
            {
              a[i * n + j] =
                x->value[(i + unused) * (n + unused) + j + unused].u.real;
            }

          u[i * n + j] = (i == j ? 1.0 : 0.0);
        }
    }

  d = 1.0;

  for (i = 0; i < n; ++i)       /* get zeroes in column i below the main
                                 * diagonal */
    {
      max = i;
      for (j = i + 1; j < n; ++j)
        {
          if (fabs(a[j * n + i]) > fabs(a[max * n + i]))
            {
              max = j;
            }
        }

      /* exchanging row i against row max */

      if (i != max)
        {
          d = -d;
        }

      for (k = i; k < n; ++k)
        {
          t = a[i * n + k];
          a[i * n + k] = a[max * n + k];
          a[max * n + k] = t;
        }

      for (k = 0; k < n; ++k)
        {
          t = u[i * n + k];
          u[i * n + k] = u[max * n + k];
          u[max * n + k] = t;
        }

      if (a[i * n + i] == 0.0)
        {
          free(a);
          free(u);
          return Value_new_ERROR(err, SINGULAR);
        }

      for (j = i + 1; j < n; ++j)
        {
          t = a[j * n + i] / a[i * n + i];

          /* Subtract row i*t from row j */

          for (k = i; k < n; ++k)
            {
              a[j * n + k] -= a[i * n + k] * t;
            }

          for (k = 0; k < n; ++k)
            {
              u[j * n + k] -= u[i * n + k] * t;
            }
        }
    }

  for (i = 0; i < n; ++i)
    {
      d *= a[i * n + i];        /* compute determinant */
    }

  for (i = n - 1; i >= 0; --i)  /* get zeroes in column i above the main diagonal */
    {
      for (j = 0; j < i; ++j)
        {
          t = a[j * n + i] / a[i * n + i];

          /* Subtract row i*t from row j */

          a[j * n + i] = 0.0;   /* a[j*n+i]-=a[i*n+i]*t; */
          for (k = 0; k < n; ++k)
            {
              u[j * n + k] -= u[i * n + k] * t;
            }
        }

      t = a[i * n + i];
      a[i * n + i] = 1.0;       /* a[i*n+i]/=t; */
      for (k = 0; k < n; ++k)
        {
          u[i * n + k] /= t;
        }
    }

  free(a);
  if (this != x)
    {
      Var_destroy(this);
      Var_new(this, thisType, 2, x->geometry, x->base);
    }

  for (i = 0; i < n; ++i)
    {
      for (j = 0; j < n; ++j)
        {
          Value_destroy(&this->value[(i + unused) * (n + unused) + j + unused]);
          if (thisType == V_INTEGER)
            {
              Value_new_INTEGER(&this->value
                                [(i + unused) * (n + unused) + j + unused],
                                u[i * n + j]);
            }
          else
            {
              Value_new_REAL(&this->
                             value[(i + unused) * (n + unused) + j + unused],
                             u[i * n + j]);
            }
        }
    }

  free(u);
  Value_destroy(det);
  if (thisType == V_INTEGER)
    {
      Value_new_INTEGER(det, d);
    }
  else
    {
      Value_new_REAL(det, d);
    }

  return (struct Value *)0;
}

struct Value *Var_mat_redim(struct Var *this, unsigned int dim,
                            const unsigned int *geometry, struct Value *err)
{
  unsigned int i, j, size;
  struct Value *value;
  int unused = 1 - this->base;
  int g0, g1;

  if (this->dim > 0 && this->dim != dim)
    {
      return Value_new_ERROR(err, DIMENSION);
    }

  for (size = 1, i = 0; i < dim; ++i)
    {
      size *= geometry[i];
    }

  value = malloc(sizeof(struct Value) * size);
  g0 = geometry[0];
  g1 = dim == 1 ? 1 : geometry[1];
  for (i = 0; i < g0; ++i)
    {
      for (j = 0; j < g1; ++j)
        {
          if (this->dim == 0 || i < unused || (dim == 2 && j < unused) ||
              i >= this->geometry[0] || (this->dim == 2 &&
                                         j >= this->geometry[1]))
            {
              Value_new_null(&(value[i * g1 + j]), this->type);
            }
          else
            {
              Value_clone(&value[dim == 1 ? i : i * g1 + j],
                          &this->value[dim ==
                                       1 ? i : i * this->geometry[1] + j]);
            }
        }
    }

  for (i = 0; i < this->size; ++i)
    {
      Value_destroy(&this->value[i]);
    }

  free(this->value);
  if (this->geometry == (unsigned int *)0)
    {
      this->geometry = malloc(sizeof(unsigned int) * dim);
    }

  for (i = 0; i < dim; ++i)
    {
      this->geometry[i] = geometry[i];
    }

  this->dim = dim;
  this->size = size;
  this->value = value;
  return (struct Value *)0;
}
