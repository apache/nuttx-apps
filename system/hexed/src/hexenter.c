/****************************************************************************
 * apps/system/hexed/src/hexenter.c
 * hexed enter command
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bfile.h"
#include "hexed.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Enter bytes at dest */

static int runenter(FAR struct command_s *cmd)
{
  /* Write to file */

  bfwrite(g_hexfile, cmd->opts.dest, cmd->opts.buf, cmd->opts.bytes);
  return 0;
}

/* Set the enter command */

static int setenter(FAR struct command_s *cmd, int optc, FAR char *opt)
{
  FAR char *s;
  FAR int64_t *hx;
  int64_t v;

  /* Set defaults */

  if (optc == 0)
    {
      cmd->flags |= CMD_FL_QUIT;
    }

  /* NULL option */

  if (opt == NULL)
    {
      return -EINVAL;
    }

  v = strtoll(opt, &s, 0x10);

  /* No value set in option */

  if (s == opt)
    {
      return -EINVAL;
    }

  /* Set destination */

  if (optc == 0)
    {
      cmd->opts.dest = v;
      optc++;

      /* Set values */
    }
  else if (optc > 0 && cmd->opts.cnt < OPT_BUF_SZ)
    {
      hx = cmd->opts.buf;
      switch (cmd->opts.word)
        {
        case WORD_64:
          *(hx + cmd->opts.cnt) = v;
          break;

        case WORD_32:
          *((int32_t *)hx + cmd->opts.cnt) = v;
          break;

        case WORD_16:
          *((int16_t *)hx + cmd->opts.cnt) = v;
          break;

        case WORD_8:
          *((int8_t *)hx + cmd->opts.cnt) = v;
          break;

        default:
          break;
        }

      cmd->opts.cnt++;
      cmd->opts.bytes = cmd->opts.cnt * cmd->opts.word;
      optc++;
    }

  /* Buffer overflow */

  else
    {
      fprintf(stderr, "ERROR: too many values set\n");
      return -E2BIG;
    }

  return optc;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Enter command */

int hexenter(FAR struct command_s *cmd, int optc, char *opt)
{
  /* Invalid command */

  if (cmd == NULL || cmd->id != CMD_ENTER)
    {
      return -EINVAL;
    }

  /* Set/run enter */

  if (optc >= 0)
    {
      optc = setenter(cmd, optc, opt);
    }
  else
    {
      optc = runenter(cmd);
    }

  return optc;
}
