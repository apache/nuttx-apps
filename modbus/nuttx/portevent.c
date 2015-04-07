/****************************************************************************
 * apps/modbus/nuttx/portevent.c
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

#include <apps/modbus/mb.h>
#include <apps/modbus/mbport.h>

#include "port.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static eMBEventType eQueuedEvent;
static bool xEventInQueue;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool xMBPortEventInit(void)
{
  xEventInQueue = false;
  return true;
}

bool xMBPortEventPost(eMBEventType eEvent)
{
  xEventInQueue = true;
  eQueuedEvent = eEvent;
  return true;
}

bool xMBPortEventGet(eMBEventType * eEvent)
{
  bool xEventHappened = false;

  if (xEventInQueue)
    {
      *eEvent = eQueuedEvent;
      xEventInQueue = false;
      xEventHappened = true;
    }
  else
    {
      /* Poll the serial device. The serial device timeouts if no
       * characters have been received within for t3.5 during an
       * active transmission or if nothing happens within a specified
       * amount of time. Both timeouts are configured from the timer
       * init functions.
       */

      (void)xMBPortSerialPoll();

      /* Check if any of the timers have expired. */

      vMBPortTimerPoll();
    }

  return xEventHappened;
}

