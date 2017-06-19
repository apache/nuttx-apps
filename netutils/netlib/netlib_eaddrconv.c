/****************************************************************************
 * netutils/netlib/netlib_eaddrconv.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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
 * Name: netlib_eaddrconv
 ****************************************************************************/

bool netlib_eaddrconv(FAR const char *hwstr, FAR uint8_t *hw)
{
  unsigned char tmp;
  unsigned char i;
  unsigned char j;
  char ch;

  /* Extended Address Form:  xx:xx:xx:xx:xx:xx:xx:xx */

  if (strlen(hwstr) != 3 * 8 - 1)
    {
      return false;
    }

  tmp = 0;

  for (i = 0; i < 8; ++i)
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
