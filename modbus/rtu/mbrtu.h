/****************************************************************************
 * apps/modbus/rtu/mbrtu.h
 *
 * FreeModbus Library: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006 Christian Walter <wolti@sil.at>
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

#ifndef __APPS_MODBUS_RTU_MBRTU_H
#define __APPS_MODBUS_RTU_MBRTU_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

eMBErrorCode eMBRTUInit(uint8_t slaveAddress, uint8_t ucPort,
                        speed_t ulBaudRate, eMBParity eParity);
void eMBRTUStart(void);
void eMBRTUStop(void);
eMBErrorCode eMBRTUReceive(uint8_t *pucRcvAddress, uint8_t **pucFrame,
                           uint16_t *pusLength);
eMBErrorCode eMBRTUSend(uint8_t slaveAddress, const uint8_t *pucFrame,
                        uint16_t usLength);
bool xMBRTUReceiveFSM(void);
bool xMBRTUTransmitFSM(void);
bool xMBRTUTimerT15Expired(void);
bool xMBRTUTimerT35Expired(void);

#if defined(CONFIG_RTU_ASCII_MASTER)
eMBErrorCode eMBMasterRTUInit(uint8_t ucPort, speed_t ulBaudRate,
                              eMBParity eParity);
void eMBMasterRTUStart(void);
void eMBMasterRTUStop(void);
eMBErrorCode eMBMasterRTUReceive(uint8_t *pucRcvAddress, uint8_t **pucFrame,
                                 uint16_t *pusLength);
eMBErrorCode eMBMasterRTUSend(uint8_t slaveAddress, const uint8_t *pucFrame,
                              uint16_t usLength);
bool xMBMasterRTUReceiveFSM(void);
bool xMBMasterRTUTransmitFSM(void);
bool xMBMasterRTUTimerExpired(void);
#endif

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif /* __APPS_MODBUS_RTU_MBRTU_H */
