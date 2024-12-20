/****************************************************************************
 * apps/interpreters/bas/bas_value.c
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
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bas_error.h"
#include "bas_value.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define _(String) String

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *typestr[] =
{
  (const char *)0,
  (const char *)0,
  "integer",
  (const char *)0,
  "real",
  "string",
  "void"
};

/* for xgettext */

const enum ValueType Value_commonType[V_VOID + 1][V_VOID + 1] =
{
  { 0, 0,       0,         0,       0,       0,        0       },
  { 0, V_ERROR, V_ERROR,   V_ERROR, V_ERROR, V_ERROR,  V_ERROR },
  { 0, V_ERROR, V_INTEGER, V_ERROR, V_REAL,  V_ERROR,  V_ERROR },
  { 0, V_ERROR, V_ERROR,   V_ERROR, V_ERROR, V_ERROR,  V_ERROR },
  { 0, V_ERROR, V_REAL,    V_ERROR, V_REAL,  V_ERROR,  V_ERROR },
  { 0, V_ERROR, V_ERROR,   V_ERROR, V_ERROR, V_STRING, V_ERROR },
  { 0, V_ERROR, V_ERROR,   V_ERROR, V_ERROR, V_ERROR,  V_ERROR }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void format_double(struct String *buf, double value, int width,
                          int precision, int exponent)
{
  if (exponent)
    {
      size_t len;
      char *e;
      int en;

      len = buf->length;
      String_appendPrintf(buf, "%.*E", width - 1 - (precision >= 0), value);
      if (buf->character[len + 1] == '.')
        {
          String_delete(buf, len + 1, 1);
        }

      if (precision >= 0)
        {
          String_insertChar(buf, len + width - precision - 1, '.');
        }

      for (e = buf->character + buf->length - 1;
           e >= buf->character && *e != 'E';
           --e);
      ++e;

      en = strtol(e, (char **)0, 10);
      en = en + 2 - (width - precision);
      len = e - buf->character;
      String_delete(buf, len, buf->length - len);
      String_appendPrintf(buf, "%+0*d", exponent - 1, en);
    }
  else if (precision > 0)
    {
      String_appendPrintf(buf, "%.*f", precision, value);
    }
  else if (precision == 0)
    {
      String_appendPrintf(buf, "%.f.", value);
    }
  else if (width)
    {
      String_appendPrintf(buf, "%.f", value);
    }
  else
    {
      double x = value;

      if (x < 0.0001 || x >= 10000000.0)        /* print scientific notation */
        {
          String_appendPrintf(buf, "%.7g", value);
        }
      else                      /* print decimal numbers or integers, if
                                 * possible */
        {
          int o;
          int n;
          int p = 6;

          while (x >= 10.0 && p > 0)
            {
              x /= 10.0;
              --p;
            }

          o = buf->length;
          String_appendPrintf(buf, "%.*f", p, value);
          n = buf->length;
          if (memchr(buf->character + o, '.', n - o))
            {
              while (buf->character[buf->length - 1] == '0')
                {
                  --buf->length;
                }

              if (buf->character[buf->length - 1] == '.')
                {
                  --buf->length;
                }
            }
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

double Value_trunc(double d)
{
  return (d < 0.0 ? ceil(d) : floor(d));
}

double Value_round(double d)
{
  return (d < 0.0 ? ceil(d - 0.5) : floor(d + 0.5));
}

long int Value_toi(double d, int *overflow)
{
  d = Value_round(d);
  *overflow = (d < LONG_MIN || d > LONG_MAX);
  return lrint(d);
}

long int Value_vali(const char *s, char **end, int *overflow)
{
  long int n;

  errno = 0;
  if (*s == '&' && tolower(*(s + 1)) == 'h')
    {
      n = strtoul(s + 2, end, 16);
    }
  else if (*s == '&' && tolower(*(s + 1)) == 'o')
    {
      n = strtoul(s + 2, end, 8);
    }
  else
    {
      n = strtol(s, end, 10);
    }

  *overflow = (errno == ERANGE);
  return n;
}

double Value_vald(const char *s, char **end, int *overflow)
{
  double d;

  errno = 0;
  d = strtod(s, end);
  *overflow = (errno == ERANGE);
  return d;
}

struct Value *Value_new_NIL(struct Value *self)
{
  assert(self != (struct Value *)0);
  self->type = V_NIL;
  return self;
}

struct Value *Value_new_ERROR(struct Value *self, int code,
                              const char *error, ...)
{
  va_list ap;
  char buf[128];

  assert(self != (struct Value *)0);
  va_start(ap, error);
  vsprintf(buf, error, ap);
  va_end(ap);
  self->type = V_ERROR;
  self->u.error.code = code;
  self->u.error.msg  = strdup(buf);
  return self;
}

struct Value *Value_new_INTEGER(struct Value *self, int n)
{
  assert(self != (struct Value *)0);
  self->type = V_INTEGER;
  self->u.integer = n;
  return self;
}

struct Value *Value_new_REAL(struct Value *self, double n)
{
  assert(self != (struct Value *)0);
  self->type = V_REAL;
  self->u.real = n;
  return self;
}

struct Value *Value_new_STRING(struct Value *self)
{
  assert(self != (struct Value *)0);
  self->type = V_STRING;
  String_new(&self->u.string);
  return self;
}

struct Value *Value_new_VOID(struct Value *self)
{
  assert(self != (struct Value *)0);
  self->type = V_VOID;
  return self;
}

struct Value *Value_new_null(struct Value *self, enum ValueType type)
{
  assert(self != (struct Value *)0);
  switch (type)
    {
    case V_INTEGER:
      {
        self->type = V_INTEGER;
        self->u.integer = 0;
        break;
      }

    case V_REAL:
      {
        self->type = V_REAL;
        self->u.real = 0.0;
        break;
      }

    case V_STRING:
      {
        self->type = V_STRING;
        String_new(&self->u.string);
        break;
      }

    case V_VOID:
      {
        self->type = V_VOID;
        break;
      }

    default:
      assert(0);
    }

  return self;
}

int Value_isNull(const struct Value *self)
{
  switch (self->type)
    {
    case V_INTEGER:
      return (self->u.integer == 0);

    case V_REAL:
      return (self->u.real == 0.0);

    case V_STRING:
      return (self->u.string.length == 0);

    default:
      assert(0);
    }

  return -1;
}

void Value_destroy(struct Value *self)
{
  assert(self != (struct Value *)0);
  switch (self->type)
    {
    case V_ERROR:
      free(self->u.error.msg);
      break;

    case V_INTEGER:
      break;

    case V_NIL:
      break;

    case V_REAL:
      break;

    case V_STRING:
      String_destroy(&self->u.string);
      break;

    case V_VOID:
      break;

    default:
      assert(0);
    }

  self->type = 0;
}

struct Value *Value_clone(struct Value *self, const struct Value *original)
{
  assert(self != (struct Value *)0);
  assert(original != (struct Value *)0);

  switch (original->type)
    {
    case V_ERROR:
      {
        self->u.error.msg = strdup(original->u.error.msg);
        self->u.error.code = original->u.error.code;
        break;
      }

    case V_INTEGER:
      self->u.integer = original->u.integer;
      break;

    case V_NIL:
      break;

    case V_REAL:
      self->u.real = original->u.real;
      break;

    case V_STRING:
      String_clone(&self->u.string, &original->u.string);
      break;

    default:
      assert(0);
    }

  self->type = original->type;
  return self;
}

struct Value *Value_uplus(struct Value *self, int calc)
{
  switch (self->type)
    {
    case V_INTEGER:
    case V_REAL:
      {
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDUOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_uneg(struct Value *self, int calc)
{
  switch (self->type)
    {
    case V_INTEGER:
      {
        if (calc)
          {
            self->u.integer = -self->u.integer;
          }
        break;
      }

    case V_REAL:
      {
        if (calc)
          {
            self->u.real = -self->u.real;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDUOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_unot(struct Value *self, int calc)
{
  switch (self->type)
    {
    case V_INTEGER:
      {
        if (calc)
          {
            self->u.integer = ~self->u.integer;
          }
        break;
      }

    case V_REAL:
      {
        Value_retype(self, V_INTEGER);
        if (calc)
          {
            self->u.integer = ~self->u.integer;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDUOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_add(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer += x->u.integer;
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            self->u.real += x->u.real;
          }
        break;
      }

    case V_STRING:
      {
        if (calc)
          {
            String_appendString(&self->u.string, &x->u.string);
          }
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_sub(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer -= x->u.integer;
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            self->u.real -= x->u.real;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_mult(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer *= x->u.integer;
          }

        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            self->u.real *= x->u.real;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_div(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            if (x->u.real == 0)
              {
                Value_destroy(self);
                Value_new_ERROR(self, UNDEFINED, "Division by zero");
              }
            else
              {
                self->u.real /= x->u.real;
              }
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            if (x->u.real == 0.0)
              {
                Value_destroy(self);
                Value_new_ERROR(self, UNDEFINED, "Division by zero");
              }
            else
              {
                self->u.real /= x->u.real;
              }
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_idiv(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            if (x->u.integer == 0)
              {
                Value_destroy(self);
                Value_new_ERROR(self, UNDEFINED, "Division by zero");
              }
            else
              {
                self->u.integer /= x->u.integer;
              }
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            if (x->u.real == 0.0)
              {
                Value_destroy(self);
                Value_new_ERROR(self, UNDEFINED, "Division by zero");
              }
            else
              {
                self->u.real = Value_trunc(self->u.real / x->u.real);
              }
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_mod(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            if (x->u.integer == 0)
              {
                Value_destroy(self);
                Value_new_ERROR(self, UNDEFINED, "Modulo by zero");
              }
            else
              {
                self->u.integer %= x->u.integer;
              }
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            if (x->u.real == 0.0)
              {
                Value_destroy(self);
                Value_new_ERROR(self, UNDEFINED, "Modulo by zero");
              }
            else
              {
                self->u.real = fmod(self->u.real, x->u.real);
              }
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_pow(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            if (self->u.integer == 0 && x->u.integer == 0)
              {
                Value_destroy(self);
                Value_new_ERROR(self, UNDEFINED, "0^0");
              }
            else if (x->u.integer > 0)
              {
                self->u.integer = pow(self->u.integer, x->u.integer);
              }
            else
              {
                long int thisi = self->u.integer;
                Value_destroy(self);
                Value_new_REAL(self, pow(thisi, x->u.integer));
              }
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            if (self->u.real == 0.0 && x->u.real == 0.0)
              {
                Value_destroy(self);
                Value_new_ERROR(self, UNDEFINED, "0^0");
              }
            else
              {
                self->u.real = pow(self->u.real, x->u.real);
              }
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_and(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
    case V_REAL:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer &= x->u.integer;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_or(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
    case V_REAL:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer |= x->u.integer;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_xor(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
    case V_REAL:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer ^= x->u.integer;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_eqv(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
    case V_REAL:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer = ~(self->u.integer ^ x->u.integer);
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_imp(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
    case V_REAL:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer = (~self->u.integer) | x->u.integer;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(self);
        Value_new_ERROR(self, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_lt(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer = (self->u.integer < x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (self->u.real < x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = (String_cmp(&self->u.string, &x->u.string) < 0) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_le(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer = (self->u.integer <= x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (self->u.real <= x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = (String_cmp(&self->u.string, &x->u.string) <= 0) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_eq(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer = (self->u.integer == x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (self->u.real == x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = (String_cmp(&self->u.string, &x->u.string) == 0) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_ge(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer = (self->u.integer >= x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (self->u.real >= x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = (String_cmp(&self->u.string, &x->u.string) >= 0) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_gt(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer = (self->u.integer > x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (self->u.real > x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = (String_cmp(&self->u.string, &x->u.string) > 0) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

struct Value *Value_ne(struct Value *self, struct Value *x, int calc)
{
  switch (Value_commonType[self->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(self, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            self->u.integer = (self->u.integer != x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(self, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (self->u.real != x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = String_cmp(&self->u.string, &x->u.string) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(self);
        Value_new_INTEGER(self, v);
        break;
      }

    default:
      assert(0);
    }

  return self;
}

int Value_exitFor(struct Value *self,
                  struct Value *limit, struct Value *step)
{
  switch (self->type)
    {
    case V_INTEGER:
      return
        (step->u.integer < 0
         ? (self->u.integer < limit->u.integer)
         : (self->u.integer > limit->u.integer));

    case V_REAL:
      return
        (step->u.real < 0.0
         ? (self->u.real < limit->u.real) : (self->u.real > limit->u.real));

    case V_STRING:
      return (String_cmp(&self->u.string, &limit->u.string) > 0);

    default:
      assert(0);
    }

  return -1;
}

void Value_errorPrefix(struct Value *self, const char *prefix)
{
  size_t prefixlen;
  size_t msglen;

  assert(self->type == V_ERROR);
  prefixlen = strlen(prefix);
  msglen = strlen(self->u.error.msg);
  self->u.error.msg = realloc(self->u.error.msg, prefixlen + msglen + 1);
  memmove(self->u.error.msg + prefixlen, self->u.error.msg, msglen);
  memcpy(self->u.error.msg, prefix, prefixlen);
}

void Value_errorSuffix(struct Value *self, const char *suffix)
{
  size_t suffixlen;
  size_t msglen;

  assert(self->type == V_ERROR);
  suffixlen = strlen(suffix);
  msglen = strlen(self->u.error.msg);
  self->u.error.msg = realloc(self->u.error.msg, suffixlen + msglen + 1);
  memcpy(self->u.error.msg + msglen, suffix, suffixlen + 1);
}

struct Value *Value_new_typeError(struct Value *self, enum ValueType t1,
                                  enum ValueType t2)
{
  assert(typestr[t1]);
  assert(typestr[t2]);
  return Value_new_ERROR(self, TYPEMISMATCH1,
                         _(typestr[t1]), _(typestr[t2]));
}

static void retypeError(struct Value *self, enum ValueType to)
{
  enum ValueType thisType = self->type;

  assert(typestr[thisType]);
  assert(typestr[to]);
  Value_destroy(self);
  Value_new_ERROR(self, TYPEMISMATCH1, _(typestr[thisType]), _(typestr[to]));
}

struct Value *Value_retype(struct Value *self, enum ValueType type)
{
  switch (self->type)
    {
    case V_INTEGER:
      {
        switch (type)
          {
          case V_INTEGER:
            break;

          case V_REAL:
            self->u.real = self->u.integer;
            self->type = type;
            break;

          case V_VOID:
            Value_destroy(self);
            Value_new_VOID(self);
            break;

          default:
            retypeError(self, type);
            break;
          }
        break;
      }

    case V_REAL:
      {
        int overflow;

        switch (type)
          {
          case V_INTEGER:
            {
              self->u.integer = Value_toi(self->u.real, &overflow);
              self->type = V_INTEGER;
              if (overflow)
                {
                  Value_destroy(self);
                  Value_new_ERROR(self, OUTOFRANGE, typestr[V_INTEGER]);
                }
              break;
            }

          case V_REAL:
            break;

          case V_VOID:
            Value_destroy(self);
            Value_new_VOID(self);
            break;

          default:
            retypeError(self, type);
            break;
          }
        break;
      }

    case V_STRING:
      {
        switch (type)
          {
          case V_STRING:
            break;

          case V_VOID:
            Value_destroy(self);
            Value_new_VOID(self);
            break;

          default:
            retypeError(self, type);
            break;
          }
        break;
      }

    case V_VOID:
      {
        switch (type)
          {
          case V_VOID:
            break;

          default:
            retypeError(self, type);
          }
        break;
      }

    case V_ERROR:
      break;

    default:
      assert(0);
    }

  return self;
}

struct String *Value_toString(struct Value *self, struct String *s, char pad,
                              int headingsign, size_t width, int commas,
                              int dollar, int dollarleft, int precision,
                              int exponent, int trailingsign)
{
  size_t oldlength = s->length;

  switch (self->type)
    {
    case V_ERROR:
      String_appendChars(s, self->u.error.msg);
      break;

    case V_REAL:
    case V_INTEGER:
      {
        int sign;
        struct String buf;
        size_t totalwidth = width;

        String_new(&buf);
        if (self->type == V_INTEGER)
          {
            if (self->u.integer < 0)
              {
                sign = -1;
                self->u.integer = -self->u.integer;
              }
            else if (self->u.integer == 0)
              {
                sign = 0;
              }
            else
              {
                sign = 1;
              }
          }
        else
          {
            if (self->u.real < 0.0)
              {
                sign = -1;
                self->u.real = -self->u.real;
              }
            else if (self->u.real == 0.0)
              {
                sign = 0;
              }
            else
              {
                sign = 1;
              }
          }

        switch (headingsign)
          {
          case -1:
            {
              ++totalwidth;
              String_appendChar(&buf, sign == -1 ? '-' : ' ');
              break;
            }

          case 0:
            {
              if (sign == -1)
                {
                  String_appendChar(&buf, '-');
                }
              break;
            }

          case 1:
            {
              ++totalwidth;
              String_appendChar(&buf, sign == -1 ? '-' : '+');
              break;
            }

          case 2:
            break;

          default:
            assert(0);
          }

        totalwidth += exponent;
        if (self->type == V_INTEGER)
          {
            if (precision > 0 || exponent)
              {
                format_double(&buf, (double)self->u.integer, width,
                              precision, exponent);
              }
            else if (precision == 0)
              {
                String_appendPrintf(&buf, "%lu.", self->u.integer);
              }
            else
              {
                String_appendPrintf(&buf, "%lu", self->u.integer);
              }
          }
        else
          {
            format_double(&buf, self->u.real, width, precision, exponent);
          }

        if (commas)
          {
            size_t digits;
            int first;

            first = (headingsign ? 1 : 0);
            for (digits = first;
                 digits < buf.length && buf.character[digits] >= '0' &&
                 buf.character[digits] <= '9'; ++digits);

            while (digits > first + 3)
              {
                digits -= 3;
                String_insertChar(&buf, digits, ',');
              }
          }

        if (dollar)
          {
            String_insertChar(&buf, 0, '$');
          }

        if (trailingsign == -1)
          {
            ++totalwidth;
            String_appendChar(&buf, sign == -1 ? '-' : ' ');
          }
        else if (trailingsign == 1)
          {
            ++totalwidth;
            String_appendChar(&buf, sign == -1 ? '-' : '+');
          }

        String_size(s,
                    oldlength + (totalwidth >
                                 buf.length ? totalwidth : buf.length));

        if (totalwidth > buf.length)
          {
            memset(s->character + oldlength, pad,
                   totalwidth - buf.length + dollarleft);
          }

        memcpy(s->character + oldlength +
               (totalwidth >
                buf.length ? (totalwidth - buf.length) : 0) + dollarleft,
               buf.character + dollarleft, buf.length - dollarleft);

        if (dollarleft)
          {
            s->character[oldlength] = '$';
          }

        String_destroy(&buf);
        break;
      }

    case V_STRING:
      {
        if (width > 0)
          {
            size_t blanks =
              (self->u.string.length <
               width ? (width - self->u.string.length) : 0);

            String_size(s, oldlength + width);
            memcpy(s->character + oldlength, self->u.string.character,
                   blanks ? self->u.string.length : width);
            if (blanks)
              {
                memset(s->character + oldlength + self->u.string.length, ' ',
                       blanks);
              }
          }
        else
          {
            String_appendString(s, &self->u.string);
          }
        break;
      }

    default:
      assert(0);
      return 0;
    }

  return s;
}

struct Value *Value_toStringUsing(struct Value *self, struct String *s,
                                  struct String *using, size_t * usingpos)
{
  char pad = ' ';
  int headingsign;
  int width = 0;
  int commas = 0;
  int dollar = 0;
  int dollarleft = 0;
  int precision = -1;
  int exponent = 0;
  int trailingsign = 0;

  headingsign = (using->length ? 0 : -1);
  if (*usingpos == using->length)
    {
      *usingpos = 0;
    }

  while (*usingpos < using->length)
    {
      switch (using->character[*usingpos])
        {
        case '_':              /* output next char */
          {
            ++(*usingpos);
            if (*usingpos < using->length)
              {
                String_appendChar(s, using->character[(*usingpos)++]);
              }
            else
              {
                Value_destroy(self);
                return Value_new_ERROR(self, MISSINGCHARACTER);
              }

            break;
          }

        case '!':              /* output first character of string */
          {
            width = 1;
            ++(*usingpos);
            goto work;
          }

        case '\\':             /* output n characters of string */
          {
            width = 1;
            ++(*usingpos);
            while (*usingpos < using->length &&
                   using->character[*usingpos] == ' ')
              {
                ++(*usingpos);
                ++width;
              }

            if (*usingpos < using->length &&
                using->character[*usingpos] == '\\')
              {
                ++(*usingpos);
                ++width;
                goto work;
              }
            else
              {
                Value_destroy(self);
                return Value_new_ERROR(self, IOERROR,
                                       _("unpaired \\ in format"));
              }

            break;
          }

        case '&':              /* output string */
          {
            width = 0;
            ++(*usingpos);
            goto work;
          }

        case '*':
        case '$':
        case '0':
        case '+':
        case '#':
        case '.':
          {
            if (using->character[*usingpos] == '+')
              {
                headingsign = 1;
                ++(*usingpos);
              }

            while (*usingpos < using->length &&
                   strchr("$#*0,", using->character[*usingpos]))
              {
                switch (using->character[*usingpos])
                  {
                  case '$':
                    if (width == 0)
                      {
                        dollarleft = 1;
                      }

                    if (++dollar > 1)
                      {
                        ++width;
                      }
                    break;

                  case '*':
                    pad = '*';
                    ++width;
                    break;

                  case '0':
                    pad = '0';
                    ++width;
                    break;

                  case ',':
                    commas = 1;
                    ++width;
                    break;

                  default:
                    ++width;
                  }

                ++(*usingpos);
              }

            if (*usingpos < using->length &&
                using->character[*usingpos] == '.')
              {
                ++(*usingpos);
                ++width;
                precision = 0;
                while (*usingpos < using->length &&
                       strchr("*#", using->character[*usingpos]))
                  {
                    ++(*usingpos);
                    ++precision;
                    ++width;
                  }

                if (width == 1 && precision == 0)
                  {
                    Value_destroy(self);
                    return Value_new_ERROR(self, BADFORMAT);
                  }
              }

            if (*usingpos < using->length &&
                using->character[*usingpos] == '-')
              {
                ++(*usingpos);
                if (headingsign == 0)
                  {
                    headingsign = 2;
                  }

                trailingsign = -1;
              }
            else if (*usingpos < using->length &&
                     using->character[*usingpos] == '+')
              {
                ++(*usingpos);
                if (headingsign == 0)
                  {
                    headingsign = 2;
                  }

                trailingsign = 1;
              }

            while (*usingpos < using->length &&
                   using->character[*usingpos] == '^')
              {
                ++(*usingpos);
                ++exponent;
              }

            goto work;
          }

        default:
          {
            String_appendChar(s, using->character[(*usingpos)++]);
          }
        }
    }

work:
  Value_toString(self, s, pad, headingsign, width, commas, dollar,
                 dollarleft, precision, exponent, trailingsign);
  if ((self->type == V_INTEGER || self->type == V_REAL) && width == 0 &&
      precision == -1)
    {
      String_appendChar(s, ' ');
    }

  while (*usingpos < using->length)
    {
      switch (using->character[*usingpos])
        {
        case '_':              /* output next char */
          {
            ++(*usingpos);
            if (*usingpos < using->length)
              {
                String_appendChar(s, using->character[(*usingpos)++]);
              }
            else
              {
                Value_destroy(self);
                return Value_new_ERROR(self, MISSINGCHARACTER);
              }
            break;
          }

        case '!':
        case '\\':
        case '&':
        case '*':
        case '0':
        case '+':
        case '#':
        case '.':
          return self;

        default:
          {
            String_appendChar(s, using->character[(*usingpos)++]);
          }
        }
    }

  return self;
}

struct String *Value_toWrite(struct Value *self, struct String *s)
{
  switch (self->type)
    {
    case V_INTEGER:
      String_appendPrintf(s, "%ld", self->u.integer);
      break;

    case V_REAL:
      {
        double x;
        int p = DBL_DIG;
        int n;
        int o;

        x = (self->u.real < 0.0 ? -self->u.real : self->u.real);
        while (x > 1.0 && p > 0)
          {
            x /= 10.0;
            --p;
          }

        o = s->length;
        String_appendPrintf(s, "%.*f", p, self->u.real);
        n = s->length;
        if (memchr(s->character + o, '.', n - o))
          {
            while (s->character[s->length - 1] == '0')
              {
                --s->length;
              }

            if (s->character[s->length - 1] == '.')
              {
                --s->length;
              }
          }
        break;
      }

    case V_STRING:
      {
        size_t l = self->u.string.length;
        char *data = self->u.string.character;

        String_appendChar(s, '"');
        while (l--)
          {
            if (*data == '"')
              {
                String_appendChar(s, '"');
              }

            String_appendChar(s, *data);
            ++data;
          }

        String_appendChar(s, '"');
        break;
      }

    default:
      assert(0);
    }

  return s;
}

struct Value *Value_nullValue(enum ValueType type)
{
  static struct Value integer =
  {
    V_INTEGER
  };

  static struct Value real =
  {
    V_REAL
  };

  static struct Value string =
  {
    V_STRING
  };

  static int init = 0;

  if (!init)
    {
      integer.u.integer = 0;
      real.u.real = 0.0;
      string.u.string.length = 0;
      string.u.string.character = "";
      init = 1;
    }

  switch (type)
    {
    case V_INTEGER:
      return &integer;

    case V_REAL:
      return &real;

    case V_STRING:
      return &string;

    default:
      assert(0);
    }

  return (struct Value *)0;
}

long int lrint(double d)
{
  return d;
}
