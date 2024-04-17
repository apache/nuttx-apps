/****************************************************************************
 * apps/system/hexed/src/hexcopy.c
 * hexed copy command
 *
 *   Copyright (c) 2011, B.ZaaR, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

/* Copy len words from src to dest */

static int copy(FAR struct command_s *cmd)
{
  bfcopy(g_hexfile, cmd->opts.dest, cmd->opts.src, cmd->opts.bytes);
  return 0;
}

/* Copy overwriting len words from src to dest */

static int copyover(FAR struct command_s *cmd)
{
  /* Adjust length for EOF */

  if ((cmd->opts.src >= cmd->opts.dest) &&
      (cmd->opts.src + cmd->opts.bytes > g_hexfile->size))
    {
      cmd->opts.bytes = g_hexfile->size - cmd->opts.src;
    }

  /* Copy overwrite */

  bfwrite(g_hexfile, cmd->opts.dest, g_hexfile->buf + cmd->opts.src,
          cmd->opts.bytes);
  return 0;
}

/* Run the copy command */

static int runcopy(FAR struct command_s *cmd)
{
  /* Error: Source past EOF */

  if (cmd->opts.src > g_hexfile->size)
    {
      return -EINVAL;
    }

  switch (cmd->id)
    {
    case CMD_COPY:
      return copy(cmd);

    case CMD_COPY_OVER:
      return copyover(cmd);

    default:
      return -EINVAL;
    }
}

/* Set the copy command */

static int setcopy(FAR struct command_s *cmd, int optc, char *opt)
{
  FAR char *s;
  uint64_t v;

  /* Set defaults */

  if (optc == 0)
    {
      cmd->flags |= CMD_FL_QUIT;
    }

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

  /* Set options */

  switch (optc)
    {
      case 0:  /* Set source */
        {
          cmd->opts.src = v;
          optc++;
        }
        break;

      case 1:  /* Set destination */
        {
          cmd->opts.dest = v;
          optc++;
        }
        break;

      case 2:  /* Set length */
        {
          cmd->opts.len = v;
          cmd->opts.bytes = cmd->opts.len * cmd->opts.word;
          optc = 0;
        }
        break;

      default:  /* Too many options specified */
        return -E2BIG;
    }

  return optc;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Copy command */

int hexcopy(FAR struct command_s *cmd, int optc, char *opt)
{
  /* Invalid command */

  if (cmd == NULL || (cmd->id != CMD_COPY && cmd->id != CMD_COPY_OVER))
    {
      return -EINVAL;
    }

  /* Set/run copy */

  if (optc >= 0)
    {
      optc = setcopy(cmd, optc, opt);
    }
  else
    {
      optc = runcopy(cmd);
    }

  return optc;
}
