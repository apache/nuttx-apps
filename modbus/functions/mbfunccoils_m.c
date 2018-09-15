/****************************************************************************
 * apps/modbus/functions/mbfunccoils_m.c
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
#include "modbus/mbport.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MB_PDU_REQ_READ_ADDR_OFF            (MB_PDU_DATA_OFF + 0)
#define MB_PDU_REQ_READ_COILCNT_OFF         (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_READ_SIZE                (4)
#define MB_PDU_FUNC_READ_COILCNT_OFF        (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_READ_VALUES_OFF         (MB_PDU_DATA_OFF + 1)
#define MB_PDU_FUNC_READ_SIZE_MIN           (1)

#define MB_PDU_REQ_WRITE_ADDR_OFF           (MB_PDU_DATA_OFF)
#define MB_PDU_REQ_WRITE_VALUE_OFF          (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_WRITE_SIZE               (4)
#define MB_PDU_FUNC_WRITE_ADDR_OFF          (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_WRITE_VALUE_OFF         (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_SIZE              (4)

#define MB_PDU_REQ_WRITE_MUL_ADDR_OFF       (MB_PDU_DATA_OFF)
#define MB_PDU_REQ_WRITE_MUL_COILCNT_OFF    (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_WRITE_MUL_BYTECNT_OFF    (MB_PDU_DATA_OFF + 4)
#define MB_PDU_REQ_WRITE_MUL_VALUES_OFF     (MB_PDU_DATA_OFF + 5)
#define MB_PDU_REQ_WRITE_MUL_SIZE_MIN       (5)
#define MB_PDU_REQ_WRITE_MUL_COILCNT_MAX    (0x07B0)
#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF      (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF   (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_MUL_SIZE          (5)

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
 *   This function will request read coil.
 *
 * Input Parameters:
 *   ucSndAddr salve address
 *   usCoilAddr coil start address
 *   usNCoils coil total number
 *   lTimeOut timeout (-1 will waiting forever)
 *
 * Returned Value:
 *   error code
 *
 ****************************************************************************/

#ifdef CONFIG_MB_MASTER_FUNC_READ_COILS_ENABLED
eMBMasterReqErrCode eMBMasterReqReadCoils(uint8_t ucSndAddr,
                                          uint16_t usCoilAddr,
                                          uint16_t usNCoils,
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
      ucMBFrame[MB_PDU_FUNC_OFF] = MB_FUNC_READ_COILS;
      ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF] = usCoilAddr >> 8;
      ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1] = usCoilAddr;
      ucMBFrame[MB_PDU_REQ_READ_COILCNT_OFF] = usNCoils >> 8;
      ucMBFrame[MB_PDU_REQ_READ_COILCNT_OFF + 1] = usNCoils;
      vMBMasterSetPDUSndLength(MB_PDU_SIZE_MIN + MB_PDU_REQ_READ_SIZE);
      (void)xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
      eErrStatus = eMBMasterWaitRequestFinish();
    }

  return eErrStatus;
}

eMBException eMBMasterFuncReadCoils(uint8_t *pucFrame, uint16_t *usLen)
{
  uint8_t *ucMBFrame;
  uint16_t usRegAddress;
  uint16_t usCoilCount;
  uint8_t ucByteCount;

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

      usCoilCount = (uint16_t) (ucMBFrame[MB_PDU_REQ_READ_COILCNT_OFF] << 8);
      usCoilCount |= (uint16_t) (ucMBFrame[MB_PDU_REQ_READ_COILCNT_OFF + 1]);

      /* Test if the quantity of coils is a multiple of 8. If not last byte is
       * only partially field with unused coils set to zero.
       */

      if ((usCoilCount & 0x0007) != 0)
        {
          ucByteCount = (uint8_t) (usCoilCount / 8 + 1);
        }
      else
        {
          ucByteCount = (uint8_t) (usCoilCount / 8);
        }

      /* Check if the number of registers to read is valid. If not return
       * Modbus illegal data value exception.
       */

      if ((usCoilCount >= 1) &&
          (ucByteCount == pucFrame[MB_PDU_FUNC_READ_COILCNT_OFF]))
        {
          /* Make callback to fill the buffer. */

          eRegStatus =
            eMBMasterRegCoilsCB(&pucFrame[MB_PDU_FUNC_READ_VALUES_OFF],
                                usRegAddress, usCoilCount, MB_REG_READ);

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
      /* Can't be a valid read coil register request because the length is
       * incorrect.
       */

      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }

  return eStatus;
}
#endif

/****************************************************************************
 * Description:
 *   This function will request write one coil.
 *
 * Input Parameters:
 *   ucSndAddr salve address
 *   usCoilAddr coil start address
 *   usCoilData data to be written
 *   lTimeOut timeout (-1 will waiting forever)
 *
 * Returned Value:
 *   error code
 *
 * See eMBMasterReqWriteMultipleCoils
 *
 ****************************************************************************/

#ifdef CONFIG_MB_MASTER_FUNC_WRITE_COIL_ENABLED
eMBMasterReqErrCode eMBMasterReqWriteCoil(uint8_t ucSndAddr,
                                          uint16_t usCoilAddr,
                                          uint16_t usCoilData,
                                          uint32_t lTimeOut)
{
  uint8_t *ucMBFrame;
  eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;

  if (ucSndAddr > CONFIG_MB_MASTER_TOTAL_SLAVE_NUM)
    {
      eErrStatus = MB_MRE_ILL_ARG;
    }
  else if ((usCoilData != 0xFF00) && (usCoilData != 0x0000))
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
      ucMBFrame[MB_PDU_FUNC_OFF] = MB_FUNC_WRITE_SINGLE_COIL;
      ucMBFrame[MB_PDU_REQ_WRITE_ADDR_OFF] = usCoilAddr >> 8;
      ucMBFrame[MB_PDU_REQ_WRITE_ADDR_OFF + 1] = usCoilAddr;
      ucMBFrame[MB_PDU_REQ_WRITE_VALUE_OFF] = usCoilData >> 8;
      ucMBFrame[MB_PDU_REQ_WRITE_VALUE_OFF + 1] = usCoilData;
      vMBMasterSetPDUSndLength(MB_PDU_SIZE_MIN + MB_PDU_REQ_WRITE_SIZE);
      (void)xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
      eErrStatus = eMBMasterWaitRequestFinish();
    }

  return eErrStatus;
}

eMBException eMBMasterFuncWriteCoil(uint8_t *pucFrame, uint16_t *usLen)
{
  uint16_t usRegAddress;
  uint8_t ucBuf[2];

  eMBException eStatus = MB_EX_NONE;
  eMBErrorCode eRegStatus;

  if (*usLen == (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN))
    {
      usRegAddress = (uint16_t) (pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8);
      usRegAddress |= (uint16_t) (pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]);
      usRegAddress++;

      if ((pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF + 1] == 0x00) &&
          ((pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF) ||
           (pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0x00)))
        {
          ucBuf[1] = 0;
          if (pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF)
            {
              ucBuf[0] = 1;
            }
          else
            {
              ucBuf[0] = 0;
            }

          eRegStatus = eMBMasterRegCoilsCB(&ucBuf[0], usRegAddress, 1,
                                           MB_REG_WRITE);

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
      /* Can't be a valid write coil register request because the length is
       * incorrect.
       */

      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }

  return eStatus;
}
#endif

/****************************************************************************
 * Description:
 *   This function will request write multiple coils.
 *
 * Input Parameters:
 *   ucSndAddr salve address
 *   usCoilAddr coil start address
 *   usNCoils coil total number
 *   usCoilData data to be written
 *   lTimeOut timeout (-1 will waiting forever)
 *
 * Returned Value:
 *   error code
 *
 * See eMBMasterReqWriteCoil
 *
 ****************************************************************************/

#ifdef CONFIG_MB_MASTER_FUNC_WRITE_MULTIPLE_COILS_ENABLED
eMBMasterReqErrCode eMBMasterReqWriteMultipleCoils(uint8_t ucSndAddr,
                                                   uint16_t usCoilAddr,
                                                   uint16_t usNCoils,
                                                   uint8_t *pucDataBuffer,
                                                   uint32_t lTimeOut)
{
  uint8_t *ucMBFrame;
  uint16_t usRegIndex = 0;
  uint8_t ucByteCount;
  eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;

  if (ucSndAddr > CONFIG_MB_MASTER_TOTAL_SLAVE_NUM)
    {
      eErrStatus = MB_MRE_ILL_ARG;
    }
  else if (usNCoils > MB_PDU_REQ_WRITE_MUL_COILCNT_MAX)
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
      ucMBFrame[MB_PDU_FUNC_OFF] = MB_FUNC_WRITE_MULTIPLE_COILS;
      ucMBFrame[MB_PDU_REQ_WRITE_MUL_ADDR_OFF] = usCoilAddr >> 8;
      ucMBFrame[MB_PDU_REQ_WRITE_MUL_ADDR_OFF + 1] = usCoilAddr;
      ucMBFrame[MB_PDU_REQ_WRITE_MUL_COILCNT_OFF] = usNCoils >> 8;
      ucMBFrame[MB_PDU_REQ_WRITE_MUL_COILCNT_OFF + 1] = usNCoils;

      if ((usNCoils & 0x0007) != 0)
        {
          ucByteCount = (uint8_t) (usNCoils / 8 + 1);
        }
      else
        {
          ucByteCount = (uint8_t) (usNCoils / 8);
        }

      ucMBFrame[MB_PDU_REQ_WRITE_MUL_BYTECNT_OFF] = ucByteCount;
      ucMBFrame += MB_PDU_REQ_WRITE_MUL_VALUES_OFF;

      while (ucByteCount > usRegIndex)
        {
          *ucMBFrame++ = pucDataBuffer[usRegIndex++];
        }

      vMBMasterSetPDUSndLength(MB_PDU_SIZE_MIN + MB_PDU_REQ_WRITE_MUL_SIZE_MIN +
                               ucByteCount);
      (void)xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
      eErrStatus = eMBMasterWaitRequestFinish();
    }

  return eErrStatus;
}

eMBException eMBMasterFuncWriteMultipleCoils(uint8_t *pucFrame,
                                             uint16_t *usLen)
{
  uint16_t usRegAddress;
  uint16_t usCoilCnt;
  uint8_t ucByteCount;
  uint8_t ucByteCountVerify;
  uint8_t *ucMBFrame;

  eMBException eStatus = MB_EX_NONE;
  eMBErrorCode eRegStatus;

  /* If this request is broadcast, the *usLen is not need check. */

  if ((*usLen == MB_PDU_FUNC_WRITE_MUL_SIZE) || xMBMasterRequestIsBroadcast())
    {
      vMBMasterGetPDUSndBuf(&ucMBFrame);
      usRegAddress = (uint16_t) (pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8);
      usRegAddress |= (uint16_t) (pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]);
      usRegAddress++;

      usCoilCnt = (uint16_t) (pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF] << 8);
      usCoilCnt |= (uint16_t) (pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1]);

      ucByteCount = ucMBFrame[MB_PDU_REQ_WRITE_MUL_BYTECNT_OFF];

      /* Compute the number of expected bytes in the request. */

      if ((usCoilCnt & 0x0007) != 0)
        {
          ucByteCountVerify = (uint8_t) (usCoilCnt / 8 + 1);
        }
      else
        {
          ucByteCountVerify = (uint8_t) (usCoilCnt / 8);
        }

      if ((usCoilCnt >= 1) && (ucByteCountVerify == ucByteCount))
        {
          eRegStatus =
            eMBMasterRegCoilsCB(&ucMBFrame[MB_PDU_REQ_WRITE_MUL_VALUES_OFF],
                                usRegAddress, usCoilCnt, MB_REG_WRITE);

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
      /* Can't be a valid write coil register request because the length is
       * incorrect.
       */

      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }

  return eStatus;
}

#endif
#endif
