/****************************************************************************
 * apps/modbus/tcp/mbtcp.c
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

#include "modbus/mb.h"

#include "port.h"
#include "mbtcp.h"

#ifdef CONFIG_MB_TCP_ENABLED

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* ----------------------- MBAP Header --------------------------------------*/
/*
 *
 * <------------------------ MODBUS TCP/IP ADU(1) ------------------------->
 *              <----------- MODBUS PDU (1') ---------------->
 *  +-----------+---------------+------------------------------------------+
 *  | TID | PID | Length | UID  |Code | Data                               |
 *  +-----------+---------------+------------------------------------------+
 *  |     |     |        |      |
 * (2)   (3)   (4)      (5)    (6)
 *
 * (2)  ... MB_TCP_TID          = 0 (Transaction Identifier - 2 Byte)
 * (3)  ... MB_TCP_PID          = 2 (Protocol Identifier - 2 Byte)
 * (4)  ... MB_TCP_LEN          = 4 (Number of bytes - 2 Byte)
 * (5)  ... MB_TCP_UID          = 6 (Unit Identifier - 1 Byte)
 * (6)  ... MB_TCP_FUNC         = 7 (Modbus Function Code)
 *
 * (1)  ... Modbus TCP/IP Application Data Unit
 * (1') ... Modbus Protocol Data Unit
 */

#define MB_TCP_TID          0
#define MB_TCP_PID          2
#define MB_TCP_LEN          4
#define MB_TCP_UID          6
#define MB_TCP_FUNC         7

#define MB_TCP_PROTOCOL_ID  0   /* 0 = Modbus Protocol */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

eMBErrorCode eMBTCPDoInit(uint16_t ucTCPPort)
{
  eMBErrorCode    eStatus = MB_ENOERR;

  if (xMBTCPPortInit(ucTCPPort) == false)
    {
      eStatus = MB_EPORTERR;
    }

   return eStatus;
}

void eMBTCPStart(void)
{
}

void eMBTCPStop(void)
{
   /* Make sure that no more clients are connected. */

  vMBTCPPortDisable();
}

eMBErrorCode eMBTCPReceive(uint8_t *pucRcvAddress, uint8_t **ppucFrame, uint16_t *pusLength)
{
  eMBErrorCode    eStatus = MB_EIO;
  uint8_t        *pucMBTCPFrame;
  uint16_t        usLength;
  uint16_t        usPID;

  if (xMBTCPPortGetRequest(&pucMBTCPFrame, &usLength) != false)
    {
      usPID = pucMBTCPFrame[MB_TCP_PID] << 8U;
      usPID |= pucMBTCPFrame[MB_TCP_PID + 1];

      if (usPID == MB_TCP_PROTOCOL_ID)
        {
          *ppucFrame = &pucMBTCPFrame[MB_TCP_FUNC];
          *pusLength = usLength - MB_TCP_FUNC;
          eStatus = MB_ENOERR;

          /* Modbus TCP does not use any addresses. Fake the source address such
           * that the processing part deals with this frame.
           */

          *pucRcvAddress = MB_TCP_PSEUDO_ADDRESS;
        }
    }
  else
    {
      eStatus = MB_EIO;
    }

  return eStatus;
}

eMBErrorCode eMBTCPSend(uint8_t _unused, const uint8_t * pucFrame, uint16_t usLength)
{
  eMBErrorCode    eStatus = MB_ENOERR;
  uint8_t        *pucMBTCPFrame = (uint8_t *) pucFrame - MB_TCP_FUNC;
  uint16_t        usTCPLength = usLength + MB_TCP_FUNC;

  /* The MBAP header is already initialized because the caller calls this
   * function with the buffer returned by the previous call. Therefore we
   * only have to update the length in the header. Note that the length
   * header includes the size of the Modbus PDU and the UID Byte. Therefore
   * the length is usLength plus one.
   */

  pucMBTCPFrame[MB_TCP_LEN] = (usLength + 1) >> 8U;
  pucMBTCPFrame[MB_TCP_LEN + 1] = (usLength + 1) & 0xFF;
  if (xMBTCPPortSendResponse(pucMBTCPFrame, usTCPLength) == false)
    {
      eStatus = MB_EIO;
    }

  return eStatus;
}

#endif /* CONFIG_MB_TCP_ENABLED */
