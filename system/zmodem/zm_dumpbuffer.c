/****************************************************************************
 * apps/system/zmodem/zm_dumpbuffer.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
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

#include <nuttx/config.h>

#include <stdio.h>

#include "zm.h"

#ifdef CONFIG_SYSTEM_ZMODEM_DUMPBUFFER

/****************************************************************************
 * Pre-processor definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  zm_dumpbuffer
 *
 * Description:
 *  Dump a buffer of zmodem data.
 *
 ****************************************************************************/

void zm_dumpbuffer(FAR const char *msg, FAR const void *buffer, size_t buflen)
{
  FAR const uint8_t *ptr = (FAR const uint8_t *)buffer;
  size_t i;
  int j, k;

  zmprintf("%s [%p]:\n", msg, ptr);
  for (i = 0; i < buflen; i += 32)
    {
      zmprintf("%04x: ", i);
      for (j = 0; j < 32; j++)
        {
          k = i + j;

          if (j == 16)
            {
              zmprintf(" ");
            }

          if (k < buflen)
            {
              zmprintf("%02x", ptr[k]);
            }
          else
            {
              zmprintf("  ");
            }
        }

      zmprintf(" ");
      for (j = 0; j < 32; j++)
        {
         k = i + j;

          if (j == 16)
            {
              zmprintf(" ");
            }

          if (k < buflen)
            {
              if (ptr[k] >= 0x20 && ptr[k] < 0x7f)
                {
                  zmprintf("%c", ptr[k]);
                }
              else
                {
                  zmprintf(".");
                }
            }
        }
      zmprintf("\n");
   }
}

#endif /* CONFIG_SYSTEM_ZMODEM_DUMPBUFFER */
