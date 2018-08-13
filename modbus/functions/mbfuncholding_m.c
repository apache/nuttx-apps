/****************************************************************************
 * apps/modbus/functions/mbfuncholding_m.c
 *
 * FreeModbus Library: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
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
#include <stdlib.h>
#include <string.h>

#include "port.h"

#include "modbus/mb.h"
#include "modbus/mb_m.h"
#include "modbus/mbframe.h"
#include "modbus/mbproto.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MB_PDU_REQ_READ_ADDR_OFF                (MB_PDU_DATA_OFF + 0)
#define MB_PDU_REQ_READ_REGCNT_OFF              (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_READ_SIZE                    (4)
#define MB_PDU_FUNC_READ_REGCNT_MAX             (0x007D)
#define MB_PDU_FUNC_READ_BYTECNT_OFF            (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_READ_VALUES_OFF             (MB_PDU_DATA_OFF + 1)
#define MB_PDU_FUNC_READ_SIZE_MIN               (1)

#define MB_PDU_REQ_WRITE_ADDR_OFF               (MB_PDU_DATA_OFF + 0)
#define MB_PDU_REQ_WRITE_VALUE_OFF              (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_WRITE_SIZE                   (4)
#define MB_PDU_FUNC_WRITE_ADDR_OFF              (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_WRITE_VALUE_OFF             (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_SIZE                  (4)

#define MB_PDU_REQ_WRITE_MUL_ADDR_OFF           (MB_PDU_DATA_OFF + 0)
#define MB_PDU_REQ_WRITE_MUL_REGCNT_OFF         (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_WRITE_MUL_BYTECNT_OFF        (MB_PDU_DATA_OFF + 4)
#define MB_PDU_REQ_WRITE_MUL_VALUES_OFF         (MB_PDU_DATA_OFF + 5)
#define MB_PDU_REQ_WRITE_MUL_SIZE_MIN           (5)
#define MB_PDU_REQ_WRITE_MUL_REGCNT_MAX         (0x0078)
#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF          (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF        (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_MUL_SIZE              (4)

#define MB_PDU_REQ_READWRITE_READ_ADDR_OFF      (MB_PDU_DATA_OFF + 0)
#define MB_PDU_REQ_READWRITE_READ_REGCNT_OFF    (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_READWRITE_WRITE_ADDR_OFF     (MB_PDU_DATA_OFF + 4)
#define MB_PDU_REQ_READWRITE_WRITE_REGCNT_OFF   (MB_PDU_DATA_OFF + 6)
#define MB_PDU_REQ_READWRITE_WRITE_BYTECNT_OFF  (MB_PDU_DATA_OFF + 8)
#define MB_PDU_REQ_READWRITE_WRITE_VALUES_OFF   (MB_PDU_DATA_OFF + 9)
#define MB_PDU_REQ_READWRITE_SIZE_MIN           (9)
#define MB_PDU_FUNC_READWRITE_READ_BYTECNT_OFF  (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_READWRITE_READ_VALUES_OFF   (MB_PDU_DATA_OFF + 1)
#define MB_PDU_FUNC_READWRITE_SIZE_MIN          (1)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

eMBException prveMBError2Exception(eMBErrorCode eErrorCode);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#if defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER)

/****************************************************************************
 * Description:
 *   This function will request write holding register.
 *
 * Input Parameters:
 *   ucSndAddr salve address
 *   usRegAddr register start address
 *   usRegData register data to be written
 *   lTimeOut timeout (-1 will waiting forever)
 *
 * Returned Value:
 *   error code
 *
 ****************************************************************************/

#ifdef CONFIG_MB_MASTER_FUNC_WRITE_HOLDING_ENABLED
eMBMasterReqErrCode eMBMasterReqWriteHoldingRegister(uint8_t ucSndAddr,
                                                     uint16_t usRegAddr,
                                                     uint16_t usRegData,
                                                     uint32_t lTimeOut)
{
  uint8_t *ucMBFrame;
  eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;

  if (ucSndAddr > CONFIG_MB_MASTER_TOTAL_SLAVE_NUM)
    {
      eErrStatus = MB_MRE_ILL_ARG;
    }
  else if (xMBMasterRunResTake(lTimeOut) == false)
    {
      eErrStatus = MB_MRE_MASTER_BUSY;
    }
  else
    {
      vMBMasterGetPDUSndBuf(&ucMBFrame);
      vMBMasterSetDestAddress(ucSndAddr);
      ucMBFrame[MB_PDU_FUNC_OFF] = MB_FUNC_WRITE_REGISTER;
      ucMBFrame[MB_PDU_REQ_WRITE_ADDR_OFF] = usRegAddr >> 8;
      ucMBFrame[MB_PDU_REQ_WRITE_ADDR_OFF + 1] = usRegAddr;
      ucMBFrame[MB_PDU_REQ_WRITE_VALUE_OFF] = usRegData >> 8;
      ucMBFrame[MB_PDU_REQ_WRITE_VALUE_OFF + 1] = usRegData;
      vMBMasterSetPDUSndLength(MB_PDU_SIZE_MIN + MB_PDU_REQ_WRITE_SIZE);
      (void)xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
      eErrStatus = eMBMasterWaitRequestFinish();
    }

  return eErrStatus;
}

eMBException eMBMasterFuncWriteHoldingRegister(uint8_t *pucFrame,
                                               uint16_t *usLen)
{
  uint16_t usRegAddress;
  eMBException eStatus = MB_EX_NONE;
  eMBErrorCode eRegStatus;

  if (*usLen == (MB_PDU_SIZE_MIN + MB_PDU_FUNC_WRITE_SIZE))
    {
      usRegAddress = (uint16_t) (pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8);
      usRegAddress |= (uint16_t) (pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]);
      usRegAddress++;

      /* Make callback to update the value. */

      eRegStatus = eMBMasterRegHoldingCB(&pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF],
                                         usRegAddress, 1, MB_REG_WRITE);

      /* If an error occured convert it into a Modbus exception. */

      if (eRegStatus != MB_ENOERR)
        {
          eStatus = prveMBError2Exception(eRegStatus);
        }
    }
  else
    {
      /* Can't be a valid request because the length is incorrect. */

      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
  return eStatus;
}
#endif

/****************************************************************************
 * This function will request write multiple holding register.
 *
 * Input Parameters:
 *   ucSndAddr salve address
 *   usRegAddr register start address
 *   usNRegs register total number
 *   pusDataBuffer data to be written
 *   lTimeOut timeout (-1 will waiting forever)
 *
 * Returned Value:
 *   error code
 *
 ****************************************************************************/

#ifdef CONFIG_MB_MASTER_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED
eMBMasterReqErrCode
  eMBMasterReqWriteMultipleHoldingRegister(uint8_t ucSndAddr,
                                           uint16_t usRegAddr,
                                           uint16_t usNRegs,
                                           uint16_t *pusDataBuffer,
                                           uint32_t lTimeOut)
{
  uint8_t *ucMBFrame;
  uint16_t usRegIndex = 0;
  eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;

  if (ucSndAddr > CONFIG_MB_MASTER_TOTAL_SLAVE_NUM)
    {
      eErrStatus = MB_MRE_ILL_ARG;
    }
  else if (xMBMasterRunResTake(lTimeOut) == false)
    {
      eErrStatus = MB_MRE_MASTER_BUSY;
    }
  else
    {
      vMBMasterGetPDUSndBuf(&ucMBFrame);
      vMBMasterSetDestAddress(ucSndAddr);
      ucMBFrame[MB_PDU_FUNC_OFF] = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
      ucMBFrame[MB_PDU_REQ_WRITE_MUL_ADDR_OFF] = usRegAddr >> 8;
      ucMBFrame[MB_PDU_REQ_WRITE_MUL_ADDR_OFF + 1] = usRegAddr;
      ucMBFrame[MB_PDU_REQ_WRITE_MUL_REGCNT_OFF] = usNRegs >> 8;
      ucMBFrame[MB_PDU_REQ_WRITE_MUL_REGCNT_OFF + 1] = usNRegs;
      ucMBFrame[MB_PDU_REQ_WRITE_MUL_BYTECNT_OFF] = usNRegs * 2;
      ucMBFrame += MB_PDU_REQ_WRITE_MUL_VALUES_OFF;

      while (usNRegs > usRegIndex)
        {
          *ucMBFrame++ = pusDataBuffer[usRegIndex] >> 8;
          *ucMBFrame++ = pusDataBuffer[usRegIndex++];
        }

      vMBMasterSetPDUSndLength(MB_PDU_SIZE_MIN + MB_PDU_REQ_WRITE_MUL_SIZE_MIN +
                               2 * usNRegs);
      (void)xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
      eErrStatus = eMBMasterWaitRequestFinish();
    }

  return eErrStatus;
}

eMBException eMBMasterFuncWriteMultipleHoldingRegister(uint8_t *pucFrame,
                                                       uint16_t *usLen)
{
  uint8_t *ucMBFrame;
  uint16_t usRegAddress;
  uint16_t usRegCount;
  uint8_t ucRegByteCount;

  eMBException eStatus = MB_EX_NONE;
  eMBErrorCode eRegStatus;

  /* If this request is broadcast, the *usLen is not need check. */

  if ((*usLen == MB_PDU_SIZE_MIN + MB_PDU_FUNC_WRITE_MUL_SIZE) ||
      xMBMasterRequestIsBroadcast())
    {
      vMBMasterGetPDUSndBuf(&ucMBFrame);
      usRegAddress = (uint16_t) (ucMBFrame[MB_PDU_REQ_WRITE_MUL_ADDR_OFF] << 8);
      usRegAddress |= (uint16_t) (ucMBFrame[MB_PDU_REQ_WRITE_MUL_ADDR_OFF + 1]);
      usRegAddress++;

      usRegCount = (uint16_t) (ucMBFrame[MB_PDU_REQ_WRITE_MUL_REGCNT_OFF] << 8);
      usRegCount |= (uint16_t) (ucMBFrame[MB_PDU_REQ_WRITE_MUL_REGCNT_OFF + 1]);

      ucRegByteCount = ucMBFrame[MB_PDU_REQ_WRITE_MUL_BYTECNT_OFF];

      if (ucRegByteCount == 2 * usRegCount)
        {
          /* Make callback to update the register values. */

          eRegStatus =
            eMBMasterRegHoldingCB(&ucMBFrame[MB_PDU_REQ_WRITE_MUL_VALUES_OFF],
                                  usRegAddress, usRegCount, MB_REG_WRITE);

          /* If an error occured convert it into a Modbus exception. */

          if (eRegStatus != MB_ENOERR)
            {
              eStatus = prveMBError2Exception(eRegStatus);
            }
        }
      else
        {
          eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
  else
    {
      /* Can't be a valid request because the length is incorrect. */

      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }

  return eStatus;
}
#endif

/****************************************************************************
 * This function will request read holding register.
 *
 * Input Parameters:
 *   ucSndAddr salve address
 *   usRegAddr register start address
 *   usNRegs register total number
 *   lTimeOut timeout (-1 will waiting forever)
 *
 * Returned Value:
 *   error code
 *
 ****************************************************************************/

#ifdef CONFIG_MB_MASTER_FUNC_READ_HOLDING_ENABLED
eMBMasterReqErrCode eMBMasterReqReadHoldingRegister(uint8_t ucSndAddr,
                                                    uint16_t usRegAddr,
                                                    uint16_t usNRegs,
                                                    uint32_t lTimeOut)
{
  uint8_t *ucMBFrame;
  eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;

  if (ucSndAddr > CONFIG_MB_MASTER_TOTAL_SLAVE_NUM)
    {
      eErrStatus = MB_MRE_ILL_ARG;
    }
  else if (xMBMasterRunResTake(lTimeOut) == false)
    {
      eErrStatus = MB_MRE_MASTER_BUSY;
    }
  else
    {
      vMBMasterGetPDUSndBuf(&ucMBFrame);
      vMBMasterSetDestAddress(ucSndAddr);
      ucMBFrame[MB_PDU_FUNC_OFF] = MB_FUNC_READ_HOLDING_REGISTER;
      ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF] = usRegAddr >> 8;
      ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1] = usRegAddr;
      ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF] = usNRegs >> 8;
      ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF + 1] = usNRegs;
      vMBMasterSetPDUSndLength(MB_PDU_SIZE_MIN + MB_PDU_REQ_READ_SIZE);
      (void)xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
      eErrStatus = eMBMasterWaitRequestFinish();
    }

  return eErrStatus;
}

eMBException eMBMasterFuncReadHoldingRegister(uint8_t *pucFrame,
                                              uint16_t *usLen)
{
  uint8_t *ucMBFrame;
  uint16_t usRegAddress;
  uint16_t usRegCount;

  eMBException eStatus = MB_EX_NONE;
  eMBErrorCode eRegStatus;

  /* If this request is broadcast, and it's read mode. This request don't need
   * execute.
   */

  if (xMBMasterRequestIsBroadcast())
    {
      eStatus = MB_EX_NONE;
    }
  else if (*usLen >= MB_PDU_SIZE_MIN + MB_PDU_FUNC_READ_SIZE_MIN)
    {
      vMBMasterGetPDUSndBuf(&ucMBFrame);
      usRegAddress = (uint16_t) (ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF] << 8);
      usRegAddress |= (uint16_t) (ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1]);
      usRegAddress++;

      usRegCount = (uint16_t) (ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF] << 8);
      usRegCount |= (uint16_t) (ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF + 1]);

      /* Check if the number of registers to read is valid. If not return
       * Modbus illegal data value exception.
       */

      if ((usRegCount >= 1) &&
          (2 * usRegCount == pucFrame[MB_PDU_FUNC_READ_BYTECNT_OFF]))
        {
          /* Make callback to fill the buffer. */

          eRegStatus =
            eMBMasterRegHoldingCB(&pucFrame[MB_PDU_FUNC_READ_VALUES_OFF],
                                  usRegAddress, usRegCount, MB_REG_READ);

          /* If an error occurred convert it into a Modbus exception. */

          if (eRegStatus != MB_ENOERR)
            {
              eStatus = prveMBError2Exception(eRegStatus);
            }
        }
      else
        {
          eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
  else
    {
      /* Can't be a valid request because the length is incorrect. */

      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
  return eStatus;
}

#endif

/****************************************************************************
 * Description:
 *   This function will request read and write holding register.
 *
 * Input Parameters:
 *   ucSndAddr salve address
 *   usReadRegAddr read register start address
 *   usNReadRegs read register total number
 *   pusDataBuffer data to be written
 *   usWriteRegAddr write register start address
 *   usNWriteRegs write register total number
 *   lTimeOut timeout (-1 will waiting forever)
 *
 * Returned Value:
 *   error code
 *
 ****************************************************************************/

#ifdef CONFIG_MB_MASTER_FUNC_READWRITE_HOLDING_ENABLED
eMBMasterReqErrCode
  eMBMasterReqReadWriteMultipleHoldingRegister(uint8_t ucSndAddr,
                                               uint16_t usReadRegAddr,
                                               uint16_t usNReadRegs,
                                               uint16_t *pusDataBuffer,
                                               uint16_t usWriteRegAddr,
                                               uint16_t usNWriteRegs,
                                               uint32_t lTimeOut)
{
  uint8_t *ucMBFrame;
  uint16_t usRegIndex = 0;
  eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;

  if (ucSndAddr > CONFIG_MB_MASTER_TOTAL_SLAVE_NUM)
    {
      eErrStatus = MB_MRE_ILL_ARG;
    }
  else if (xMBMasterRunResTake(lTimeOut) == false)
    {
      eErrStatus = MB_MRE_MASTER_BUSY;
    }
  else
    {
      vMBMasterGetPDUSndBuf(&ucMBFrame);
      vMBMasterSetDestAddress(ucSndAddr);
      ucMBFrame[MB_PDU_FUNC_OFF] = MB_FUNC_READWRITE_MULTIPLE_REGISTERS;
      ucMBFrame[MB_PDU_REQ_READWRITE_READ_ADDR_OFF] = usReadRegAddr >> 8;
      ucMBFrame[MB_PDU_REQ_READWRITE_READ_ADDR_OFF + 1] = usReadRegAddr;
      ucMBFrame[MB_PDU_REQ_READWRITE_READ_REGCNT_OFF] = usNReadRegs >> 8;
      ucMBFrame[MB_PDU_REQ_READWRITE_READ_REGCNT_OFF + 1] = usNReadRegs;
      ucMBFrame[MB_PDU_REQ_READWRITE_WRITE_ADDR_OFF] = usWriteRegAddr >> 8;
      ucMBFrame[MB_PDU_REQ_READWRITE_WRITE_ADDR_OFF + 1] = usWriteRegAddr;
      ucMBFrame[MB_PDU_REQ_READWRITE_WRITE_REGCNT_OFF] = usNWriteRegs >> 8;
      ucMBFrame[MB_PDU_REQ_READWRITE_WRITE_REGCNT_OFF + 1] = usNWriteRegs;
      ucMBFrame[MB_PDU_REQ_READWRITE_WRITE_BYTECNT_OFF] = usNWriteRegs * 2;
      ucMBFrame += MB_PDU_REQ_READWRITE_WRITE_VALUES_OFF;

      while (usNWriteRegs > usRegIndex)
        {
          *ucMBFrame++ = pusDataBuffer[usRegIndex] >> 8;
          *ucMBFrame++ = pusDataBuffer[usRegIndex++];
        }

      vMBMasterSetPDUSndLength(MB_PDU_SIZE_MIN + MB_PDU_REQ_READWRITE_SIZE_MIN +
                               2 * usNWriteRegs);
      (void)xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
      eErrStatus = eMBMasterWaitRequestFinish();
    }

  return eErrStatus;
}

eMBException eMBMasterFuncReadWriteMultipleHoldingRegister(uint8_t *pucFrame,
                                                           uint16_t *usLen)
{
  uint16_t usRegReadAddress;
  uint16_t usRegReadCount;
  uint16_t usRegWriteAddress;
  uint16_t usRegWriteCount;
  uint8_t *ucMBFrame;

  eMBException eStatus = MB_EX_NONE;
  eMBErrorCode eRegStatus;

  /* If this request is broadcast, and it's read mode. This request don't need
   * execute.
   */

  if (xMBMasterRequestIsBroadcast())
    {
      eStatus = MB_EX_NONE;
    }
  else if (*usLen >= MB_PDU_SIZE_MIN + MB_PDU_FUNC_READWRITE_SIZE_MIN)
    {
      vMBMasterGetPDUSndBuf(&ucMBFrame);
      usRegReadAddress =
        (uint16_t) (ucMBFrame[MB_PDU_REQ_READWRITE_READ_ADDR_OFF] << 8U);
      usRegReadAddress |=
        (uint16_t) (ucMBFrame[MB_PDU_REQ_READWRITE_READ_ADDR_OFF + 1]);
      usRegReadAddress++;

      usRegReadCount =
        (uint16_t) (ucMBFrame[MB_PDU_REQ_READWRITE_READ_REGCNT_OFF] << 8U);
      usRegReadCount |=
        (uint16_t) (ucMBFrame[MB_PDU_REQ_READWRITE_READ_REGCNT_OFF + 1]);

      usRegWriteAddress =
        (uint16_t) (ucMBFrame[MB_PDU_REQ_READWRITE_WRITE_ADDR_OFF] << 8U);
      usRegWriteAddress |=
        (uint16_t) (ucMBFrame[MB_PDU_REQ_READWRITE_WRITE_ADDR_OFF + 1]);
      usRegWriteAddress++;

      usRegWriteCount =
        (uint16_t) (ucMBFrame[MB_PDU_REQ_READWRITE_WRITE_REGCNT_OFF] << 8U);
      usRegWriteCount |=
        (uint16_t) (ucMBFrame[MB_PDU_REQ_READWRITE_WRITE_REGCNT_OFF + 1]);

      if ((2 * usRegReadCount) ==
          pucFrame[MB_PDU_FUNC_READWRITE_READ_BYTECNT_OFF])
        {
          /* Make callback to update the register values. */

          eRegStatus =
            eMBMasterRegHoldingCB(&ucMBFrame
                                  [MB_PDU_REQ_READWRITE_WRITE_VALUES_OFF],
                                  usRegWriteAddress, usRegWriteCount,
                                  MB_REG_WRITE);

          if (eRegStatus == MB_ENOERR)
            {
              /* Make the read callback. */

              eRegStatus =
                eMBMasterRegHoldingCB(&pucFrame
                                      [MB_PDU_FUNC_READWRITE_READ_VALUES_OFF],
                                      usRegReadAddress, usRegReadCount,
                                      MB_REG_READ);
            }

          if (eRegStatus != MB_ENOERR)
            {
              eStatus = prveMBError2Exception(eRegStatus);
            }
        }
      else
        {
          eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }

  return eStatus;
}

#endif
#endif
