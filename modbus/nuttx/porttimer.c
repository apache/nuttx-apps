/****************************************************************************
 * apps/modbus/nuttx/porttimer.c
 *
 * FreeModbus Library: NuttX Port
 * Copyright (c) 2006 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/time.h>
#include <stdlib.h>
#include <assert.h>

#include "port.h"

#include <apps/modbus/mb.h>
#include <apps/modbus/mbport.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

uint32_t ulTimeOut;
bool     bTimeoutEnable;

static struct timeval xTimeLast;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool xMBPortTimersInit(uint16_t usTim1Timerout50us)
{
  ulTimeOut = usTim1Timerout50us / 20U;
  if (ulTimeOut == 0)
    {
      ulTimeOut = 1;
    }

  return xMBPortSerialSetTimeout(ulTimeOut);
}

void xMBPortTimersClose()
{
  /* Does not use any hardware resources. */
}

void vMBPortTimerPoll()
{
  uint32_t       ulDeltaMS;
  struct timeval xTimeCur;

  /* Timers are called from the serial layer because we have no high
   * res timer in Win32.
   */

  if (bTimeoutEnable)
    {
      if (gettimeofday(&xTimeCur, NULL) != 0)
        {
          /* gettimeofday failed - retry next time. */
        }
      else
        {
          ulDeltaMS = (xTimeCur.tv_sec - xTimeLast.tv_sec) * 1000L +
                      (xTimeCur.tv_usec - xTimeLast.tv_usec) * 1000L;
          if (ulDeltaMS > ulTimeOut)
            {
              bTimeoutEnable = false;
              (void)pxMBPortCBTimerExpired();
            }
        }
    }
}

void vMBPortTimersEnable()
{
  int res = gettimeofday(&xTimeLast, NULL);

  ASSERT(res == 0);
  bTimeoutEnable = true;
}

void vMBPortTimersDisable()
{
  bTimeoutEnable = false;
}
