/****************************************************************************
 * apps/functions/mbfunccoils.c
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2006 Christian Walter <wolti@sil.at>
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
#include "modbus/mbframe.h"
#include "modbus/mbproto.h"
#include "modbus/mbport.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MB_PDU_FUNC_READ_ADDR_OFF           (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_READ_COILCNT_OFF        (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_READ_SIZE               (4)
#define MB_PDU_FUNC_READ_COILCNT_MAX        (0x07D0)

#define MB_PDU_FUNC_WRITE_ADDR_OFF          (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_WRITE_VALUE_OFF         (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_SIZE              (4)

#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF      (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF   (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF   (MB_PDU_DATA_OFF + 4)
#define MB_PDU_FUNC_WRITE_MUL_VALUES_OFF    (MB_PDU_DATA_OFF + 5)
#define MB_PDU_FUNC_WRITE_MUL_SIZE_MIN      (5)
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_MAX   (0x07B0)

/****************************************************************************
 * External Function Prototypes
 ****************************************************************************/

eMBException prveMBError2Exception(eMBErrorCode eErrorCode);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_MB_FUNC_READ_COILS_ENABLED

eMBException eMBFuncReadCoils(uint8_t *pucFrame, uint16_t *usLen)
{
  uint16_t usRegAddress;
  uint16_t usCoilCount;
  uint8_t ucNBytes;
  uint8_t *pucFrameCur;

  eMBException eStatus = MB_EX_NONE;
  eMBErrorCode eRegStatus;

  if (*usLen == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN))
    {
      usRegAddress = (uint16_t)(pucFrame[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
      usRegAddress |= (uint16_t)(pucFrame[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
      usRegAddress++;

      usCoilCount = (uint16_t)(pucFrame[MB_PDU_FUNC_READ_COILCNT_OFF] << 8);
      usCoilCount |= (uint16_t)(pucFrame[MB_PDU_FUNC_READ_COILCNT_OFF + 1]);

      /* Check if the number of registers to read is valid. If not
       * return Modbus illegal data value exception.
       */

      if ((usCoilCount >= 1) &&
          (usCoilCount < MB_PDU_FUNC_READ_COILCNT_MAX))
        {
          /* Set the current PDU data pointer to the beginning. */

          pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
          *usLen = MB_PDU_FUNC_OFF;

          /* First byte contains the function code. */

          *pucFrameCur++ = MB_FUNC_READ_COILS;
          *usLen += 1;

          /* Test if the quantity of coils is a multiple of 8. If not last
           * byte is only partially field with unused coils set to zero.
           */

          if ((usCoilCount & 0x0007) != 0)
            {
              ucNBytes = (uint8_t)(usCoilCount / 8 + 1);
            }
          else
            {
              ucNBytes = (uint8_t)(usCoilCount / 8);
            }

          *pucFrameCur++ = ucNBytes;
          *usLen += 1;

          eRegStatus = eMBRegCoilsCB(pucFrameCur, usRegAddress, usCoilCount,
                                     MB_REG_READ);

          /* If an error occurred convert it into a Modbus exception. */

          if (eRegStatus != MB_ENOERR)
            {
              eStatus = prveMBError2Exception(eRegStatus);
            }
          else
            {
              /* The response contains the function code, the starting address
               * and the quantity of registers. We reuse the old values in the
               * buffer because they are still valid.
               */

              *usLen += ucNBytes;
            }
        }
      else
        {
          eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else
    {
      /* Can't be a valid read coil register request because the length
       * is incorrect.
       */

      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }

  return eStatus;
}

#ifdef CONFIG_MB_FUNC_WRITE_COIL_ENABLED
eMBException eMBFuncWriteCoil(uint8_t *pucFrame, uint16_t *usLen)
{
  uint16_t usRegAddress;
  uint8_t ucBuf[2];

  eMBException eStatus = MB_EX_NONE;
  eMBErrorCode eRegStatus;

  if (*usLen == (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN))
    {
      usRegAddress = (uint16_t)(pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8);
      usRegAddress |= (uint16_t)(pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]);
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

          eRegStatus = eMBRegCoilsCB(&ucBuf[0], usRegAddress, 1,
                                     MB_REG_WRITE);

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
      /* Can't be a valid write coil register request because the length
       * is incorrect.
       */

      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }

  return eStatus;
}

#endif

#ifdef CONFIG_MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED
eMBException eMBFuncWriteMultipleCoils(uint8_t *pucFrame, uint16_t *usLen)
{
  uint16_t usRegAddress;
  uint16_t usCoilCnt;
  uint8_t ucByteCount;
  uint8_t ucByteCountVerify;

  eMBException eStatus = MB_EX_NONE;
  eMBErrorCode eRegStatus;

  if (*usLen > (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN))
    {
      usRegAddress = (uint16_t)(pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8);
      usRegAddress |= (uint16_t)(pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]);
      usRegAddress++;

      usCoilCnt = (uint16_t)(pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF] << 8);
      usCoilCnt |= (uint16_t)(pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1]);

      ucByteCount = pucFrame[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];

      /* Compute the number of expected bytes in the request. */

      if ((usCoilCnt & 0x0007) != 0)
        {
          ucByteCountVerify = (uint8_t)(usCoilCnt / 8 + 1);
        }
      else
        {
          ucByteCountVerify = (uint8_t)(usCoilCnt / 8);
        }

      if ((usCoilCnt >= 1) &&
          (usCoilCnt <= MB_PDU_FUNC_WRITE_MUL_COILCNT_MAX) &&
          (ucByteCountVerify == ucByteCount))
        {
          eRegStatus =
            eMBRegCoilsCB(&pucFrame[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF],
                          usRegAddress, usCoilCnt, MB_REG_WRITE);

          /* If an error occurred convert it into a Modbus exception. */

          if (eRegStatus != MB_ENOERR)
            {
              eStatus = prveMBError2Exception(eRegStatus);
            }
          else
            {
              /* The response contains the function code, the starting address
               * and the quantity of registers. We reuse the old values in the
               * buffer because they are still valid.
               */

              *usLen = MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF;
            }
        }
      else
        {
          eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else
    {
      /* Can't be a valid write coil register request because the length
       * is incorrect.
       */

      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }

  return eStatus;
}

#endif
#endif
