/****************************************************************************
 * apps/graphics/traveler/src/trv_fsutils.c
 * Miscellaneous file access utilities
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Included files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_main.h"
#include "trv_fsutils.h"

#include <stdio.h>
#include <ctype.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_read_decimal
 *
 * Description:
 *   Read a decimal number from the steam 'fp'
 *
 ****************************************************************************/

int16_t trv_read_decimal(FAR FILE *fp)
{
  int16_t value = 0;
  bool negative = false;
  int ch;

  /* Skip over any leading whitespace  */

  do
    {
      ch = getc(fp);
    }
  while (isspace(ch));

  /* if the first character is '-', then its a negative number */

  if (ch == '-')
    {
      negative = true;
      ch = getc(fp);
    }

  /* Now get the unsigned portion of the number */

  while (ch >= '0' && ch <= '9')
    {
      value = 10  *value + (ch - (int)'0');
      ch = getc(fp);
    }

  /* Apply the negation, if appropriate */

  if (negative)
    {
      value = -value;
    }

  return value;
}

/****************************************************************************
 * Name: trv_fullpath
 *
 * Description:
 *   Concatenate a filename and a path to produce the full, absolute path
 *   to the file.  The pointer returned by this function is allocated and
 *   must be freed by the caller.
 *
 ****************************************************************************/

FAR char *trv_fullpath(FAR const char *path, FAR const char *name)
{
  FAR char *fullpath = NULL;

  (void)asprintf(&fullpath, "%s/%s", path, name);
  if (!fullpath)
    {
      trv_abort("ERROR: Failured to created full path\n");
    }

  return fullpath;
}
