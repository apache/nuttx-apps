/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_readdecimal.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

/*************************************************************************
 * Included files
 *************************************************************************/

#include <stdbool.h>
#include <stdio.h>

#include "wld_utils.h"

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_read_decimal
 * Description:
 * Read a decimal number from the steam fp
 ************************************************************************/

int16_t wld_read_decimal(FILE * fp)
{
  int16_t value = 0;
  bool negative = false;
  int ch;

  /* Skip over any leading spaces, new lines, or carriage returns (for MSDOS
   * compatibility)
   */

  do
    {
      ch = getc(fp);
    }
  while ((ch == ' ') || (ch == '\n') || (ch == '\r'));

  /* if the first character is '-', then its a negative number */

  if (ch == '-')
    {
      negative = true;
      ch = getc(fp);
    }

  /* Now get the unsigned portion of the number */

  while ((ch >= '0') && (ch <= '9'))
    {
      value = 10 * value + (ch - (int)'0');
      ch = getc(fp);
    }

  /* Apply the negation, if appropriate */

  if (negative)
    {
      value = -value;
    }

  return value;
}
