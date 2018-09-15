/****************************************************************************
 * apps/system/hexed/src/hexword.c
 * hexed word command
 *
 *   Copyright (c) 2011, B.ZaaR, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   The names of contributors may not be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "bfile.h"
#include "hexed.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

int g_wordsize;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Set the word size */

static int setword(FAR struct command_s *cmd, int optc, char *opt)
{
  char *s;
  size_t v;

  /* NULL option */

  if (opt == NULL)
    {
      return -EFAULT;
    }

  v = strtoll(opt, &s, 0x10);

  /* No value set in option */

  if (s == opt)
    {
      return -EINVAL;
    }

  /* Set global word size */

  if (optc == 0)
    {
      if (v >= WORD_64)
        {
          g_wordsize = WORD_64;
        }
      else if (v >= WORD_32)
        {
          g_wordsize = WORD_32;
        }
      else if (v >= WORD_16)
        {
          g_wordsize = WORD_16;
        }
      else
        {
          g_wordsize = WORD_8;
        }

      optc = 0; /* no more options accepted */
    }
  else
    {
      /* Too many options specified */

      return -E2BIG;
    }

  return optc;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Word command */

int hexword(FAR struct command_s *cmd, int optc, char *opt)
{
  /* Invalid command */

  if (cmd == NULL || cmd->id != CMD_WORD)
    {
      return -EINVAL;
    }

  /* Set word size */

  if (optc >= 0)
    {
      optc = setword(cmd, optc, opt);
    }
  else
    {
      optc = 0;
    }

  return optc;
}
