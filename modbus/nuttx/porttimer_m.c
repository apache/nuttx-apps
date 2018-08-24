/****************************************************************************
 * apps/modbus/nuttx/porttimer_m.c
 *
 * FreeModbus Library: NuttX Modbus Master Port
 * Original work (c) 2006 Christian Walter <wolti@sil.at>
 * Modified work (c) 2016 Vytautas Lukenskas <lukevyta@gmail.com>
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

#include "modbus/mb.h"
#include "modbus/mb_m.h"
#include "modbus/mbport.h"

#if defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER)

#ifndef CONFIG_MB_MASTER_DELAY_MS_CONVERT
#  define MB_MASTER_DELAY_MS_CONVERT 200
#else
#  define MB_MASTER_DELAY_MS_CONVERT CONFIG_MB_MASTER_DELAY_MS_CONVERT
#endif

#ifndef CONFIG_MB_MASTER_TIMEOUT_MS_RESPOND
#  define MB_MASTER_TIMEOUT_MS_RESPOND 1000
#else
#  define MB_MASTER_TIMEOUT_MS_RESPOND CONFIG_MB_MASTER_TIMEOUT_MS_RESPOND
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

uint32_t ulTimeOut;                /* current timeout duration        */
uint32_t ulTimeoutT35;             /* 3.5 byte transmission duration  */
uint32_t ulTimeoutConvertDelay;    /* timeout after broadcast message */
uint32_t ulTimeoutResponse;        /* response timeout duration       */
static struct timeval xTimeLast;
bool bTimeoutEnable;               /* timeout is active */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

void vMBMasterPortTimersEnable( void )
{
  int res = gettimeofday(&xTimeLast, NULL);

  DEBUGASSERT(res == 0);
  bTimeoutEnable = true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool xMBMasterPortTimersInit(uint16_t usTimeOut50us)
{
  /* Configure all timeout values */

  ulTimeoutT35 = usTimeOut50us / 20U;
  if (ulTimeoutT35 == 0)
    {
      ulTimeoutT35 = 1;
    }

  ulTimeoutConvertDelay = MB_MASTER_DELAY_MS_CONVERT;
  if (ulTimeoutConvertDelay == 0)
    {
      ulTimeoutConvertDelay = 1;
    }

  ulTimeoutResponse = MB_MASTER_TIMEOUT_MS_RESPOND;
  if (ulTimeoutResponse == 0)
    {
      ulTimeoutResponse = 1;
    }

  ulTimeOut = ulTimeoutT35;

  return xMBMasterPortSerialSetTimeout(ulTimeOut);
}

void xMBMasterPortTimersClose()
{
  /* Does not use any hardware resources. */
}

INLINE void vMBMasterPortTimersT35Enable( void )
{
  vMBMasterPortTimersEnable();
  ulTimeOut = ulTimeoutT35;
  vMBMasterSetCurTimerMode(MB_TMODE_T35);
}

INLINE void vMBMasterPortTimersConvertDelayEnable( void )
{
  vMBMasterPortTimersEnable();
  ulTimeOut = ulTimeoutConvertDelay;
  vMBMasterSetCurTimerMode(MB_TMODE_CONVERT_DELAY);
}

INLINE void vMBMasterPortTimersRespondTimeoutEnable( void )
{
  vMBMasterPortTimersEnable();
  ulTimeOut = ulTimeoutResponse;
  vMBMasterSetCurTimerMode( MB_TMODE_RESPOND_TIMEOUT );
}

void vMBMasterPortTimerPoll( void )
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
                      (xTimeCur.tv_usec - xTimeLast.tv_usec) / 1000L;
          if (ulDeltaMS > ulTimeOut)
            {
              bTimeoutEnable = false;
              (void)pxMBMasterPortCBTimerExpired();
            }
        }
    }
}

void vMBMasterPortTimersDisable()
{
  bTimeoutEnable = false;
}

#endif /* if defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER) */
