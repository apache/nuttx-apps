/****************************************************************************
 * apps/interpreters/bas/bas_value.h
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

#ifndef __APPS_EXAMPLES_BAS_BAS_VALUE_H
#define __APPS_EXAMPLES_BAS_BAS_VALUE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "bas_str.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VALUE_NEW_INTEGER(this,n) ((this)->type=V_INTEGER,(this)->u.integer=(n))
#define VALUE_NEW_REAL(this,n) ((this)->type=V_REAL,(this)->u.real=(n))
#define VALUE_RETYPE(v,t) ((v)->type==(t) ? (v) : Value_retype(v,t))
#define VALUE_DESTROY(this) assert((this)!=(struct Value*)0); \
  switch ((this)->type) \
  { \
    case V_ERROR: free((this)->u.error.msg); break; \
    case V_INTEGER: break; \
    case V_NIL: break; \
    case V_REAL: break; \
    case V_STRING: String_destroy(&(this)->u.string); break; \
    case V_VOID: break; \
    default: assert(0); \
  } \
  (this)->type=0;

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum ValueType
{
  V_ERROR=1,
  V_INTEGER,
  V_NIL,
  V_REAL,
  V_STRING,
  V_VOID
};

struct Value
{
  enum ValueType type;
  union
  {
    /* V_ERROR   */ struct { char *msg; long int code; } error;
    /* V_INTEGER */ long int integer;
    /* V_NIL     */
    /* V_REAL    */ double real;
    /* V_STRING  */ struct String string;
    /* V_VOID    */
  } u;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const enum ValueType Value_commonType[V_VOID+1][V_VOID+1];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

long int lrint(double d);
double Value_trunc(double d);
double Value_round(double d);
long int Value_toi(double d, int *overflow);
long int Value_vali(const char *s, char **end, int *overflow);
double Value_vald(const char *s, char **end, int *overflow);

struct Value *Value_new_NIL(struct Value *this);
struct Value *Value_new_ERROR(struct Value *this, int code,
                              const char *error, ...);
struct Value *Value_new_INTEGER(struct Value *this, int n);
struct Value *Value_new_REAL(struct Value *this, double n);
struct Value *Value_new_STRING(struct Value *this);
struct Value *Value_new_VOID(struct Value *this);
struct Value *Value_new_null(struct Value *this, enum ValueType type);
int Value_isNull(const struct Value *this);
void Value_destroy(struct Value *this);
void Value_errorPrefix(struct Value *this, const char *prefix);
void Value_errorSuffix(struct Value *this, const char *suffix);
struct Value *Value_new_typeError(struct Value *this, enum ValueType t1,
                                  enum ValueType t2);
struct Value *Value_retype(struct Value *this, enum ValueType type);
struct Value *Value_clone(struct Value *this, const struct Value *original);
struct Value *Value_uplus(struct Value *this, int calc);
struct Value *Value_uneg(struct Value *this, int calc);
struct Value *Value_unot(struct Value *this, int calc);
struct Value *Value_add(struct Value *this, struct Value *x, int calc);
struct Value *Value_sub(struct Value *this, struct Value *x, int calc);
struct Value *Value_mult(struct Value *this, struct Value *x, int calc);
struct Value *Value_div(struct Value *this, struct Value *x, int calc);
struct Value *Value_idiv(struct Value *this, struct Value *x, int calc);
struct Value *Value_mod(struct Value *this, struct Value *x, int calc);
struct Value *Value_pow(struct Value *this, struct Value *x, int calc);
struct Value *Value_and(struct Value *this, struct Value *x, int calc);
struct Value *Value_or(struct Value *this, struct Value *x, int calc);
struct Value *Value_xor(struct Value *this, struct Value *x, int calc);
struct Value *Value_eqv(struct Value *this, struct Value *x, int calc);
struct Value *Value_imp(struct Value *this, struct Value *x, int calc);
struct Value *Value_lt(struct Value *this, struct Value *x, int calc);
struct Value *Value_le(struct Value *this, struct Value *x, int calc);
struct Value *Value_eq(struct Value *this, struct Value *s, int calc);
struct Value *Value_ge(struct Value *this, struct Value *x, int calc);
struct Value *Value_gt(struct Value *this, struct Value *x, int calc);
struct Value *Value_ne(struct Value *this, struct Value *x, int calc);
int Value_exitFor(struct Value *this, struct Value *limit,
                  struct Value *step);
struct String *Value_toString(struct Value *this, struct String *s,
                              char pad, int headingsign, size_t width,
                              int commas, int dollar, int dollarleft,
                              int precision, int exponent,
                              int trailingsign);
struct Value *Value_toStringUsing(struct Value *this, struct String *s,
                                  struct String *using, size_t *usingpos);
struct String *Value_toWrite(struct Value *this, struct String *s);
struct Value *Value_nullValue(enum ValueType type);

#endif /* __APPS_EXAMPLES_BAS_BAS_VALUE_H */
