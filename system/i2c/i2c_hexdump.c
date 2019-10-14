/****************************************************************************
 * apps/system/i2c/i2c_hexdump.c
 *
 *   Copyright (C) 2005, 2016-2019 David S. Alessio. All rights reserved.
 *   Author: David S. Alessio <david.s.alessio@gmail.com>
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>

#include "i2ctool.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hexdump_line
 ****************************************************************************/

static int hexdump_line(FILE *ostream, void *addr, int len)
{
  int i;
  uint8_t *p;

  if (len <= 0)
    {
      return 0;
    }

  /* print at most 16 chars */

  if (len > 16)
    {
      len = 16;
    }

  /* print hex */

  for (i = 0, p = addr; i < len; ++i, ++p)
    {
      if (i % 8 == 0)
        {
          fputc(' ', ostream);
        }

      fprintf(ostream, "%02x ", *p);
    }

  /* pad if necessary */

  if (i <= 8)
    {
      fputc(' ', ostream);
    }

  while (i++ < 16)
    {
      fputs("   ", ostream);
    }

  /* print ASCII */

  fputs(" |", ostream);
  for (i = 0, p = addr; i < len; ++i, ++p)
    {
      fputc(isprint(*p) ? *p : '.', ostream);
    }

  /* pad if necessary */

  while (i++ < 16)
    {
      fputc(' ', ostream);
    }

  fputs("|\n", ostream);
  return len;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void i2ctool_hexdump(FILE *outstream, void *addr, int len)
{
  int nbytes;
  uint8_t *p = addr;

  /* print one line at a time */

  while (len > 0)
    {
      /* print address */

      fprintf(outstream, "%08x ", p - (uint8_t *) addr);

      /* print one line of data */

      nbytes = hexdump_line(outstream, p, len);
      len -= nbytes;
      p += nbytes;
    }
}
