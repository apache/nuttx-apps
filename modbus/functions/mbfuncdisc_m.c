/****************************************************************************
 * apps/modbus/functions/mbfuncdisc_m.c
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

#define MB_PDU_REQ_READ_ADDR_OFF            (MB_PDU_DATA_OFF + 0)
#define MB_PDU_REQ_READ_DISCCNT_OFF         (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_READ_SIZE                (4)
#define MB_PDU_FUNC_READ_DISCCNT_OFF        (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_READ_VALUES_OFF         (MB_PDU_DATA_OFF + 1)
#define MB_PDU_FUNC_READ_SIZE_MIN           (1)

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
 *   This function will request read discrete inputs.
 *
 * Input Parameters:
 *   ucSndAddr salve address
 *   usDiscreteAddr discrete start address
 *   usNDiscreteIn discrete total number
 *   lTimeOut timeout (-1 will waiting forever)
 *
 * Returned Value:
 *   error code
 *
 ****************************************************************************/

#ifdef CONFIG_MB_MASTER_FUNC_READ_DISCRETE_INPUTS_ENABLED
eMBMasterReqErrCode eMBMasterReqReadDiscreteInputs(uint8_t ucSndAddr,
                                                   uint16_t usDiscreteAddr,
                                                   uint16_t usNDiscreteIn,
                                                   uint32_t lTimeOut)
{
  uint8_t *ucMBFrame;
  eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;

  if (ucSndAddr > CONFIG_MB_MASTER_TOTAL_SLAVE_NUM)
    {
      eErrStatus = MB_MRE_ILL_ARG;
    }
  else if (xMBMasterRunResTake(lTimeOut) == FALSE)
    {
      eErrStatus = MB_MRE_MASTER_BUSY;
    }
  else
    {
      vMBMasterGetPDUSndBuf(&ucMBFrame);
      vMBMasterSetDestAddress(ucSndAddr);
      ucMBFrame[MB_PDU_FUNC_OFF] = MB_FUNC_READ_DISCRETE_INPUTS;
      ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF] = usDiscreteAddr >> 8;
      ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1] = usDiscreteAddr;
      ucMBFrame[MB_PDU_REQ_READ_DISCCNT_OFF] = usNDiscreteIn >> 8;
      ucMBFrame[MB_PDU_REQ_READ_DISCCNT_OFF + 1] = usNDiscreteIn;
      vMBMasterSetPDUSndLength(MB_PDU_SIZE_MIN + MB_PDU_REQ_READ_SIZE);
      (void)xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
      eErrStatus = eMBMasterWaitRequestFinish();
    }

  return eErrStatus;
}

eMBException eMBMasterFuncReadDiscreteInputs(uint8_t *pucFrame,
                                             uint16_t *usLen)
{
  uint16_t usRegAddress;
  uint16_t usDiscreteCnt;
  uint8_t ucNBytes;
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
  else if (*usLen >= MB_PDU_SIZE_MIN + MB_PDU_FUNC_READ_SIZE_MIN)
    {
      vMBMasterGetPDUSndBuf(&ucMBFrame);
      usRegAddress = (uint16_t) (ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF] << 8);
      usRegAddress |= (uint16_t) (ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1]);
      usRegAddress++;

      usDiscreteCnt = (uint16_t) (ucMBFrame[MB_PDU_REQ_READ_DISCCNT_OFF] << 8);
      usDiscreteCnt |= (uint16_t) (ucMBFrame[MB_PDU_REQ_READ_DISCCNT_OFF + 1]);

      /* Test if the quantity of coils is a multiple of 8. If not last byte is
       * only partially field with unused coils set to zero.
       */

      if ((usDiscreteCnt & 0x0007) != 0)
        {
          ucNBytes = (uint8_t) (usDiscreteCnt / 8 + 1);
        }
      else
        {
          ucNBytes = (uint8_t) (usDiscreteCnt / 8);
        }

      /* Check if the number of registers to read is valid. If not return
       * Modbus illegal data value exception.
       */

      if ((usDiscreteCnt >= 1) &&
          ucNBytes == pucFrame[MB_PDU_FUNC_READ_DISCCNT_OFF])
        {
          /* Make callback to fill the buffer. */

          eRegStatus =
            eMBMasterRegDiscreteCB(&pucFrame[MB_PDU_FUNC_READ_VALUES_OFF],
                                   usRegAddress, usDiscreteCnt);

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
#endif
