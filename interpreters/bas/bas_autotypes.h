/****************************************************************************
 * apps/interpreters/bas/bas_autotypes.h
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

/* REVISIT:  Why is this?  If the following is __APPS_EXAMPLES_BAS_BAS_AUTO_H
 * then there are compile errors!  Those compile errors occur because this
 * function defines some of the same structures as does bas_auto.h.  BUT, the
 * definitions ARE NOT THE SAME.  What is up with this?
 */

#ifndef __APPS_EXAMPLES_BAS_BAS_AUTO_H
#define __APPS_EXAMPLES_BAS_BAS_AUTO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "bas_program.h"
#include "bas_var.h"
#include "bas_token.h"

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
  int resumeable;

  struct Symbol *cur,*all;
};

union AutoSlot
{
  struct
  {
    long int framePointer;
    long int frameSize;
    struct Pc pc;
  } ret;
  struct Var var;
};

#endif /* __APPS_EXAMPLES_BAS_BAS_AUTO_H */
