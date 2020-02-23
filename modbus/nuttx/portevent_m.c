/****************************************************************************
 * apps/modbus/nuttx/portevent_m.c
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

#include "modbus/mb.h"
#include "modbus/mb_m.h"
#include "modbus/mbport.h"
#include <sys/time.h>
#include <semaphore.h>
#include <mqueue.h>
#include <errno.h>

#include "port.h"

#if defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WAITER_EVENTS (EV_MASTER_PROCESS_SUCCESS           \
                       | EV_MASTER_ERROR_RESPOND_TIMEOUT   \
                       | EV_MASTER_ERROR_RECEIVE_DATA      \
                       | EV_MASTER_ERROR_EXECUTE_FUNCTION)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t bussysem;
static sem_t waitersem;
static eMBMasterEventType eQueuedEvent;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool xMBMasterPortEventInit(void)
{
  /* Initialize semaphore for waiter */

  sem_init(&waitersem, 0, 0);

  /* No event in queue */

  eQueuedEvent = 0;

  return true;
}

bool xMBMasterPortEventPost(eMBMasterEventType eEvent)
{
  /* Post waiter sem, if event belongs to one of waiter events */

  if (eEvent & WAITER_EVENTS)
    {
      sem_post(&waitersem);
    }

  eQueuedEvent |= eEvent;

  return true;
}

bool xMBMasterPortEventGet(eMBMasterEventType * eEvent)
{
  bool xEventHappened = false;

  *eEvent = 0;

  if (eQueuedEvent & ~(WAITER_EVENTS))
    {

      /* Fetch events by priority */

      if (eQueuedEvent & EV_MASTER_READY)
        {
          *eEvent = EV_MASTER_READY;
        }
      else if (eQueuedEvent & EV_MASTER_FRAME_RECEIVED)
        {
          *eEvent = EV_MASTER_FRAME_RECEIVED;
        }
      else if (eQueuedEvent & EV_MASTER_EXECUTE)
        {
          *eEvent = EV_MASTER_EXECUTE;
        }
      else if (eQueuedEvent & EV_MASTER_FRAME_SENT)
        {
          *eEvent = EV_MASTER_FRAME_SENT;
        }
      else if (eQueuedEvent & EV_MASTER_FRAME_SENT)
        {
          *eEvent = EV_MASTER_FRAME_SENT;
        }
      else if (eQueuedEvent & EV_MASTER_ERROR_PROCESS)
        {
          *eEvent = EV_MASTER_ERROR_PROCESS;
        }

      eQueuedEvent &= ~(*eEvent);
      xEventHappened = true;
    }
  else
    {
      /* Poll the serial device. The serial device timeouts if no characters
       * have been received within for t3.5 during an active transmission or if
       * nothing happens within a specified amount of time. Both timeouts are
       * configured from the timer init functions.
       */

      xMBMasterPortSerialPoll();

      /* Check if any of the timers have expired. */

      vMBMasterPortTimerPoll();
    }

  return xEventHappened;
}

/* This function should init Modbus Master running OS resource */

void vMBMasterOsResInit(void)
{
  int res;

  if ((res = sem_init(&bussysem, 0, 0)) != OK)
    {
      vMBPortLog(MB_LOG_ERROR,
                 "EVENT-INIT",
                 "Can't initialize locking semaphore. Err: %d\n", res);
    }

  sem_post(&bussysem);
}

/* This function should take Modbus Master running resource.
 * Note: The resource is defined by Operating System. If you do not use OS,
 *   this function can just return true.
 *
 * Input Parameters:
 *   ulTimeOut the waiting time
 *
 * Returned Value:
 *   resource taken result
 */

bool xMBMasterRunResTake(int32_t lTimeOut)
{
  struct timespec time;

  if (lTimeOut == -1)
    {
      if (sem_wait(&bussysem) != OK)
        {
          return false;
        }
      return true;
    }
  else
    {
      time.tv_sec = 0;
      time.tv_nsec = lTimeOut * 1000;   /* convert to nano seconds */
      if (sem_timedwait(&bussysem, &time) != OK)
        {
          return false;
        }
      return true;
    }
}

/* This function should release Modbus Master running resource.
 * NOTE: The resource is defined by Operating System. If you do not use OS,
 * this function can just return true.
 */

void vMBMasterRunResRelease(void)
{
  if (sem_post(&bussysem) != OK)
    {
      vMBPortLog(MB_LOG_ERROR,
                 "RUN-RES-RELEASE",
                 "Failed to release Modbus Master OS resource\n");
    }
}

/* This is Modbus Master Respond Timeout error callback function.
 * NOTE: This function will block modbus master poll.
 *
 * Input Parameters:
 *   ucDestAddress destination slave address
 *   pucPDUData PDU buffer data
 *   ucPDULength PDU buffer length
 */

void vMBMasterErrorCBRespondTimeout(uint8_t ucDestAddress,
                                    const uint8_t * pucPDUData,
                                    uint16_t usPDULength)
{
  xMBMasterPortEventPost(EV_MASTER_ERROR_RESPOND_TIMEOUT);
}

/* This is Modbus Master receive data error callback function.
 * NOTE: This function will block modbus master poll.
 *
 * Input Parameters:
 *   ucDestAddress destination slave address
 *   pucPDUData PDU buffer data
 *   usPDULength PDU buffer length
 */

void vMBMasterErrorCBReceiveData(uint8_t ucDestAddress,
                                 const uint8_t * pudPDUData,
                                 uint16_t usPDULength)
{
  xMBMasterPortEventPost(EV_MASTER_ERROR_RECEIVE_DATA);
}

/* This is Modbus Master execute function error callback function.
 * NOTE: This function will block modbus master poll.
 *
 * Input Parameters:
 *   ucDestAddress destination slave address
 *   pucPDUData PDU buffer data
 *   usPDULength PDU buffer length
 */

void vMBMasterErrorCBExecuteFunction(uint8_t ucDestAddress,
                                     const uint8_t * pucPDUData,
                                     uint16_t usPDULength)
{
  xMBMasterPortEventPost(EV_MASTER_ERROR_EXECUTE_FUNCTION);
}

/* This is Modbus Master execute function success callback function.
 * NOTE: This function will block modbus master poll.
 */

void vMBMasterCBRequestSuccess(void)
{
  xMBMasterPortEventPost(EV_MASTER_PROCESS_SUCCESS);
}

/* This function will wait for Modbus Master request finish and return result.
 */

eMBMasterReqErrCode eMBMasterWaitRequestFinish(void)
{
  eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;

  /* wait forever for OS event */

  sem_wait(&waitersem);

  if (eQueuedEvent & WAITER_EVENTS)
    {
      if (eQueuedEvent & EV_MASTER_PROCESS_SUCCESS)
        {
          /* Do nothing */
        }
      else if (eQueuedEvent & EV_MASTER_ERROR_RESPOND_TIMEOUT)
        {
          eErrStatus = MB_MRE_TIMEDOUT;
        }
      else if (eQueuedEvent & EV_MASTER_ERROR_RECEIVE_DATA)
        {
          eErrStatus = MB_MRE_REV_DATA;
        }
      else if (eQueuedEvent & EV_MASTER_ERROR_EXECUTE_FUNCTION)
        {
          eErrStatus = MB_MRE_EXE_FUN;
        }
      eQueuedEvent &= ~WAITER_EVENTS;
    }

  return eErrStatus;
}

#endif /* defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER) */
