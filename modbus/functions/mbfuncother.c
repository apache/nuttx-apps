/****************************************************************************
 * apps/functions/mbfuncother.c
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

#ifdef CONFIG_MB_FUNC_OTHER_REP_SLAVEID_ENABLED

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t  ucMBSlaveID[CONFIG_MB_FUNC_OTHER_REP_SLAVEID_BUF];
static uint16_t usMBSlaveIDLen;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

eMBErrorCode eMBSetSlaveID(uint8_t ucSlaveID, bool xIsRunning,
                           uint8_t const *pucAdditional,
                           uint16_t usAdditionalLen)
{
  eMBErrorCode eStatus = MB_ENOERR;

  /* the first byte and second byte in the buffer is reserved for
   * the parameter ucSlaveID and the running flag. The rest of
   * the buffer is available for additional data.
   */

  if (usAdditionalLen + 2 < CONFIG_MB_FUNC_OTHER_REP_SLAVEID_BUF)
    {
      usMBSlaveIDLen = 0;
      ucMBSlaveID[usMBSlaveIDLen++] = ucSlaveID;
      ucMBSlaveID[usMBSlaveIDLen++] = (uint8_t)(xIsRunning ? 0xFF : 0x00);

      if (usAdditionalLen > 0)
        {
          memcpy(&ucMBSlaveID[usMBSlaveIDLen], pucAdditional,
                  (size_t)usAdditionalLen);
          usMBSlaveIDLen += usAdditionalLen;
        }
    }
  else
    {
      eStatus = MB_ENORES;
    }

  return eStatus;
}

eMBException eMBFuncReportSlaveID(uint8_t *pucFrame, uint16_t *usLen)
{
  memcpy(&pucFrame[MB_PDU_DATA_OFF], &ucMBSlaveID[0], (size_t)usMBSlaveIDLen);
  *usLen = (uint16_t)(MB_PDU_DATA_OFF + usMBSlaveIDLen);
  return MB_EX_NONE;
}

#endif
