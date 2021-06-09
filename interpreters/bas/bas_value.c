/****************************************************************************
 * apps/interpreters/bas/bas_value.c
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
          int o, n, p = 6;

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

struct Value *Value_new_NIL(struct Value *this)
{
  assert(this != (struct Value *)0);
  this->type = V_NIL;
  return this;
}

struct Value *Value_new_ERROR(struct Value *this, int code, const char *error,
                              ...)
{
  va_list ap;
  char buf[128];

  assert(this != (struct Value *)0);
  va_start(ap, error);
  vsprintf(buf, error, ap);
  va_end(ap);
  this->type = V_ERROR;
  this->u.error.code = code;
  this->u.error.msg = strcpy(malloc(strlen(buf) + 1), buf);
  return this;
}

struct Value *Value_new_INTEGER(struct Value *this, int n)
{
  assert(this != (struct Value *)0);
  this->type = V_INTEGER;
  this->u.integer = n;
  return this;
}

struct Value *Value_new_REAL(struct Value *this, double n)
{
  assert(this != (struct Value *)0);
  this->type = V_REAL;
  this->u.real = n;
  return this;
}

struct Value *Value_new_STRING(struct Value *this)
{
  assert(this != (struct Value *)0);
  this->type = V_STRING;
  String_new(&this->u.string);
  return this;
}

struct Value *Value_new_VOID(struct Value *this)
{
  assert(this != (struct Value *)0);
  this->type = V_VOID;
  return this;
}

struct Value *Value_new_null(struct Value *this, enum ValueType type)
{
  assert(this != (struct Value *)0);
  switch (type)
    {
    case V_INTEGER:
      {
        this->type = V_INTEGER;
        this->u.integer = 0;
        break;
      }

    case V_REAL:
      {
        this->type = V_REAL;
        this->u.real = 0.0;
        break;
      }

    case V_STRING:
      {
        this->type = V_STRING;
        String_new(&this->u.string);
        break;
      }

    case V_VOID:
      {
        this->type = V_VOID;
        break;
      }

    default:
      assert(0);
    }

  return this;
}

int Value_isNull(const struct Value *this)
{
  switch (this->type)
    {
    case V_INTEGER:
      return (this->u.integer == 0);

    case V_REAL:
      return (this->u.real == 0.0);

    case V_STRING:
      return (this->u.string.length == 0);

    default:
      assert(0);
    }

  return -1;
}

void Value_destroy(struct Value *this)
{
  assert(this != (struct Value *)0);
  switch (this->type)
    {
    case V_ERROR:
      free(this->u.error.msg);
      break;

    case V_INTEGER:
      break;

    case V_NIL:
      break;

    case V_REAL:
      break;

    case V_STRING:
      String_destroy(&this->u.string);
      break;

    case V_VOID:
      break;

    default:
      assert(0);
    }

  this->type = 0;
}

struct Value *Value_clone(struct Value *this, const struct Value *original)
{
  assert(this != (struct Value *)0);
  assert(original != (struct Value *)0);
  switch (original->type)
    {
    case V_ERROR:
      {
        strcpy(this->u.error.msg =
               malloc(strlen(original->u.error.msg) + 1),
               original->u.error.msg);
        this->u.error.code = original->u.error.code;
        break;
      }

    case V_INTEGER:
      this->u.integer = original->u.integer;
      break;

    case V_NIL:
      break;

    case V_REAL:
      this->u.real = original->u.real;
      break;

    case V_STRING:
      String_clone(&this->u.string, &original->u.string);
      break;

    default:
      assert(0);
    }

  this->type = original->type;
  return this;
}

struct Value *Value_uplus(struct Value *this, int calc)
{
  switch (this->type)
    {
    case V_INTEGER:
    case V_REAL:
      {
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDUOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_uneg(struct Value *this, int calc)
{
  switch (this->type)
    {
    case V_INTEGER:
      {
        if (calc)
          {
            this->u.integer = -this->u.integer;
          }
        break;
      }

    case V_REAL:
      {
        if (calc)
          {
            this->u.real = -this->u.real;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDUOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_unot(struct Value *this, int calc)
{
  switch (this->type)
    {
    case V_INTEGER:
      {
        if (calc)
          {
            this->u.integer = ~this->u.integer;
          }
        break;
      }

    case V_REAL:
      {
        Value_retype(this, V_INTEGER);
        if (calc)
          {
            this->u.integer = ~this->u.integer;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDUOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_add(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer += x->u.integer;
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            this->u.real += x->u.real;
          }
        break;
      }

    case V_STRING:
      {
        if (calc)
          {
            String_appendString(&this->u.string, &x->u.string);
          }
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_sub(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer -= x->u.integer;
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            this->u.real -= x->u.real;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_mult(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer *= x->u.integer;
          }

        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            this->u.real *= x->u.real;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_div(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            if (x->u.real == 0)
              {
                Value_destroy(this);
                Value_new_ERROR(this, UNDEFINED, "Division by zero");
              }
            else
              {
                this->u.real /= x->u.real;
              }
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            if (x->u.real == 0.0)
              {
                Value_destroy(this);
                Value_new_ERROR(this, UNDEFINED, "Division by zero");
              }
            else
              {
                this->u.real /= x->u.real;
              }
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_idiv(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            if (x->u.integer == 0)
              {
                Value_destroy(this);
                Value_new_ERROR(this, UNDEFINED, "Division by zero");
              }
            else
              {
                this->u.integer /= x->u.integer;
              }
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            if (x->u.real == 0.0)
              {
                Value_destroy(this);
                Value_new_ERROR(this, UNDEFINED, "Division by zero");
              }
            else
              {
                this->u.real = Value_trunc(this->u.real / x->u.real);
              }
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_mod(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            if (x->u.integer == 0)
              {
                Value_destroy(this);
                Value_new_ERROR(this, UNDEFINED, "Modulo by zero");
              }
            else
              {
                this->u.integer %= x->u.integer;
              }
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            if (x->u.real == 0.0)
              {
                Value_destroy(this);
                Value_new_ERROR(this, UNDEFINED, "Modulo by zero");
              }
            else
              {
                this->u.real = fmod(this->u.real, x->u.real);
              }
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_pow(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            if (this->u.integer == 0 && x->u.integer == 0)
              {
                Value_destroy(this);
                Value_new_ERROR(this, UNDEFINED, "0^0");
              }
            else if (x->u.integer > 0)
              {
                this->u.integer = pow(this->u.integer, x->u.integer);
              }
            else
              {
                long int thisi = this->u.integer;
                Value_destroy(this);
                Value_new_REAL(this, pow(thisi, x->u.integer));
              }
          }
        break;
      }

    case V_REAL:
      {
        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            if (this->u.real == 0.0 && x->u.real == 0.0)
              {
                Value_destroy(this);
                Value_new_ERROR(this, UNDEFINED, "0^0");
              }
            else
              {
                this->u.real = pow(this->u.real, x->u.real);
              }
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_and(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
    case V_REAL:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer &= x->u.integer;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_or(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
    case V_REAL:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer |= x->u.integer;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_xor(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
    case V_REAL:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer ^= x->u.integer;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_eqv(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
    case V_REAL:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer = ~(this->u.integer ^ x->u.integer);
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_imp(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
    case V_REAL:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer = (~this->u.integer) | x->u.integer;
          }
        break;
      }

    case V_STRING:
      {
        Value_destroy(this);
        Value_new_ERROR(this, INVALIDOPERAND);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_lt(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer = (this->u.integer < x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (this->u.real < x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = (String_cmp(&this->u.string, &x->u.string) < 0) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_le(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer = (this->u.integer <= x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (this->u.real <= x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = (String_cmp(&this->u.string, &x->u.string) <= 0) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_eq(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer = (this->u.integer == x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (this->u.real == x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }
    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = (String_cmp(&this->u.string, &x->u.string) == 0) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_ge(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer = (this->u.integer >= x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (this->u.real >= x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = (String_cmp(&this->u.string, &x->u.string) >= 0) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_gt(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer = (this->u.integer > x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (this->u.real > x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = (String_cmp(&this->u.string, &x->u.string) > 0) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

struct Value *Value_ne(struct Value *this, struct Value *x, int calc)
{
  switch (Value_commonType[this->type][x->type])
    {
    case V_INTEGER:
      {
        VALUE_RETYPE(this, V_INTEGER);
        VALUE_RETYPE(x, V_INTEGER);
        if (calc)
          {
            this->u.integer = (this->u.integer != x->u.integer) ? -1 : 0;
          }
        break;
      }

    case V_REAL:
      {
        int v;

        VALUE_RETYPE(this, V_REAL);
        VALUE_RETYPE(x, V_REAL);
        if (calc)
          {
            v = (this->u.real != x->u.real) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    case V_STRING:
      {
        int v;

        if (calc)
          {
            v = String_cmp(&this->u.string, &x->u.string) ? -1 : 0;
          }
        else
          {
            v = 0;
          }

        Value_destroy(this);
        Value_new_INTEGER(this, v);
        break;
      }

    default:
      assert(0);
    }

  return this;
}

int Value_exitFor(struct Value *this, struct Value *limit, struct Value *step)
{
  switch (this->type)
    {
    case V_INTEGER:
      return
        (step->u.integer < 0
         ? (this->u.integer < limit->u.integer)
         : (this->u.integer > limit->u.integer));

    case V_REAL:
      return
        (step->u.real < 0.0
         ? (this->u.real < limit->u.real) : (this->u.real > limit->u.real));

    case V_STRING:
      return (String_cmp(&this->u.string, &limit->u.string) > 0);

    default:
      assert(0);
    }

  return -1;
}

void Value_errorPrefix(struct Value *this, const char *prefix)
{
  size_t prefixlen, msglen;

  assert(this->type == V_ERROR);
  prefixlen = strlen(prefix);
  msglen = strlen(this->u.error.msg);
  this->u.error.msg = realloc(this->u.error.msg, prefixlen + msglen + 1);
  memmove(this->u.error.msg + prefixlen, this->u.error.msg, msglen);
  memcpy(this->u.error.msg, prefix, prefixlen);
}

void Value_errorSuffix(struct Value *this, const char *suffix)
{
  size_t suffixlen, msglen;

  assert(this->type == V_ERROR);
  suffixlen = strlen(suffix);
  msglen = strlen(this->u.error.msg);
  this->u.error.msg = realloc(this->u.error.msg, suffixlen + msglen + 1);
  memcpy(this->u.error.msg + msglen, suffix, suffixlen + 1);
}

struct Value *Value_new_typeError(struct Value *this, enum ValueType t1,
                                  enum ValueType t2)
{
  assert(typestr[t1]);
  assert(typestr[t2]);
  return Value_new_ERROR(this, TYPEMISMATCH1, _(typestr[t1]), _(typestr[t2]));
}

static void retypeError(struct Value *this, enum ValueType to)
{
  enum ValueType thisType = this->type;

  assert(typestr[thisType]);
  assert(typestr[to]);
  Value_destroy(this);
  Value_new_ERROR(this, TYPEMISMATCH1, _(typestr[thisType]), _(typestr[to]));
}

struct Value *Value_retype(struct Value *this, enum ValueType type)
{
  switch (this->type)
    {
    case V_INTEGER:
      {
        switch (type)
          {
          case V_INTEGER:
            break;

          case V_REAL:
            this->u.real = this->u.integer;
            this->type = type;
            break;

          case V_VOID:
            Value_destroy(this);
            Value_new_VOID(this);
            break;

          default:
            retypeError(this, type);
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
              this->u.integer = Value_toi(this->u.real, &overflow);
              this->type = V_INTEGER;
              if (overflow)
                {
                  Value_destroy(this);
                  Value_new_ERROR(this, OUTOFRANGE, typestr[V_INTEGER]);
                }
              break;
            }

          case V_REAL:
            break;

          case V_VOID:
            Value_destroy(this);
            Value_new_VOID(this);
            break;

          default:
            retypeError(this, type);
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
            Value_destroy(this);
            Value_new_VOID(this);
            break;

          default:
            retypeError(this, type);
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
            retypeError(this, type);
          }
        break;
      }

    case V_ERROR:
      break;

    default:
      assert(0);
    }

  return this;
}

struct String *Value_toString(struct Value *this, struct String *s, char pad,
                              int headingsign, size_t width, int commas,
                              int dollar, int dollarleft, int precision,
                              int exponent, int trailingsign)
{
  size_t oldlength = s->length;

  switch (this->type)
    {
    case V_ERROR:
      String_appendChars(s, this->u.error.msg);
      break;

    case V_REAL:
    case V_INTEGER:
      {
        int sign;
        struct String buf;
        size_t totalwidth = width;

        String_new(&buf);
        if (this->type == V_INTEGER)
          {
            if (this->u.integer < 0)
              {
                sign = -1;
                this->u.integer = -this->u.integer;
              }
            else if (this->u.integer == 0)
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
            if (this->u.real < 0.0)
              {
                sign = -1;
                this->u.real = -this->u.real;
              }
            else if (this->u.real == 0.0)
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
        if (this->type == V_INTEGER)
          {
            if (precision > 0 || exponent)
              {
                format_double(&buf, (double)this->u.integer, width, precision,
                              exponent);
              }
            else if (precision == 0)
              {
                String_appendPrintf(&buf, "%lu.", this->u.integer);
              }
            else
              {
                String_appendPrintf(&buf, "%lu", this->u.integer);
              }
          }
        else
          {
            format_double(&buf, this->u.real, width, precision, exponent);
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
              (this->u.string.length <
               width ? (width - this->u.string.length) : 0);

            String_size(s, oldlength + width);
            memcpy(s->character + oldlength, this->u.string.character,
                   blanks ? this->u.string.length : width);
            if (blanks)
              {
                memset(s->character + oldlength + this->u.string.length, ' ',
                       blanks);
              }
          }
        else
          {
            String_appendString(s, &this->u.string);
          }
        break;
      }

    default:
      assert(0);
      return 0;
    }

  return s;
}

struct Value *Value_toStringUsing(struct Value *this, struct String *s,
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
                Value_destroy(this);
                return Value_new_ERROR(this, MISSINGCHARACTER);
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
                Value_destroy(this);
                return Value_new_ERROR(this, IOERROR,
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

            if (*usingpos < using->length && using->character[*usingpos] == '.')
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
                    Value_destroy(this);
                    return Value_new_ERROR(this, BADFORMAT);
                  }
              }

            if (*usingpos < using->length && using->character[*usingpos] == '-')
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
  Value_toString(this, s, pad, headingsign, width, commas, dollar, dollarleft,
                 precision, exponent, trailingsign);
  if ((this->type == V_INTEGER || this->type == V_REAL) && width == 0 &&
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
                Value_destroy(this);
                return Value_new_ERROR(this, MISSINGCHARACTER);
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
          return this;

        default:
          {
            String_appendChar(s, using->character[(*usingpos)++]);
          }
        }
    }

  return this;
}

struct String *Value_toWrite(struct Value *this, struct String *s)
{
  switch (this->type)
    {
    case V_INTEGER:
      String_appendPrintf(s, "%ld", this->u.integer);
      break;

    case V_REAL:
      {
        double x;
        int p = DBL_DIG;
        int n, o;

        x = (this->u.real < 0.0 ? -this->u.real : this->u.real);
        while (x > 1.0 && p > 0)
          {
            x /= 10.0;
            --p;
          }

        o = s->length;
        String_appendPrintf(s, "%.*f", p, this->u.real);
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
        size_t l = this->u.string.length;
        char *data = this->u.string.character;

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
  static struct Value integer = { V_INTEGER };
  static struct Value real = { V_REAL };
  static struct Value string = { V_STRING };
  static char n[] = "";
  static int init = 0;

  if (!init)
    {
      integer.u.integer = 0;
      real.u.real = 0.0;
      string.u.string.length = 0;
      string.u.string.character = n;
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
