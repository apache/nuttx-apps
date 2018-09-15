/****************************************************************************
 * apps/nshlib/nsh_printf.c
 *
 *   Copyright (C) 2016 Alan Carvalho de Assis. All rights reserved.
 *   Author: Alan Carvalho de Assis <acassis@gmail.com>
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

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <errno.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_printf
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_PRINTF
int cmd_printf(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR char *fmt;
  char ch;
  int i;

  /* parse each argument, detecting the right action to take
   * case it doesn't starts with a format then just print it
   */

  for (i = 1; i < argc; i++)
    {
      fmt = argv[i];

      /* Is it really a FORMAT ? */

      if (fmt[0] == '\\')
        {
          fmt++;
          ch = fmt[0];

          switch (ch)
            {
              case 'x':
                fmt++;
                nsh_output(vtbl, "%c", strtol(fmt, NULL, 16));
                break;

              case 'n':
                nsh_output(vtbl, "\n");
                break;

              case 'r':
                nsh_output(vtbl, "\r");
                break;

              case 't':
                nsh_output(vtbl, "\t");
                break;

              default:
               break;
            }
        }
      else
        {
          nsh_output(vtbl, "%s", argv[i]);
        }
    }

  return OK;
}
#endif
