/****************************************************************************
 * apps/nshlib/nsh_printf.c
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
  uint32_t value;
  int len;
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
                value = strtoul(fmt, NULL, 16);
                len = strnlen(fmt, 10);

                if (len >= 7)
                  {
                    nsh_output(vtbl, "%c%c%c%c", value & 0xff,
                                                 (value & 0xff00) >> 8,
                                                 (value & 0xff0000) >> 16,
                                                 (value & 0xff000000) >> 24);
                  }
                else if (len >= 3)
                    {
                      nsh_output(vtbl, "%c%c", value & 0xff,
                                               (value & 0xff00) >> 8);
                    }
                  else if (len >= 1)
                      {
                        nsh_output(vtbl, "%c", value & 0xff);
                      }
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
