/****************************************************************************
 * apps/include/modbus/mbport.h
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

#ifndef __APPS_INCLUDE_MODBUS_MBPORT_H
#define __APPS_INCLUDE_MODBUS_MBPORT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

typedef enum
{
  EV_READY,                   /* Startup finished. */
  EV_FRAME_RECEIVED,          /* Frame received. */
  EV_EXECUTE,                 /* Execute function. */
  EV_FRAME_SENT               /* Frame sent. */
} eMBEventType;

typedef enum
{
  EV_MASTER_READY                  = 1<<0,  /* Startup finished. */
  EV_MASTER_FRAME_RECEIVED         = 1<<1,  /* Frame received. */
  EV_MASTER_EXECUTE                = 1<<2,  /* Execute function. */
  EV_MASTER_FRAME_SENT             = 1<<3,  /* Frame sent. */
  EV_MASTER_ERROR_PROCESS          = 1<<4,  /* Frame error process. */
  EV_MASTER_PROCESS_SUCESS         = 1<<5,  /* Request process success. */
  EV_MASTER_ERROR_RESPOND_TIMEOUT  = 1<<6,  /* Request respond timeout. */
  EV_MASTER_ERROR_RECEIVE_DATA     = 1<<7,  /* Request receive data error. */
  EV_MASTER_ERROR_EXECUTE_FUNCTION = 1<<8   /* Request execute function error. */
} eMBMasterEventType;

typedef enum
{
  EV_ERROR_RESPOND_TIMEOUT,   /* Slave respond timeout. */
  EV_ERROR_RECEIVE_DATA,      /* Receive frame data erroe. */
  EV_ERROR_EXECUTE_FUNCTION   /* Execute function error. */
} eMBMasterErrorEventType;

/* Parity used for characters in serial mode.
 *
 * The parity which should be applied to the characters sent over the serial
 * link. Please note that this values are actually passed to the porting
 * layer and therefore not all parity modes might be available.
 */

typedef enum
{
  MB_PAR_NONE,                /* No parity. */
  MB_PAR_ODD,                 /* Odd parity. */
  MB_PAR_EVEN                 /* Even parity. */
} eMBParity;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Callback function for the porting layer when a new byte is available.
 *
 * Depending upon the mode this callback function is used by the RTU or
 * ASCII transmission layers. In any case a call to xMBPortSerialGetByte()
 * must immediately return a new character.
 *
 * Return true if a event was posted to the queue because a new byte was
 * received. The port implementation should wake up the tasks which are
 * currently blocked on the eventqueue.
 */

extern bool(*pxMBFrameCBByteReceived)(void);
extern bool(*pxMBFrameCBTransmitterEmpty)(void);
extern bool(*pxMBPortCBTimerExpired)(void);

extern bool(*pxMBMasterFrameCBByteReceived)(void);
extern bool(*pxMBMasterFrameCBTransmitterEmpty)(void);
extern bool(*pxMBMasterPortCBTimerExpired)(void);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Supporting functions */

bool xMBPortEventInit(void);
bool xMBPortEventPost(eMBEventType eEvent);
bool xMBPortEventGet(eMBEventType *eEvent);

bool xMBMasterPortEventInit(void);
bool xMBMasterPortEventPost(eMBMasterEventType eEvent);
bool xMBMasterPortEventGet(eMBMasterEventType * eEvent);
void vMBMasterOsResInit(void);
bool xMBMasterRunResTake(int32_t time);
void vMBMasterRunResRelease(void);

/* Serial port functions */

bool xMBPortSerialInit(uint8_t ucPort, speed_t ulBaudRate,
                       uint8_t ucDataBits, eMBParity eParity);
void vMBPortClose(void);
void xMBPortSerialClose(void);
void vMBPortSerialEnable(bool xRxEnable, bool xTxEnable);
bool xMBPortSerialGetByte(int8_t * pucByte);
bool xMBPortSerialPutByte(int8_t ucByte);

bool xMBMasterPortSerialInit(uint8_t ucPort, speed_t ulBaudRate,
                             uint8_t ucDataBits, eMBParity eParity);
void vMBMasterPortClose(void);
void xMBMasterPortSerialClose(void);
void vMBMasterPortSerialEnable(bool xRxEnable, bool xTxEnable);
bool xMBMasterPortSerialGetByte(int8_t * pucByte);
bool xMBMasterPortSerialPutByte(int8_t ucByte);

/* Timers functions */

bool xMBPortTimersInit(uint16_t usTimeOut50us);
void xMBPortTimersClose(void);
void vMBPortTimersEnable(void);
void vMBPortTimersDisable(void);
void vMBPortTimersDelay(uint16_t usTimeOutMS);

bool xMBMasterPortTimersInit(uint16_t usTimeOut50us);
void xMBMasterPortTimersClose(void);
void vMBMasterPortTimersT35Enable(void);
void vMBMasterPortTimersConvertDelayEnable(void);
void vMBMasterPortTimersRespondTimeoutEnable(void);
void vMBMasterPortTimersDisable(void);

/* Callback for the master error process */

void vMBMasterErrorCBRespondTimeout(uint8_t ucDestAddress, const uint8_t *pucPDUData,
                                    uint16_t ucPDULength);
void vMBMasterErrorCBReceiveData(uint8_t ucDestAddress, const uint8_t *pucPDUData,
                                 uint16_t ucPDULength);
void vMBMasterErrorCBExecuteFunction(uint8_t ucDestAddress, const uint8_t *pucPDUData,
                                     uint16_t ucPDULength);
void vMBMasterCBRequestScuuess(void);

#ifdef CONFIG_MB_TCP_ENABLED
/* TCP port function */

bool xMBTCPPortInit(uint16_t usTCPPort);
#ifdef CONFIG_MB_HAVE_CLOSE
void vMBTCPPortClose(void);
#endif
void vMBTCPPortDisable(void);
bool xMBTCPPortGetRequest(uint8_t **ppucMBTCPFrame, uint16_t * usTCPLength);
bool xMBTCPPortSendResponse(const uint8_t *pucMBTCPFrame, uint16_t usTCPLength);
#endif

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif /* __APPS_INCLUDE_MODBUS_MBPORT_H */
