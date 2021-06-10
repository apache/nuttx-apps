/****************************************************************************
 * apps/system/i2c/i2c_hexdump.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
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

      fprintf(outstream, "%08tx ", p - (uint8_t *) addr);

      /* print one line of data */

      nbytes = hexdump_line(outstream, p, len);
      len -= nbytes;
      p += nbytes;
    }
}
