/****************************************************************************
 * apps/functions/mbfuncdisc.c
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

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MB_PDU_FUNC_READ_ADDR_OFF           (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_READ_DISCCNT_OFF        (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_READ_SIZE               (4)
#define MB_PDU_FUNC_READ_DISCCNT_MAX        (0x07D0)

/****************************************************************************
 * External Function Prototypes
 ****************************************************************************/

eMBException prveMBError2Exception(eMBErrorCode eErrorCode);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_MB_FUNC_READ_DISCRETE_INPUTS_ENABLED
eMBException eMBFuncReadDiscreteInputs(uint8_t *pucFrame, uint16_t *usLen)
{
  uint16_t usRegAddress;
  uint16_t usDiscreteCnt;
  uint8_t ucNBytes;
  uint8_t *pucFrameCur;

  eMBException eStatus = MB_EX_NONE;
  eMBErrorCode eRegStatus;

  if (*usLen == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN))
    {
      usRegAddress = (uint16_t)(pucFrame[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
      usRegAddress |= (uint16_t)(pucFrame[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
      usRegAddress++;

      usDiscreteCnt = (uint16_t)(pucFrame[MB_PDU_FUNC_READ_DISCCNT_OFF] << 8);
      usDiscreteCnt |= (uint16_t)(pucFrame[MB_PDU_FUNC_READ_DISCCNT_OFF + 1]);

      /* Check if the number of registers to read is valid. If not
       * return Modbus illegal data value exception.
       */

      if ((usDiscreteCnt >= 1) &&
          (usDiscreteCnt < MB_PDU_FUNC_READ_DISCCNT_MAX))
        {
          /* Set the current PDU data pointer to the beginning. */

          pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
          *usLen = MB_PDU_FUNC_OFF;

          /* First byte contains the function code. */

          *pucFrameCur++ = MB_FUNC_READ_DISCRETE_INPUTS;
          *usLen += 1;

          /* Test if the quantity of coils is a multiple of 8. If not last
           * byte is only partially field with unused coils set to zero.
           */

          if ((usDiscreteCnt & 0x0007) != 0)
            {
              ucNBytes = (uint8_t) (usDiscreteCnt / 8 + 1);
            }
          else
            {
              ucNBytes = (uint8_t) (usDiscreteCnt / 8);
            }

          *pucFrameCur++ = ucNBytes;
          *usLen += 1;

          eRegStatus = eMBRegDiscreteCB(pucFrameCur, usRegAddress,
                                        usDiscreteCnt);

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

#endif
