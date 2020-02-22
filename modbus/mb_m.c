/****************************************************************************
 * FreeModbus Library: A portable Modbus implementation for Modbus ASCII/RTU.
 *
 *   Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *   All rights reserved.
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
#include <stdlib.h>
#include <string.h>

#include "port.h"

#include "modbus/mb.h"
#include "modbus/mb_m.h"
#include "modbus/mbframe.h"
#include "modbus/mbproto.h"
#include "modbus/mbfunc.h"

#include "modbus/mbport.h"

#ifdef CONFIG_MB_RTU_MASTER
#  include "mbrtu_m.h"
#endif

#ifdef CONFIG_MB_ASCII_MASTER
#  include "mbascii.h"
#endif

#ifdef CONFIG_MB_TCP_MASTER
#  include "mbtcp.h"
#endif

#if defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER)

#ifndef CONFIG_MB_PORT_HAS_CLOSE
#  define MB_PORT_HAS_CLOSE 0
#else
#  define MB_PORT_HAS_CLOSE 1
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t ucMBMasterDestAddress;
static bool xMBRunInMasterMode = false;
static eMBMasterErrorEventType eMBMasterCurErrorType;

static enum
{
  STATE_ENABLED,
  STATE_DISABLED,
  STATE_NOT_INITIALIZED
} eMBState = STATE_NOT_INITIALIZED;

/* Functions pointer which are initialized in eMBInit( ). Depending on the
 * mode (RTU or ASCII) the are set to the correct implementations.
 * Using for Modbus Master,Add by Armink 20130813
 */

static peMBFrameSend peMBMasterFrameSendCur;
static pvMBFrameStart pvMBMasterFrameStartCur;
static pvMBFrameStop pvMBMasterFrameStopCur;
static peMBFrameReceive peMBMasterFrameReceiveCur;
static pvMBFrameClose pvMBMasterFrameCloseCur;

/* Callback functions required by the porting layer. They are called when
 * an external event has happened which includes a timeout or the reception
 * or transmission of a character.
 * Using for Modbus Master,Add by Armink 20130813
 */

bool(*pxMBMasterFrameCBByteReceived) (void);
bool(*pxMBMasterFrameCBTransmitterEmpty) (void);
bool(*pxMBMasterPortCBTimerExpired) (void);

bool(*pxMBMasterFrameCBReceiveFSMCur) (void);
bool(*pxMBMasterFrameCBTransmitFSMCur) (void);

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */

static xMBFunctionHandler xMasterFuncHandlers[CONFIG_MB_FUNC_HANDLERS_MAX] = {
#ifdef CONFIG_MB_FUNC_OTHER_REP_SLAVEID_ENABLED

  /* TODO Add Master function define */

  {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#ifdef CONFIG_MB_MASTER_FUNC_READ_INPUT_ENABLED
  {MB_FUNC_READ_INPUT_REGISTER, eMBMasterFuncReadInputRegister},
#endif
#ifdef CONFIG_MB_MASTER_FUNC_READ_HOLDING_ENABLED
  {MB_FUNC_READ_HOLDING_REGISTER, eMBMasterFuncReadHoldingRegister},
#endif
#ifdef CONFIG_MB_MASTER_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED
  {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBMasterFuncWriteMultipleHoldingRegister},
#endif
#ifdef CONFIG_MB_MASTER_FUNC_WRITE_HOLDING_ENABLED
  {MB_FUNC_WRITE_REGISTER, eMBMasterFuncWriteHoldingRegister},
#endif
#ifdef CONFIG_MB_MASTER_FUNC_READWRITE_HOLDING_ENABLED
  {MB_FUNC_READWRITE_MULTIPLE_REGISTERS,
   eMBMasterFuncReadWriteMultipleHoldingRegister},
#endif
#ifdef CONFIG_MB_MASTER_FUNC_READ_COILS_ENABLED
  {MB_FUNC_READ_COILS, eMBMasterFuncReadCoils},
#endif
#ifdef CONFIG_MB_MASTER_FUNC_WRITE_COIL_ENABLED
  {MB_FUNC_WRITE_SINGLE_COIL, eMBMasterFuncWriteCoil},
#endif
#ifdef CONFIG_MB_MASTER_FUNC_WRITE_MULTIPLE_COILS_ENABLED
  {MB_FUNC_WRITE_MULTIPLE_COILS, eMBMasterFuncWriteMultipleCoils},
#endif
#ifdef CONFIG_MB_MASTER_FUNC_READ_DISCRETE_INPUTS_ENABLED
  {MB_FUNC_READ_DISCRETE_INPUTS, eMBMasterFuncReadDiscreteInputs},
#endif
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

eMBErrorCode eMBMasterInit(eMBMode eMode, uint8_t ucPort,
                           speed_t ulBaudRate, eMBParity eParity)
{
  eMBErrorCode eStatus = MB_ENOERR;

  switch (eMode)
    {
#ifdef CONFIG_MB_RTU_MASTER
    case MB_RTU:
      pvMBMasterFrameStartCur = eMBMasterRTUStart;
      pvMBMasterFrameStopCur = eMBMasterRTUStop;
      peMBMasterFrameSendCur = eMBMasterRTUSend;
      peMBMasterFrameReceiveCur = eMBMasterRTUReceive;
      pvMBMasterFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBMasterPortClose : NULL;
      pxMBMasterFrameCBByteReceived = xMBMasterRTUReceiveFSM;
      pxMBMasterFrameCBTransmitterEmpty = xMBMasterRTUTransmitFSM;
      pxMBMasterPortCBTimerExpired = xMBMasterRTUTimerExpired;

      eStatus = eMBMasterRTUInit(ucPort, ulBaudRate, eParity);
      break;
#endif
#ifdef CONFIG_MB_ASCII_MASTER
    case MB_ASCII:
      pvMBMasterFrameStartCur = eMBMasterASCIIStart;
      pvMBMasterFrameStopCur = eMBMasterASCIIStop;
      peMBMasterFrameSendCur = eMBMasterASCIISend;
      peMBMasterFrameReceiveCur = eMBMasterASCIIReceive;
      pvMBMasterFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBMasterPortClose : NULL;
      pxMBMasterFrameCBByteReceived = xMBMasterASCIIReceiveFSM;
      pxMBMasterFrameCBTransmitterEmpty = xMBMasterASCIITransmitFSM;
      pxMBMasterPortCBTimerExpired = xMBMasterASCIITimerT1SExpired;

      eStatus = eMBMasterASCIIInit(ucPort, ulBaudRate, eParity);
      break;
#endif
    default:
      eStatus = MB_EINVAL;
      break;
    }

  if (eStatus == MB_ENOERR)
    {
      if (!xMBMasterPortEventInit())
        {
          /* Port dependent event module initialization failed. */

          eStatus = MB_EPORTERR;
        }
      else
        {
          eMBState = STATE_DISABLED;
        }

      /* Initialize the OS resource for modbus master. */

      vMBMasterOsResInit();
    }

  return eStatus;
}

eMBErrorCode eMBMasterClose(void)
{
  eMBErrorCode eStatus = MB_ENOERR;

  if (eMBState == STATE_DISABLED)
    {
      if (pvMBMasterFrameCloseCur != NULL)
        {
          pvMBMasterFrameCloseCur();
        }
    }
  else
    {
      eStatus = MB_EILLSTATE;
    }

  return eStatus;
}

eMBErrorCode eMBMasterEnable(void)
{
  eMBErrorCode eStatus = MB_ENOERR;

  if (eMBState == STATE_DISABLED)
    {
      /* Activate the protocol stack. */

      pvMBMasterFrameStartCur();
      eMBState = STATE_ENABLED;
    }
  else
    {
      eStatus = MB_EILLSTATE;
    }

  return eStatus;
}

eMBErrorCode eMBMasterDisable(void)
{
  eMBErrorCode eStatus;

  if (eMBState == STATE_ENABLED)
    {
      pvMBMasterFrameStopCur();
      eMBState = STATE_DISABLED;
      eStatus = MB_ENOERR;
    }
  else if (eMBState == STATE_DISABLED)
    {
      eStatus = MB_ENOERR;
    }
  else
    {
      eStatus = MB_EILLSTATE;
    }

  return eStatus;
}

eMBErrorCode eMBMasterPoll(void)
{
  static uint8_t *ucMBFrame;
  static uint8_t ucRcvAddress;
  static uint8_t ucFunctionCode;
  static uint16_t usLength;
  static eMBException eException;
  int i;
  int j;

  eMBErrorCode eStatus = MB_ENOERR;
  eMBMasterEventType eEvent;
  eMBMasterErrorEventType errorType;

  /* Check if the protocol stack is ready. */

  if (eMBState != STATE_ENABLED)
    {
      return MB_EILLSTATE;
    }

  /* Check if there is a event available. If not return control to caller.
   * Otherwise we will handle the event.
   */

  if (xMBMasterPortEventGet(&eEvent) == true)
    {
      switch (eEvent)
        {
        case EV_MASTER_READY:
          break;

        case EV_MASTER_FRAME_RECEIVED:
          eStatus =
            peMBMasterFrameReceiveCur(&ucRcvAddress, &ucMBFrame, &usLength);

          /* Check if the frame is for us. If not, send an error process event. */

          if ((eStatus == MB_ENOERR) &&
              (ucRcvAddress == ucMBMasterGetDestAddress()))
            {
              xMBMasterPortEventPost(EV_MASTER_EXECUTE);
            }
          else
            {
              vMBMasterSetErrorType(EV_ERROR_RECEIVE_DATA);
              xMBMasterPortEventPost(EV_MASTER_ERROR_PROCESS);
            }
          break;

        case EV_MASTER_EXECUTE:
          ucFunctionCode = ucMBFrame[MB_PDU_FUNC_OFF];
          eException = MB_EX_ILLEGAL_FUNCTION;

          /* If receive frame has exception. The receive function code highest
           * bit is 1.
           */

          if (ucFunctionCode >> 7)
            {
              eException = (eMBException) ucMBFrame[MB_PDU_DATA_OFF];
            }
          else
            {
              for (i = 0; i < CONFIG_MB_FUNC_HANDLERS_MAX; i++)
                {
                  /* No more function handlers registered. Abort. */

                  if (xMasterFuncHandlers[i].ucFunctionCode == 0)
                    {
                      break;
                    }
                  else if (xMasterFuncHandlers[i].ucFunctionCode ==
                           ucFunctionCode)
                    {
                      vMBMasterSetCBRunInMasterMode(true);

                      /* If master request is broadcast, the master need
                       * execute function for all slave. */

                      if (xMBMasterRequestIsBroadcast())
                        {
                          usLength = usMBMasterGetPDUSndLength();
                          for (j = 1; j <= CONFIG_MB_MASTER_TOTAL_SLAVE_NUM;
                               j++)
                            {
                              vMBMasterSetDestAddress(j);
                              eException =
                                xMasterFuncHandlers[i].pxHandler(ucMBFrame,
                                                                 &usLength);
                            }
                        }
                      else
                        {
                          eException =
                            xMasterFuncHandlers[i].pxHandler(ucMBFrame,
                                                             &usLength);
                        }

                      vMBMasterSetCBRunInMasterMode(false);
                      break;
                    }
                }
            }

          /* If master has exception, Master will send error process. Otherwise
           * the Master is idle.
           */

          if (eException != MB_EX_NONE)
            {
              vMBMasterSetErrorType(EV_ERROR_EXECUTE_FUNCTION);
              xMBMasterPortEventPost(EV_MASTER_ERROR_PROCESS);
            }
          else
            {
              vMBMasterCBRequestSuccess();
              vMBMasterRunResRelease();
            }
          break;

        case EV_MASTER_FRAME_SENT:

          /* Master is busy now. */

          vMBMasterGetPDUSndBuf(&ucMBFrame);
          eStatus =
            peMBMasterFrameSendCur(ucMBMasterGetDestAddress(), ucMBFrame,
                                   usMBMasterGetPDUSndLength());
          break;

        case EV_MASTER_ERROR_PROCESS:

          /* Execute specified error process callback function. */

          errorType = eMBMasterGetErrorType();
          vMBMasterGetPDUSndBuf(&ucMBFrame);
          switch (errorType)
            {
            case EV_ERROR_RESPOND_TIMEOUT:
              vMBMasterErrorCBRespondTimeout(ucMBMasterGetDestAddress(),
                                             ucMBFrame,
                                             usMBMasterGetPDUSndLength());
              break;
            case EV_ERROR_RECEIVE_DATA:
              vMBMasterErrorCBReceiveData(ucMBMasterGetDestAddress(),
                                          ucMBFrame,
                                          usMBMasterGetPDUSndLength());
              break;
            case EV_ERROR_EXECUTE_FUNCTION:
              vMBMasterErrorCBExecuteFunction(ucMBMasterGetDestAddress(),
                                              ucMBFrame,
                                              usMBMasterGetPDUSndLength());
              break;
            }

          vMBMasterRunResRelease();
          break;
        }
    }

  return MB_ENOERR;
}

/* Get whether the Modbus Master is run in master mode.*/

bool xMBMasterGetCBRunInMasterMode(void)
{
  return xMBRunInMasterMode;
}

/* Set whether the Modbus Master is run in master mode.*/

void vMBMasterSetCBRunInMasterMode(bool IsMasterMode)
{
  xMBRunInMasterMode = IsMasterMode;
}

/* Get Modbus Master send destination address. */

uint8_t ucMBMasterGetDestAddress(void)
{
  return ucMBMasterDestAddress;
}

/* Set Modbus Master send destination address. */

void vMBMasterSetDestAddress(uint8_t Address)
{
  ucMBMasterDestAddress = Address;
}

/* Get Modbus Master current error event type. */

eMBMasterErrorEventType eMBMasterGetErrorType(void)
{
  return eMBMasterCurErrorType;
}

/* Set Modbus Master current error event type. */

void vMBMasterSetErrorType(eMBMasterErrorEventType errorType)
{
  eMBMasterCurErrorType = errorType;
}

#endif /* defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER) */
