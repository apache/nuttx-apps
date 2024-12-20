/****************************************************************************
 * apps/modbus/tcp/mbtcp.h
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

#ifndef __APPS_MODBUS_TCP_MBTCP_H
#define __APPS_MODBUS_TCP_MBTCP_H

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MB_TCP_PSEUDO_ADDRESS   255

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

eMBErrorCode eMBTCPDoInit(uint16_t ucTCPPort);
void         eMBTCPStart(void);
void         eMBTCPStop(void);
eMBErrorCode eMBTCPReceive(uint8_t *pucRcvAddress, uint8_t **pucFrame,
                           uint16_t *pusLength);
eMBErrorCode eMBTCPSend(uint8_t _unused, const uint8_t *pucFrame,
                        uint16_t usLength);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_MODBUS_TCP_MBTCP_H */
