/****************************************************************************
 * apps/netutils/netlib/netlib_saddrconv.c
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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <nuttx/net/sixlowpan.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_saddrconv
 ****************************************************************************/

bool netlib_saddrconv(FAR const char *hwstr, FAR uint8_t *hw)
{
  unsigned char tmp;
  unsigned char i;
  unsigned char j;
  char ch;

  /* Extended Address Form:  xx:xx */

  if (strlen(hwstr) != 5)
    {
      return false;
    }

  tmp = 0;

  for (i = 0; i < 2; ++i)
    {
      j = 0;
      do
        {
          ch = *hwstr++;
          if (++j > 3)
            {
              return false;
            }

          if (ch == ':' || ch == '\0')
            {
              *hw++ = tmp;
              tmp = 0;
            }
          else if (ch >= '0' && ch <= '9')
            {
              tmp = (tmp << 4) + (ch - '0');
            }
          else if (ch >= 'a' && ch <= 'f')
            {
              tmp = (tmp << 4) + (ch - 'a' + 10);
            }
          else if (ch >= 'A' && ch <= 'F')
            {
              tmp = (tmp << 4) + (ch - 'A' + 10);
            }
          else
            {
              return false;
            }
        }
      while (ch != ':' && ch != 0);
    }

  return true;
}
